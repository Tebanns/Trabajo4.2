# Sistema IoT de Monitoreo y Actuación con ESP32, ESP-NOW, Flask y Supabase

**Asignatura:** DCSH01 - Desarrollo de Software para Hardware
**Evaluación:** Sumativa 3
**Institución:** Universidad Tecnológica de Chile INACAP / Centro de Formación Técnica

---

## 1. Integrantes y roles

| Integrante | Rol dentro del proyecto |
|---|---|
| Esteban Rivera - Sergio Paez_ | Firmware tarjeta Emisor / lectura de sensores |
| Esteban Rivera - Sergio Paez_ | Firmware tarjeta Receptor / actuación / backend Flask |
| Esteban Rivera - Sergio Paez_ | Integración Supabase / documentación / pruebas |

> Ajustar esta tabla según cómo se repartió realmente el trabajo del grupo.

---

## 2. Explicación general del sistema

Este proyecto implementa un sistema IoT de dos capas: **sensado + actuación local** en el
hardware, y **registro + monitoreo remoto** en la nube. La idea es simular un pequeño sistema
de vigilancia/control ambiental: un potenciómetro simula una variable física continua (por
ejemplo, un nivel o una intensidad regulable) y un sensor PIR detecta movimiento en el entorno.

El sistema funciona así, de punta a punta:

1. La **Tarjeta 1 (Emisor)** lee cada 3 segundos el potenciómetro y el sensor PIR.
2. Envía esos datos por **ESP-NOW** (protocolo de comunicación directa entre ESP32, visto en
   clases) hacia la **Tarjeta 2 (Receptor/Gateway)**.
3. La Tarjeta 2 recibe el paquete, **decide el estado del sistema** y enciende físicamente
   3 LEDs según ese estado (verde = normal, rojo = alerta, azul = modo manual).
4. La Tarjeta 2, que sí está conectada a la red WiFi local, envía esos mismos datos por
   **HTTP (POST)** a un backend hecho en **Flask (Python)**.
5. Flask guarda cada evento en una base de datos en la nube (**Supabase**, PostgreSQL) con
   fecha y hora exacta, y además sirve una página web tipo **monitor de terminal** que muestra
   el último estado del sistema y el historial reciente.
6. Desde ese mismo panel Flask, una persona puede **activar o desactivar manualmente** el LED
   azul de la Tarjeta 2, sin tocar el hardware. La tarjeta consulta ese comando cada 2 segundos
   por HTTP (GET) y actúa en consecuencia.

En otras palabras: los sensores "hablan" por ESP-NOW entre las dos placas, y la placa
receptora "traduce" eso a HTTP para conversar con Flask, que a su vez conversa con Supabase.
Flask es el único punto donde se toca JavaScript... y de hecho **no se usa JavaScript en
ningún lugar**: toda la interactividad del panel (botones, refresco automático) se logra con
HTML puro y `<meta http-equiv="refresh">`.

---

## 3. Justificación del rol de cada tarjeta

El grupo cuenta con **2 tarjetas ESP32**, por lo que cada una cumple una función claramente
distinta y complementaria, evitando que ninguna quede sin un propósito real dentro del sistema:

### Tarjeta 1 — "Emisor de sensores"
- **Por qué existe:** alguien tiene que estar físicamente cerca de los sensores (potenciómetro
  y PIR). Esta tarjeta se dedica *solo* a leer esas dos señales y transmitirlas.
- **Por qué no hace nada más:** al no conectarse a WiFi con Internet, ahorra recursos y
  simplifica su código; su única responsabilidad es "sensar y avisar". Esto también permite que
  esta tarjeta pueda ubicarse en un punto distinto (por ejemplo, otra habitación) mientras
  mantenga solo enlace ESP-NOW de corto alcance con la otra placa.

### Tarjeta 2 — "Receptor / Gateway / Actuador"
- **Por qué existe:** es el puente entre el mundo "local" (ESP-NOW, LEDs) y el mundo "remoto"
  (Internet, Flask, Supabase). Concentra tres responsabilidades relacionadas:
  1. **Recepción:** escucha los paquetes ESP-NOW del emisor.
  2. **Actuación real:** enciende/apaga 3 LEDs físicos según el estado del sistema.
  3. **Gateway:** traduce esos datos a HTTP para que un backend en Python (Flask), que no
     entiende ESP-NOW, pueda recibirlos y guardarlos en la nube.
- **Por qué se justifica juntar estas 3 tareas en una sola tarjeta:** con solo 2 ESP32
  disponibles, esta placa es la única con acceso a Internet, por lo que es el candidato natural
  para centralizar la actuación (que depende del estado recién recibido) y el reenvío a Flask
  (que también depende de ese mismo estado). Separarlas en una tercera tarjeta no aportaría
  valor adicional con el hardware disponible.

---

## 4. Diagrama del sistema

```
┌─────────────────────┐        ESP-NOW         ┌──────────────────────────────┐
│   TARJETA 1          │ ─────────────────────▶ │   TARJETA 2                   │
│   ESP32 "Emisor"      │   { potValor,          │   ESP32 "Receptor/Gateway"    │
│                        │     potPorcentaje,     │                                │
│  - Potenciómetro (34)  │     movimiento }       │  - LED verde (25) normal       │
│  - Sensor PIR   (32)   │                        │  - LED rojo  (26) alerta       │
│                        │                        │  - LED azul  (27) manual       │
└───────────────────────┘                        └───────────────┬────────────────┘
                                                                    │  WiFi / HTTP
                                                                    │  POST /api/datos
                                                                    │  GET  /api/comando
                                                                    ▼
                                                    ┌────────────────────────────────┐
                                                    │   BACKEND FLASK (Python)         │
                                                    │                                   │
                                                    │  /api/datos   -> guarda evento    │
                                                    │  /api/comando -> entrega comando  │
                                                    │  /            -> panel terminal   │
                                                    │  /comando     -> activa LED manual│
                                                    └───────────────┬───────────────────┘
                                                                    │ REST API
                                                                    ▼
                                                    ┌────────────────────────────────┐
                                                    │        SUPABASE (PostgreSQL)     │
                                                    │  tabla: eventos                  │
                                                    │  (fecha/hora, pot, pir, leds)     │
                                                    └───────────────────────────────────┘

Usuario humano ──▶ navegador ──▶ panel Flask (monitor terminal + botones ACTIVAR/DESACTIVAR)

---

## 5. Estructura del repositorio

```
proyecto_iot_esp32/
├── firmware/
│   ├── emisor_sensores/
│   │   └── emisor_sensores.ino        # Tarjeta 1: lee sensores, envía ESP-NOW
│   └── receptor_gateway/
│       └── receptor_gateway.ino       # Tarjeta 2: recibe, actúa LEDs, habla con Flask
├── backend/
│   ├── app.py                         # Backend Flask (Python puro)
│   ├── requirements.txt               # Dependencias Python
│   ├── .env.example                   # Plantilla de credenciales Supabase
│   └── templates/
│       └── dashboard.html             # Monitor tipo terminal (HTML + CSS, sin JS)
├── supabase/
│   └── crear_tabla.sql                # Script SQL para crear la tabla "eventos"
├── capturas/
│   └── LEEME.txt                      # Guía de qué evidencia fotografiar/grabar
├── .gitignore
└── README.md
```

---

## 6. Requisitos y materiales

- 2 x placas ESP32 DevKit
- 1 x potenciómetro (10kΩ recomendado)
- 1 x sensor de movimiento PIR (HC-SR501 o similar)
- 3 x LEDs (verde, rojo, azul) + resistencias de 220Ω
- Arduino IDE (con soporte para ESP32 instalado)
- Python 3.10+ y `pip`
- Una cuenta gratuita en [Supabase](https://supabase.com)
- Ambas placas y el computador con Flask conectados a la **misma red WiFi**

### Conexionado (pinout)

| Componente | Tarjeta | Pin ESP32 |
|---|---|---|
| Potenciómetro (señal central) | Emisor | GPIO34 |
| Sensor PIR (salida) | Emisor | GPIO35 |
| LED verde | Receptor | GPIO12 |
| LED rojo | Receptor | GPIO14 |
| LED azul | Receptor | GPIO22 |

---

## 7. Configuración de Supabase (paso a paso)

Como es la primera vez que se configura, aquí va el detalle completo:

1. Entra a [supabase.com](https://supabase.com) y crea una cuenta gratuita (puedes usar tu
   cuenta de GitHub para registrarte más rápido).
2. Haz clic en **"New project"**. Ponle un nombre (ej: `proyecto-iot-dcsh01`), elige una
   contraseña para la base de datos (guárdala, no es la API key) y selecciona una región
   cercana (ej: `South America (São Paulo)`).
3. Espera 1-2 minutos mientras Supabase aprovisiona el proyecto.
4. En el menú lateral izquierdo, entra a **"SQL Editor" → "New query"**.
5. Copia y pega el contenido del archivo [`supabase/crear_tabla.sql`](supabase/crear_tabla.sql)
   de este repositorio, y presiona **"Run"**. Esto crea la tabla `eventos` con todas sus
   columnas, incluyendo `created_at` (fecha y hora automática).
6. Ve a **"Project Settings" (ícono de engranaje) → "API"**.
7. Copia dos valores:
   - **Project URL** → lo usarás como `SUPABASE_URL`
   - **anon public key** (o `service_role` si prefieres más permisos) → lo usarás como
     `SUPABASE_KEY`
8. En este repositorio, entra a la carpeta `backend/`, copia el archivo `.env.example` y
   renómbralo a `.env`. Pega ahí tus valores reales:

```
SUPABASE_URL=https://tu-proyecto.supabase.co
SUPABASE_KEY=tu-api-key-real
``

---

## 8. Instrucciones para ejecutar el proyecto

### Paso 1 — Backend Flask

```bash
cd backend
python -m venv venv
source venv/bin/activate        # En Windows: venv\Scripts\activate
pip install -r requirements.txt
python app.py
```

El servidor quedará escuchando en `http://0.0.0.0:5000`. Anota la **IP local** del
computador (en Windows: `ipconfig`, en Linux/Mac: `ifconfig` o `ip a`), ya que la Tarjeta 2 la
necesitará para saber a quién enviarle los datos.

### Paso 2 — Firmware Tarjeta 2 (Receptor/Gateway)

1. Abre `firmware/receptor_gateway/receptor_gateway.ino` en Arduino IDE.
2. Instala (si falta) la placa **ESP32** desde el gestor de tarjetas, y la librería
   `HTTPClient` (viene incluida con el core de ESP32).
3. Cambia estas líneas por tus datos reales:
   ```cpp
   const char* ssid     = "Incendio";
   const char* password = "1234567899";
   const char* SERVIDOR_FLASK = "http://127.0.0.1:5000";
   ```
4. Sube el código. Abre el **Monitor Serial (115200 baudios)** y anota:
   - La **MAC** de esta tarjeta (la usarás en el emisor)
   - El **canal WiFi** en uso (debe coincidir en el emisor)

### Paso 3 — Firmware Tarjeta 1 (Emisor)

1. Abre `firmware/emisor_sensores/emisor_sensores.ino`.
2. Reemplaza:
   ```cpp
   uint8_t macReceptor[] = {0x30, 0xC6, 0xF7, 0x29, 0x2D, 0xB0}; //
   #define CANAL_WIFI 11 // 
   ```
3. Sube el código a la Tarjeta 1.

### Paso 4 — Verificación

1. En el Monitor Serial del **Emisor**, deberías ver lecturas del potenciómetro/PIR y
   `[ESP-NOW] Enviado OK` cada 3 segundos.
2. En el Monitor Serial del **Receptor**, deberías ver `[RX ESP-NOW] ...` y
   `[Flask] POST /api/datos -> 200`.
3. En el navegador, entra a `http://IP_DE_TU_PC:5000` y verifica que el panel muestre los
   datos actualizándose solo cada 3 segundos.
4. Presiona el botón **ACTIVAR** del panel y confirma que el LED azul de la Tarjeta 2 se
   encienda físicamente (y **DESACTIVAR** para apagarlo).
5. En Supabase, ve a **"Table Editor" → "eventos"** y confirma que los registros se están
   guardando con fecha y hora.

---

## 9. Cumplimiento de restricciones técnicas de la pauta

| Restricción | Cumplimiento |
|---|---|
| Integrantes ≤ tarjetas | 2 tarjetas, grupo acorde |
| Cada tarjeta con rol claro | Emisor (sensores) / Receptor-Gateway-Actuador |
| Protocolo visto en clase | ESP-NOW (entre placas) + HTTP (placa 2 ↔ Flask) |
| Monitor estilo terminal en Flask | `templates/dashboard.html`, estética de consola |
| Actuación real (mínimo 3 LEDs) | LED verde, rojo y azul en Tarjeta 2 |
| Activación desde Python/Flask | Botones ACTIVAR/DESACTIVAR → `/comando` → LED azul |
| Registro en Supabase con fecha/hora | Columna `created_at` (automática) en tabla `eventos` |
| Repositorio en GitHub | Este repositorio |
| Sin JavaScript | Todo el frontend es HTML + CSS puro, refresco vía `<meta>` |
| Solo `id` en CSS (no `class`) | `dashboard.html` usa exclusivamente selectores `id` |
| Backend 100% Python | Flask, sin Node/otros backends |
| Base de datos relacional | Supabase = PostgreSQL |

---

## 10. Evidencia de funcionamiento

Ver carpeta [`capturas/`](capturas/) para capturas de pantalla y video corto de la demo.
