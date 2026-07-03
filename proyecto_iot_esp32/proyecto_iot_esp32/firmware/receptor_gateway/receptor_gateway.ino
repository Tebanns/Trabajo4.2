/* ============================================================
 * PROYECTO IOT - EVALUACIÓN SUMATIVA 3 - DCSH01
 * TARJETA 2: RECEPTOR / GATEWAY / ACTUADOR
 * ------------------------------------------------------------
 * ROL DE ESTA TARJETA:
 *   1) Recibe los datos del potenciómetro y PIR enviados por
 *      la tarjeta Emisora, usando ESP-NOW.
 *   2) Actúa físicamente sobre 3 LEDs según el estado del
 *      sistema (verde = normal, rojo = alerta, azul = modo
 *      manual activado desde Flask).
 *   3) Se conecta a WiFi con Internet/red local y actúa como
 *      "puente" (gateway): envía los datos por HTTP (POST) al
 *      backend Flask, y consulta (GET) si hay algún comando
 *      manual pendiente desde el panel Flask.
 *
 *   Esta tarjeta es el único punto de conexión entre el mundo
 *   físico (sensores/LEDs) y la nube (Flask + Supabase).
 * ============================================================ */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>

// ============================================================
// CREDENCIALES WiFi  <- cambiar por la red real
// ============================================================
const char* ssid     = "Incendio";
const char* password = "1234567899";

// ============================================================
// DIRECCIÓN DEL BACKEND FLASK
// <- cambiar por la IP real del computador donde corre Flask
//    (debe estar en la misma red WiFi que esta tarjeta)
// ============================================================
const char* SERVIDOR_FLASK = "http://192.168.1.100:5000";

// ============================================================
// PINES DE ACTUACIÓN (LEDs) - requisito mínimo: 3 indicadores
// ============================================================
#define LED_VERDE   12   // Sistema OK / sin movimiento
#define LED_ROJO    14   // Alerta: movimiento detectado o potenciómetro alto
#define LED_AZUL    22   // Modo manual activado desde el panel Flask

// ============================================================
// Umbral de "alerta" según el potenciómetro (0-100%)
// ============================================================
#define UMBRAL_ALERTA_POT 80.0

// ============================================================
// INTERVALOS DE COMUNICACIÓN CON FLASK (no bloqueantes)
// ============================================================
const unsigned long INTERVALO_ENVIO_DATOS   = 3000; // ms -> POST /api/datos
const unsigned long INTERVALO_CONSULTA_CMD  = 2000; // ms -> GET  /api/comando

unsigned long ultimoEnvioDatos    = 0;
unsigned long ultimaConsultaCmd   = 0;

// ============================================================
// ESTRUCTURA DE DATOS RECIBIDA (idéntica a la del emisor)
// ============================================================
typedef struct {
  int   potValor;
  float potPorcentaje;
  bool  movimiento;
} SensorData;

SensorData datos;
bool       hayDatosNuevos = false;
bool       ledManualActivo = false;

// ============================================================
// CALLBACK: recepción de un paquete ESP-NOW
// ============================================================
void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData, int len) {
  memcpy(&datos, incomingData, sizeof(datos));
  hayDatosNuevos = true;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);
  apagarTodosLosLeds();

  conectarWiFi();
  mostrarInfoDeRed();
  inicializarEspNow();

  Serial.println("Receptor/Gateway listo.\n");
}

// ============================================================
// LOOP
// ============================================================
void loop() {

  if (hayDatosNuevos) {
    Serial.printf("[RX ESP-NOW] Pot: %d (%.1f%%)  PIR: %s\n",
      datos.potValor, datos.potPorcentaje,
      datos.movimiento ? "SI" : "no");
    hayDatosNuevos = false;
  }

  actualizarLEDs();

  unsigned long ahora = millis();

  if (ahora - ultimoEnvioDatos >= INTERVALO_ENVIO_DATOS) {
    enviarDatosAFlask();
    ultimoEnvioDatos = ahora;
  }

  if (ahora - ultimaConsultaCmd >= INTERVALO_CONSULTA_CMD) {
    consultarComandoFlask();
    ultimaConsultaCmd = ahora;
  }
}

// ============================================================
// FUNCIONES DE RED
// ============================================================

// Conecta la tarjeta a la red WiFi local (necesaria para hablar con Flask)
void conectarWiFi() {
  WiFi.mode(WIFI_AP_STA); // AP_STA permite además que ESP-NOW siga operando
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
}

// Muestra la IP, MAC y canal de esta tarjeta (dato clave para el emisor)
void mostrarInfoDeRed() {
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC (copiar en el emisor): ");
  Serial.println(WiFi.macAddress());

  uint8_t canalActual;
  wifi_second_chan_t canalSecundario;
  esp_wifi_get_channel(&canalActual, &canalSecundario);
  Serial.print("Canal WiFi en uso (usar el mismo en el emisor): ");
  Serial.println(canalActual);
}

// Inicializa ESP-NOW y registra el callback de recepción
void inicializarEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW no inició");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

// Envía por HTTP (POST, JSON) los datos actuales al backend Flask
void enviarDatosAFlask() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(String(SERVIDOR_FLASK) + "/api/datos");
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"pot_valor\":" + String(datos.potValor) + ",";
  json += "\"pot_porcentaje\":" + String(datos.potPorcentaje, 1) + ",";
  json += "\"movimiento\":" + String(datos.movimiento ? "true" : "false") + ",";
  json += "\"led_alerta\":" + String(digitalRead(LED_ROJO) ? "true" : "false");
  json += "}";

  int codigoRespuesta = http.POST(json);
  Serial.printf("[Flask] POST /api/datos -> %d\n", codigoRespuesta);
  http.end();
}

// Consulta por HTTP (GET) si el panel Flask activó el modo manual (LED azul)
void consultarComandoFlask() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(String(SERVIDOR_FLASK) + "/api/comando");
  int codigoRespuesta = http.GET();

  if (codigoRespuesta == 200) {
    String respuesta = http.getString();
    ledManualActivo = respuesta.indexOf("true") != -1;
  } else {
    Serial.printf("[Flask] GET /api/comando -> %d\n", codigoRespuesta);
  }
  http.end();
}

// ============================================================
// FUNCIONES DE ACTUACIÓN (LEDs)
// ============================================================

void apagarTodosLosLeds() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AZUL, LOW);
}

// Decide el estado de los 3 LEDs según sensores + comando manual
void actualizarLEDs() {
  bool alerta = datos.movimiento || (datos.potPorcentaje >= UMBRAL_ALERTA_POT);

  digitalWrite(LED_VERDE, !alerta);         // Verde: todo normal
  digitalWrite(LED_ROJO,  alerta);          // Rojo: alerta activa
  digitalWrite(LED_AZUL,  ledManualActivo); // Azul: control manual desde Flask
}
