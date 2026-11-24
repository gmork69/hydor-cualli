/* ================================================================
 *  HYDOR-CUALLI · Nodo ESP8266 + ADS1115 · FIRMWARE PUENTE
 *  Lee TDS (agua) y MQ-135 (aire) SIMULTANEAMENTE y publica
 *  cada lectura en DOS destinos a la vez:
 *
 *    1) FIREBASE   -> lo que usa hoy el sitio web del taller/colectivo.
 *                     Se mantiene mientras el grupo siga usándolo.
 *    2) API PROPIO -> el servidor de Hydor-Cualli en el VPS, que sí
 *                     guarda el historial completo para siempre.
 *
 *  Por qué "puente": el día que el proyecto migre 100% al servidor
 *  propio, basta con cambiar UNA línea (USAR_FIREBASE a false, más
 *  abajo) para apagar todo lo de Firebase sin borrar una sola línea
 *  de código. Lo mismo al revés con USAR_API. Cada destino vive en
 *  su propio bloque "#if ... #endif": includes, variables y llamadas.
 *  Si un destino está apagado, ni su librería se compila (ahorra
 *  memoria flash y RAM, que en el ESP8266 escasean).
 *
 *  Diferencia con el firmware oficial del taller
 *  (hydor_cualli_embajador.ino):
 *    - El oficial lee UN sensor a la vez desde A0, eligiendo con un
 *      jumper físico (D7). Aquí no hay jumper: leemos los DOS
 *      sensores a la vez, cada uno en su propio canal del ADS1115.
 *    - El oficial no escribe nunca en "/historial", así que en la
 *      página web el historial sale vacío para todos. Este firmware
 *      SÍ lo llena (ver la función publicarEnFirebase más abajo).
 *
 * ================================================================
 *  CONEXIONES
 * ----------------------------------------------------------------
 *   ADS1115   ->   ESP8266 (NodeMCU)
 *     VDD     ->   3V3
 *     GND     ->   GND
 *     SCL     ->   D1  (GPIO5)
 *     SDA     ->   D2  (GPIO4)
 *     ADDR    ->   GND   (fija la dirección I2C en 0x48)
 *
 *   Sensor TDS (SEN0244)
 *     Alimentación: 3V3 y GND
 *     Señal        -> canal A0 del ADS1115, DIRECTO (sin nada en medio).
 *                     Su salida es 0-2.3V, el ADS a 3.3V la tolera bien.
 *
 *   Sensor MQ-135
 *     Alimentación: VCC -> pin "Vin" del NodeMCU (los 5V que trae el
 *                   USB), GND -> GND. Necesita 5V para que su
 *                   calentador interno llegue a temperatura; a 3.3V
 *                   el sensor no calienta lo suficiente y las
 *                   lecturas no sirven.
 *     Señal (AO)  -> NO VA DIRECTO AL ADS1115. Ver el aviso de abajo.
 *
 * ----------------------------------------------------------------
 *  ATENCION: EL DIVISOR DE VOLTAJE DEL MQ-135 (esto protege tu ADS1115)
 * ----------------------------------------------------------------
 *  El MQ-135 vive alimentado a 5V (lo necesita para el calentador),
 *  así que su salida analógica (AO) puede llegar hasta casi 5V.
 *
 *  El ADS1115, en cambio, está alimentado a 3.3V, y sus entradas
 *  analógicas NO TOLERAN más de aproximadamente 3.6V (VDD + 0.3V,
 *  el límite absoluto que marca el fabricante). Meterle los 5V del
 *  MQ-135 directo puede DAÑAR PERMANENTEMENTE EL CHIP.
 *
 *  La solución es un divisor de voltaje resistivo con dos
 *  resistencias IGUALES de 10 kOhm, así:
 *
 *        MQ-135                                    ADS1115
 *        salida (AO)                               canal A1
 *           |                                          |
 *           +------[ R1 = 10k ]------+------[ R2 = 10k ]------+
 *                                    |                        |
 *                                    +----> A1 del ADS1115    |
 *                                                              |
 *                                                             GND
 *
 *      (dibujado como circuito: AO --[10k]-- nodo --[10k]-- GND
 *       y el nodo del medio es lo que se conecta al canal A1)
 *
 *  Ese divisor parte el voltaje exactamente a la mitad: si el
 *  MQ-135 entrega 5V, al ADS1115 le llegan 2.5V. Seguro.
 *
 *  Como el ADS ve la MITAD del voltaje real, en el código hay que
 *  MULTIPLICAR por 2 lo que lee para saber el voltaje real que
 *  entregó el sensor (ver la constante DIVISOR_MQ135 más abajo).
 *  Si algún día conectas el MQ-135 SIN divisor -- cosa que NO se
 *  recomienda, es el mismo riesgo de dañar el ADS1115 -- tendrías
 *  que poner esa constante en 1.0, bajo tu propio riesgo.
 *
 * ================================================================
 *  Librerías, placa y solución de problemas: ver el comentario
 *  "GUIA RAPIDA" hasta el final de este archivo.
 * ================================================================
 */

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// ============================================================
//  ===== CONFIGURA ESTO =====
// ============================================================

// --- Interruptores maestros de cada destino -----------------
// Cuando el colectivo apague Firebase (o cuando ya no lo necesites),
// cambia esto a "false" y esa mitad del sketch deja de compilarse.
// Igual para el API propio. Puedes tener los dos encendidos a la
// vez (modo puente, el normal mientras dura la migración).
#define USAR_FIREBASE true    // hoy: cada quien publica a su propio Firebase
#define USAR_API      false   // servidor central: todavía no existe, se prende después

// --- WiFi, Firebase y API: viven en src/secretos.h ----------
// WIFI_SSID1/2, WIFI_PASSWORD1/2 (dos redes, por si la principal
// falla), FIREBASE_HOST, FIREBASE_AUTH, SECRETO_API_HOST y
// SECRETO_API_KEY se definen en src/secretos.h, que NO se sube a git.
// Copia src/secretos.ejemplo.h como src/secretos.h y llena tus datos.
#include "secretos.h"

#if USAR_API
// --- API propio: el servidor de Hydor-Cualli en el VPS ------
// Mientras no haya dominio, usa la IP directa del servidor.
const char* api_host = SECRETO_API_HOST;    // <-- en secretos.h
const int   api_port = 8080;                // <-- 8080 = frontend Nginx (ya expuesto), proxy /api -> backend
const char* api_key  = SECRETO_API_KEY;     // <-- llave del dashboard, en secretos.h
#endif

// --- Sensor de temperatura del agua (para compensar el TDS) -
// Este nodo NO trae sensor de temperatura, así que usamos un valor
// fijo razonable para agua a temperatura ambiente en México.
// Si más adelante agregas un DS18B20 sumergible, aquí es donde se
// enchufa: cambia esta constante por la lectura real del sensor
// justo antes de llamar a calcularTDS_ppm().
const float TEMP_AGUA_C = 25.0;

// --- Divisor de voltaje del MQ-135 (ver el aviso arriba) -----
// Si armaste el divisor de 10k + 10k tal como se explica arriba,
// deja esto en 2.0. Solo ponlo en 1.0 si conectaste el MQ-135
// DIRECTO al ADS1115 sin divisor -- lo cual arriesga dañar el chip.
const float DIVISOR_MQ135 = 2.0;

// --- ¿Este nodo tiene sensor de temperatura/humedad (DHT22)? -
// Hoy no. Cuando se agregue uno, cambia esto a "true" y agrega la
// lectura real (y los campos "temperatura"/"humedad" en el JSON
// del API, ver publicarEnAPI) en vez de omitirlos como se hace ahora.
#define TIENE_DHT false

// ============================================================
//  fin de "CONFIGURA ESTO"
// ============================================================

#if USAR_FIREBASE
  #include <FirebaseESP8266.h>
  #include <time.h>
#endif

#if USAR_API
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
#endif

// ---------------- ADS1115 ----------------
Adafruit_ADS1115 ads;
bool adsListo = false;
unsigned long ultimoIntentoAds = 0;
const unsigned long INTERVALO_REINTENTO_ADS = 60000; // 1 minuto

const int CH_TDS   = 0; // A0 del ADS -> TDS (agua)
const int CH_MQ135 = 1; // A1 del ADS -> MQ-135 (aire), via divisor

const int NUM_MUESTRAS = 10;
const unsigned long ESPERA_ENTRE_MUESTRAS = 20; // ms

// ---------------- WiFi (reconexion sin bloquear) ----------------
unsigned long ultimoIntentoWifi = 0;
const unsigned long INTERVALO_REINTENTO_WIFI = 30000; // 30 segundos
bool intentandoRed2 = false;

// ---------------- Publicacion ----------------
const unsigned long INTERVALO = 60000; // 60 s (para pruebas de banco puedes bajarlo a 10000)
unsigned long ultimaPublicacion = 0;

#if USAR_FIREBASE
FirebaseData   fbData;
FirebaseAuth   fbAuth;
FirebaseConfig fbConfig;
bool horaSincronizada = false;
#endif

// ================================================================
//  ADS1115: encendido con reintento (nunca se queda trabado)
// ================================================================
void intentarIniciarADS() {
  if (ads.begin(0x48)) {
    adsListo = true;
    ads.setGain(GAIN_ONE); // +/-4.096V, cómodo para 0-3.3V y el MQ135 tras el divisor
    Serial.println("[OK] ADS1115 detectado.");
  } else {
    adsListo = false;
    Serial.println("[X] No se encontro el ADS1115. Revisa el cableado: SDA=D2, SCL=D1, ADDR=GND. Se reintentara en 60s, sin detener el resto del nodo.");
  }
}

// ================================================================
//  WiFi: conexion y reconexion NO bloqueante
// ================================================================
void iniciarConexionWifi() {
  WiFi.mode(WIFI_STA);
  if (!intentandoRed2) {
    Serial.printf("[WiFi] Probando red 1 (%s)...\n", WIFI_SSID1);
    WiFi.begin(WIFI_SSID1, WIFI_PASSWORD1);
  } else {
    Serial.printf("[WiFi] Probando red 2 (%s)...\n", WIFI_SSID2);
    WiFi.begin(WIFI_SSID2, WIFI_PASSWORD2);
  }
}

// Se llama en cada vuelta del loop(). Si ya hay WiFi, no hace nada.
// Si no hay, reintenta cada INTERVALO_REINTENTO_WIFI sin usar ningun
// "while" que detenga el resto del nodo (lecturas, publicaciones, etc).
void gestionarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - ultimoIntentoWifi < INTERVALO_REINTENTO_WIFI) return;
  ultimoIntentoWifi = millis();
  intentandoRed2 = !intentandoRed2; // alterna entre red 1 y red 2 en cada intento
  Serial.println("[WiFi] Sin conexion. Reintentando...");
  iniciarConexionWifi();
}

// ================================================================
//  Lectura de un canal del ADS1115, promediada y filtrada
//  (misma idea que "leerPromedio" del firmware oficial, pero
//   leyendo del ADS1115 en vez de analogRead(A0))
// ================================================================
float leerPromedioADS(int canal) {
  float valores[NUM_MUESTRAS];
  float suma = 0;

  for (int i = 0; i < NUM_MUESTRAS; i++) {
    int16_t crudo = ads.readADC_SingleEnded(canal);
    valores[i] = ads.computeVolts(crudo);
    suma += valores[i];
    delay(ESPERA_ENTRE_MUESTRAS);
    yield(); // no disparar el watchdog del ESP8266
  }
  float promedio = suma / NUM_MUESTRAS;

  // descarta muestras que se alejan mas de 30% del promedio y recalcula
  float sumaFiltrada = 0;
  int contadas = 0;
  for (int i = 0; i < NUM_MUESTRAS; i++) {
    if (abs(valores[i] - promedio) <= promedio * 0.30) {
      sumaFiltrada += valores[i];
      contadas++;
    }
  }
  return (contadas > 0) ? (sumaFiltrada / contadas) : promedio;
}

// ================================================================
//  TDS: formula real de DFRobot con compensacion de temperatura
// ================================================================
float calcularTDS_ppm(float voltaje, float tempC) {
  float vComp = voltaje / (1.0 + 0.02 * (tempC - 25.0));
  float ppm = (133.42 * vComp * vComp * vComp
             - 255.86 * vComp * vComp
             + 857.39 * vComp) * 0.5;
  if (ppm < 0) ppm = 0;
  return ppm;
}

// ================================================================
//  MQ-135: NO inventamos ppm. Sin burn-in (24-48h encendido) y sin
//  calcular R0 en aire limpio, cualquier "ppm" que diéramos seria
//  falso. En vez de eso publicamos un INDICE relativo honesto: el
//  voltaje real (ya corregido por el divisor) en milivolts. Sirve
//  para comparar lecturas entre si (sube/baja) aunque no sea una
//  concentracion de gas exacta todavia.
//
//  TODO (cuando se calibre este sensor):
//    1) Dejarlo encendido 24-48h seguidas (burn-in) sin apagarlo.
//    2) Con el sensor ya estable y en aire limpio, medir su
//       resistencia (R0) siguiendo la hoja de datos del MQ-135.
//    3) Aplicar la curva RS/R0 de la hoja de datos para convertir
//       esa relacion en ppm reales de CO2/NH3/etc.
//    4) Solo entonces reemplazar esta funcion por el calculo de ppm.
// ================================================================
int calcularIndiceMQ135(float voltajeReal) {
  return (int)(voltajeReal * 1000); // milivolts
}

#if USAR_FIREBASE
// ================================================================
//  Hora (solo hace falta para los timestamps que ve el sitio del
//  taller). Intenta sincronizar por NTP hasta 15s y sigue adelante
//  aunque falle -- un nodo sin internet estable no se puede dar el
//  lujo de quedarse esperando la hora para siempre.
// ================================================================
bool sincronizarHora() {
  configTime(-6 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // Mexico Central (UTC-6)
  Serial.print("[NTP] Sincronizando hora");
  unsigned long inicio = millis();
  while (time(nullptr) < 100000 && millis() - inicio < 15000) {
    delay(500);
    Serial.print(".");
    yield();
  }
  if (time(nullptr) > 100000) {
    Serial.println(" listo!");
    return true;
  }
  Serial.println(" no se pudo sincronizar (se sigue con el uptime).");
  return false;
}

String obtenerTimestamp() {
  time_t ahora = time(nullptr);
  struct tm* t = localtime(&ahora);
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
  return String(buf);
}

String obtenerUptimeStr() {
  return String(millis() / 1000);
}

// Si la hora NTP no esta disponible, mandamos un texto que a
// proposito NO es una fecha valida (por eso el prefijo). Asi, del
// lado del sitio web, parseTimestamp() falla al interpretarlo y cae
// de vuelta a decodePushTime(key), que saca la hora real desde la
// propia clave que genera Firebase al hacer push(). Ver la funcion
// addHistorialEntry() en el index.html del colectivo.
String obtenerTimestampParaFirebase() {
  if (horaSincronizada) return obtenerTimestamp();
  return "sin-hora-ntp:" + obtenerUptimeStr();
}

// ================================================================
//  Publicar en Firebase (mismos nodos que el firmware oficial, mas
//  el historial que el oficial nunca llena)
// ================================================================
void publicarEnFirebase(float tds_ppm, int mq135_indice) {
  String timestamp = obtenerTimestampParaFirebase();

  Firebase.setFloat(fbData, "/sensor/agua", tds_ppm);
  Firebase.setFloat(fbData, "/sensor/aire", (float)mq135_indice);
  Firebase.setString(fbData, "/sensor/timestamp", timestamp);
  Firebase.setString(fbData, "/sensor/uptime", obtenerUptimeStr());
  Firebase.setString(fbData, "/sensor/estadoConexion", "conectado");

  // --- /historial ------------------------------------------------
  // El index.html del colectivo LEE el nodo "/historial" esperando,
  // por cada entrada empujada con push(), un objeto con esta forma:
  //   { "tipo": "agua" | "aire", "valor": <numero>, "timestamp": "YYYY-MM-DD HH:MM:SS" }
  // (ver la funcion addHistorialEntry() y el onValue(ref(db,'/historial'))
  // en su index.html). El firmware oficial del taller NUNCA escribe
  // ahi -- por eso a todos los participantes el historial les sale
  // vacio y cada lectura nueva pisa a la anterior en "/sensor".
  // Aqui SI lo llenamos: una entrada por tipo de sensor, en cada ciclo.
  FirebaseJson entradaAgua;
  entradaAgua.set("tipo", "agua");
  entradaAgua.set("valor", tds_ppm);
  entradaAgua.set("timestamp", timestamp);
  if (!Firebase.pushJSON(fbData, "/historial", entradaAgua)) {
    Serial.println("   [Firebase][X] No se pudo guardar /historial (agua): " + fbData.errorReason());
  }

  FirebaseJson entradaAire;
  entradaAire.set("tipo", "aire");
  entradaAire.set("valor", mq135_indice);
  entradaAire.set("timestamp", timestamp);
  if (!Firebase.pushJSON(fbData, "/historial", entradaAire)) {
    Serial.println("   [Firebase][X] No se pudo guardar /historial (aire): " + fbData.errorReason());
  }

  Serial.println("   [Firebase] publicado.");
}
#endif // USAR_FIREBASE

#if USAR_API
// ================================================================
//  Publicar en el API propio (POST HTTP con x-api-key)
//  JSON armado a mano con snprintf -- NO usamos ArduinoJson aqui a
//  proposito: el ESP8266 ya tiene poca RAM libre cargando WiFi +
//  Firebase, y el payload son nada mas 1 o 2 numeros. No vale la
//  pena la dependencia extra.
// ================================================================
void publicarEnAPI(float tds_ppm, int mq135_indice) {
  // Este nodo NO tiene sensor de temperatura ni humedad todavia.
  // A proposito NO mandamos "temperatura":0 ni "humedad":0 -- eso
  // el backend lo guardaria como si fueran mediciones reales de
  // 0 grados / 0% de humedad, y ensuciaria el historico para
  // siempre. Mejor omitir esos campos por completo.
  // (Cuando TIENE_DHT sea true y haya un sensor real, aqui se
  // agregarian con snprintf, ej: ",\"temperatura\":%.1f,\"humedad\":%d")
  char payload[96];
  snprintf(payload, sizeof(payload), "{\"tds\":%d,\"mq135\":%d}",
           (int)round(tds_ppm), mq135_indice);

  WiFiClient client;
  HTTPClient http;
  String url = "http://" + String(api_host) + ":" + String(api_port) + "/api/datos/publicar";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", api_key);

  int code = http.POST(payload);
  if (code > 0) {
    Serial.printf("   [API] HTTP %d\n", code);
    if (code != 200 && code != 201) {
      // Aqui es donde el participante ve, por ejemplo, "API key invalida"
      Serial.println("   [API] Respuesta: " + http.getString());
    }
  } else {
    Serial.printf("   [API][X] Error de conexion: %s\n", http.errorToString(code).c_str());
  }
  http.end();
}
#endif // USAR_API

// ================================================================
//  Ciclo de lectura + publicacion (llama a los dos destinos activos)
// ================================================================
void publicarLecturas() {
  if (!adsListo) {
    Serial.println("[!] ADS1115 no disponible todavia; se omite este ciclo de lectura.");
    return;
  }

  Serial.println("\n[>] Leyendo sensores...");

  float v_tds        = leerPromedioADS(CH_TDS);
  float v_mq135_crudo = leerPromedioADS(CH_MQ135);
  float v_mq135_real  = v_mq135_crudo * DIVISOR_MQ135; // deshace el divisor 10k+10k

  float tds_ppm      = calcularTDS_ppm(v_tds, TEMP_AGUA_C);
  int   mq135_indice = calcularIndiceMQ135(v_mq135_real);

  // Siempre se imprime el voltaje crudo de cada canal ademas del
  // valor ya convertido -- es lo que hace falta para calibrar despues.
  Serial.printf("   TDS:   %.2f ppm   (voltaje canal A0: %.3f V)\n", tds_ppm, v_tds);
  Serial.printf("   MQ135: %d (indice, mV)   (voltaje canal A1: %.3f V | voltaje real tras divisor: %.3f V)\n",
                mq135_indice, v_mq135_crudo, v_mq135_real);

#if USAR_FIREBASE
  publicarEnFirebase(tds_ppm, mq135_indice);
#endif

#if USAR_API
  publicarEnAPI(tds_ppm, mq135_indice);
#endif
}

// ================================================================
//  setup()
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n===================================");
  Serial.println(" HYDOR-CUALLI -- Nodo puente (Firebase + API propio)");
  Serial.println("===================================");

  // I2C: SDA=D2(GPIO4), SCL=D1(GPIO5)
  Wire.begin(4, 5);
  intentarIniciarADS();
  ultimoIntentoAds = millis();

  // Primer intento de WiFi: espera acotada (no infinita) para salir
  // del setup() ya conectado si se puede. Si no, el loop() se hace
  // cargo de seguir intentando sin bloquear nada mas.
  iniciarConexionWifi();
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
    delay(300);
    Serial.print(".");
    yield();
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Todavia sin conexion; se seguira intentando en el loop().");
  }
  ultimoIntentoWifi = millis();

#if USAR_FIREBASE
  horaSincronizada = sincronizarHora();
  fbConfig.host = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);
  Firebase.setString(fbData, "/sensor/estadoConexion", "conectado");
  Serial.println("[Firebase] configurado.");
#endif

  Serial.println("Listo. Esperando ciclo de publicacion...\n");
}

// ================================================================
//  loop()
// ================================================================
void loop() {
  gestionarWiFi();

  if (!adsListo && millis() - ultimoIntentoAds >= INTERVALO_REINTENTO_ADS) {
    ultimoIntentoAds = millis();
    intentarIniciarADS();
  }

  if (millis() - ultimaPublicacion >= INTERVALO) {
    ultimaPublicacion = millis();
    publicarLecturas();
  }

  yield();
  delay(200);
}

/* ================================================================
   GUIA RAPIDA
   ----------------------------------------------------------------
   LIBRERIAS A INSTALAR (Arduino IDE -> Herramientas -> Administrar
   Bibliotecas... busca el nombre EXACTO y da "Instalar"):

     - "Firebase ESP8266 Client" de Mobizt        (solo si USAR_FIREBASE)
     - "Adafruit ADS1X15"                          (siempre)
     - "Adafruit BusIO"                            (se instala junto con
                                                     la anterior si el IDE
                                                     te lo pide; instalala)

   PLACA A ELEGIR:
     Herramientas -> Placa -> "NodeMCU 1.0 (ESP-12E Module)"

   MONITOR SERIAL:
     Velocidad: 115200 baudios

   SI ALGO FALLA:

     1) "No se encontro el ADS1115" en el monitor serial
        -> Revisa que SDA vaya a D2, SCL a D1, y que ADDR este
           conectado a GND (no suelto). Revisa que VDD del ADS1115
           tenga 3V3 real (midelo con un multimetro si tienes).
           El nodo NO se queda trabado: reintenta solo cada minuto.

     2) El WiFi nunca conecta ("No se pudo conectar aun...")
        -> Revisa que el SSID y password esten bien escritos
           (mayusculas/minusculas importan). Si tu red es de 5GHz,
           el ESP8266 no la va a ver: necesita 2.4GHz.

     3) El HTTP al API da un codigo raro (401, 403, 500, o "-1")
        -> Codigo 401/403: la api_key esta mal copiada o el
           dispositivo no esta registrado en el dashboard.
        -> Codigo -1 o "connection refused": revisa api_host y
           api_port, y que el servidor este realmente prendido
           (pidele a quien administra el VPS que revise con
           "docker compose ps").

     4) Las lecturas de TDS o MQ-135 se ven todas en cero o clavadas
        -> Revisa la alimentacion de cada sensor (TDS a 3V3, MQ-135
           a Vin/5V) y que la señal SI llegue al canal correcto del
           ADS1115 (A0 = TDS, A1 = MQ-135). Revisa tambien que el
           divisor de voltaje del MQ-135 este armado con las dos
           resistencias de 10k -- si falta una resistencia, el
           voltaje que llega al ADS puede salir raro o en cero.
   ================================================================ */
