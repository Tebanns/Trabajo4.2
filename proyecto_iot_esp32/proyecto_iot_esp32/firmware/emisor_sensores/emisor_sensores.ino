/* ============================================================
 * PROYECTO IOT - EVALUACIÓN SUMATIVA 3 - DCSH01
 * TARJETA 1: EMISOR DE SENSORES
 * ------------------------------------------------------------
 * ROL DE ESTA TARJETA:
 *   Esta placa es la encargada de tomar las lecturas físicas
 *   del sistema (potenciómetro y sensor de movimiento PIR) y
 *   transmitirlas de forma inalámbrica mediante el protocolo
 *   ESP-NOW hacia la tarjeta Receptor/Gateway.
 *
 *   No se conecta a ninguna red WiFi con Internet: su única
 *   tarea es leer sensores y enviar datos, lo que la hace
 *   liviana y de bajo consumo.
 * ============================================================ */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ============================================================
// PINES DE SENSORES
// ============================================================
#define PIN_POT   34   // Potenciómetro -> GPIO34 (ADC1, solo entrada)
#define PIN_PIR   35   // Sensor de movimiento PIR -> GPIO32 (digital)

// ============================================================
// CANAL WIFI
// Debe coincidir EXACTAMENTE con el canal que use la tarjeta
// Receptora (se imprime en su Monitor Serial al conectarse
// al router). Ver README para más detalle.
// ============================================================
#define CANAL_WIFI 11

// ============================================================
// MAC DE LA TARJETA RECEPTORA
// Se obtiene desde el Monitor Serial del Receptor al iniciar
// (WiFi.macAddress()). Reemplazar por la MAC real.
// ============================================================
uint8_t macReceptor[] = {0x30, 0xC6, 0xF7, 0x29, 0x2D, 0xB0};

// ============================================================
// ESTRUCTURA DE DATOS ENVIADA
// Debe ser IDÉNTICA (mismos campos, mismo orden) a la que
// espera la tarjeta Receptora.
// ============================================================
typedef struct {
  int   potValor;       // Valor crudo del potenciómetro (0 - 4095)
  float potPorcentaje;  // Valor mapeado a porcentaje (0 - 100%)
  bool  movimiento;     // true si el PIR detecta movimiento
} SensorData;

SensorData datos;

// ============================================================
// CALLBACK: se ejecuta automáticamente cuando ESP-NOW confirma
// (o falla) el envío de un paquete.
// ============================================================
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS
    ? "[ESP-NOW] Enviado OK"
    : "[ESP-NOW] Error al enviar");
}

// ============================================================
// SETUP: se ejecuta una sola vez al iniciar la placa
// ============================================================
void setup() {
  Serial.begin(115200);

  // El potenciómetro no necesita pinMode (analogRead lo maneja solo)
  pinMode(PIN_PIR, INPUT);

  // ESP-NOW requiere modo estación, sin conectarse a ningún router
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Fijamos el mismo canal que usa la tarjeta receptora
  esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE);

  Serial.print("MAC de este emisor: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Canal configurado: ");
  Serial.println(CANAL_WIFI);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: no se pudo iniciar ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // Registramos a la tarjeta receptora como "peer" (destinatario)
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, macReceptor, 6);
  peer.channel = CANAL_WIFI;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("ERROR: no se pudo agregar el receptor");
    return;
  }

  Serial.println("Emisor listo. Enviando cada 3 s...\n");
}

// ============================================================
// LOOP: se repite continuamente
// ============================================================
void loop() {
  leerSensores();
  mostrarPorSerial();
  enviarPorEspNow();
  delay(3000);
}

// ============================================================
// FUNCIONES AUXILIARES
// ============================================================

// Lee el potenciómetro y el PIR, y llena la estructura "datos"
void leerSensores() {
  datos.potValor      = analogRead(PIN_POT);              // 0 - 4095 (ADC de 12 bits)
  datos.potPorcentaje = (datos.potValor / 4095.0) * 100.0; // convertido a %
  datos.movimiento    = digitalRead(PIN_PIR) == HIGH;
}

// Imprime las lecturas actuales en el Monitor Serial (para depuración)
void mostrarPorSerial() {
  Serial.printf("Pot: %d (%.1f%%)  PIR: %s\n",
    datos.potValor, datos.potPorcentaje,
    datos.movimiento ? "SI" : "no");
}

// Envía la estructura "datos" a la tarjeta receptora vía ESP-NOW
void enviarPorEspNow() {
  esp_now_send(macReceptor, (uint8_t*) &datos, sizeof(datos));
}
