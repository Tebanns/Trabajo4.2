# Informe  Visualización de Datos X-Y con Flask

**Asignatura:** Desarrollo de Software para Hardware (DCSH01)
**Evaluación:** Sumativa 4.2
**Nivel:** Tercer año
**Actividad:** Visualización de Datos X-Y con Flask

---

## 1. Introducción

Este informe documenta el desarrollo de la actividad evaluada según la Pauta de Evaluación Sumativa 4.2 de DCSH01. El proyecto recibe datos de los ejes **X** e **Y** desde una aplicación móvil (vía socket TCP) y los representa en distintas vistas web servidas con Flask.

A diferencia de una primera versión de prueba, esta entrega fue ajustada para cumplir **estrictamente** las restricciones obligatorias indicadas en la pauta, detalladas en la sección 3.

---

## 2. Objetivos

### 2.1 Objetivo general

Aplicar Flask, Jinja, HTML y CSS para construir un sistema que reciba y visualice en tiempo (casi) real los datos de un sensor de movimiento, cumpliendo las restricciones técnicas establecidas por la pauta de evaluación.

### 2.2 Objetivos específicos

* Recibir los valores X e Y desde el sensor mediante sockets TCP.
* Representar los datos en una cuadrícula de al menos 4×4 celdas (Etapa 1).
* Agregar dos vistas adicionales con representaciones distintas de los mismos datos, navegables entre sí (Etapa 2).
* Cumplir las restricciones obligatorias: sin JavaScript, CSS solo dentro de `<head>`, y referenciado únicamente por `id`.
* Documentar el proyecto en este README.

---

## 3. Restricciones obligatorias y cómo se cumplen

| Restricción de la pauta | Cómo se cumple en este proyecto |
|---|---|
| No usar JavaScript (`<script>`) | Ningún archivo del proyecto contiene `<script>`. La actualización periódica de los datos se logra con `<meta http-equiv="refresh" content="3">`, que es HTML puro y hace que el navegador vuelva a pedir la página cada 3 segundos. |
| Solo Flask, HTML, CSS y Jinja | El backend usa únicamente Flask y la librería estándar (`socket`, `math`). El frontend usa solo HTML, CSS y plantillas Jinja (`{% extends %}`, `{% block %}`, `{% for %}`). |
| CSS dentro de `<head>` | Todo el CSS vive en el `<style>` definido en `templates/base.html` (variables y estilos comunes) y en los bloques `extra_style` de cada vista, que se insertan dentro de ese mismo `<style>` mediante Jinja. No hay estilos en línea (`style="..."`) repartidos en el `<body>`. |
| CSS solo por `id` (no clases) | Ningún elemento usa el atributo `class`. Cada elemento tiene un `id` único y cada regla CSS lo referencia con `#id`. Incluso los valores dinámicos (color del núcleo, alto del termómetro, ángulo de la aguja, celda activa de la cuadrícula) se inyectan con Jinja directamente dentro de reglas `#id { ... }`. |

---

## 4. Descripción de las vistas

### 4.1 Vista Principal — Cuadrícula (`/`) — Etapa 1

Incluye el título de la actividad, el nombre del estudiante (subtítulo) y una explicación de cómo se visualizan los datos, tal como exige la pauta. Los valores de X e Y se traducen en una posición dentro de una **cuadrícula de 8×8 celdas** (se eligió 8×8 en lugar del mínimo 4×4 para una representación más detallada): la columna depende de X, la fila depende de Y. La fila y columna activas se marcan en un tono tenue, y la celda exacta se resalta con una animación pulsante (en CSS puro, sin JavaScript).

### 4.2 Vista Termómetros (`/termometros`) — Etapa 2, pestaña 1

Representa los mismos valores de X e Y como dos termómetros verticales independientes. La altura de cada columna es proporcional al valor del eje correspondiente, y el color cambia entre verde y rojo según el signo del valor.

### 4.3 Vista Armería (`/arma`) — Etapa 2, pestaña 2

Representa la magnitud combinada de los ejes (√(X² + Y²)) mediante una ilustración de un cañón de energía: el color del núcleo cambia según el nivel de energía detectado, y un medidor con aguja gira en proporción a esa misma magnitud.

Las tres vistas comparten una barra de navegación, por lo que desde cualquiera de ellas se puede llegar a las otras dos.

---

## 5. Tecnologías utilizadas

| Tecnología | Uso en el proyecto |
|---|---|
| Python 3 / Flask | Servidor web y rutas (`/`, `/termometros`, `/arma`) |
| Sockets TCP | Comunicación con la aplicación móvil que envía X, Y |
| HTML5 / CSS3 | Estructura y estilo visual de las tres vistas |
| Jinja2 | Herencia de plantillas, bucles para generar la cuadrícula, e inyección de valores dinámicos dentro del CSS |
| `<meta http-equiv="refresh">` | Actualización periódica de la página sin usar JavaScript |

---

## 6. Estructura del proyecto

```bash
.
├── app.py
└── templates
    ├── base.html
    ├── index.html
    ├── termometros.html
    └── arma.html
```

---

## 7. Instrucciones de instalación y ejecución

### 7.1 Clonar el repositorio

```bash
git clone https://github.com/<usuario>/<repositorio> " insertando datos del repositorio del profesor para tener la base del proyecto"
cd <repositorio>
```

### 7.2 Instalar dependencias

```bash
pip install flask
```

### 7.3 Configurar la IP y el puerto del sensor

En `app.py`, editar las constantes según la red local utilizada:

```python
HOST = "10.178.118.219"   # IP del servidor/sensor
PORT = 12345               # Puerto del servidor/sensor
```


### 7.4 Ejecutar la aplicación

```bash
python3 app.py
```

### 7.5 Probar en el navegador

```
http://127.0.0.1:5000               # Vista Principal (cuadrícula)
http://127.0.0.1:5000/termometros   # Vista Termómetros
http://127.0.0.1:5000/arma          # Vista Armería
```

> **Nota:** la aplicación móvil que envía los datos X, Y debe estar conectada a la misma red local que el equipo donde corre Flask. Esta no se incluye en el repositorio; se hace referencia a su ubicación en la actividad base entregada por el docente.

---

## 8. Referencias

Esta entrega corresponde a un desarrollo personal; no se utilizaron visualizaciones de repositorios de compañeros para las pestañas de la Etapa 2.

---

## 9. Dificultades encontradas y soluciones

* **Sin JavaScript no es posible refrescar los datos por AJAX:** se solucionó usando `<meta http-equiv="refresh" content="3">`, de modo que el navegador vuelve a solicitar la página completa cada 3 segundos y Flask la re-renderiza con la lectura más reciente del sensor.
* **CSS solo por `id`, sin clases, para elementos repetidos (las 64 celdas de la cuadrícula):** se resolvió generando las reglas CSS con un bucle de Jinja dentro del propio `<style>`, de modo que cada celda mantiene su propio `id` único sin necesidad de una clase compartida.
* **Mostrar valores dinámicos (color, alto, ángulo) sin usar `style=""` en el `body`:** se resolvió inyectando esos valores directamente dentro de las reglas `#id { ... }` del `<style>` en el `<head>`, usando Jinja, en lugar de atributos de estilo en línea.

---

## 10. Conclusión

El desarrollo de esta actividad permitió reforzar el uso de Flask y Jinja para generar interfaces dinámicas sin depender de JavaScript, comprendiendo cómo combinar lógica de servidor (Python) con generación de CSS basada en plantillas. Las restricciones de la pauta (sin scripts, CSS solo por `id`) obligaron a buscar soluciones alternativas, como el uso de `meta refresh` y la generación de reglas CSS mediante bucles de Jinja, lo que profundizó el entendimiento de la separación entre lógica de backend y presentación.

---

## 11. Autor

* Esteban Rivera Zurita "Tebanns"
* 20-06-2026
