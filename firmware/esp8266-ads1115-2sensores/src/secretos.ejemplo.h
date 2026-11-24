// secretos.ejemplo.h — PLANTILLA con valores falsos.
// Copia este archivo como "secretos.h" (en esta misma carpeta src/)
// y cambia los valores por los tuyos. secretos.h NO se sube a git.
#pragma once

// WiFi (2.4 GHz). Si solo tienes una red, repite la misma en la 2.
#define WIFI_SSID1     "TU_WIFI"
#define WIFI_PASSWORD1 "TU_CLAVE"
#define WIFI_SSID2     "TU_WIFI"
#define WIFI_PASSWORD2 "TU_CLAVE"

// Firebase Realtime Database (solo se usa si USAR_FIREBASE es true)
#define FIREBASE_HOST  "tu-proyecto-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH  "TU_TOKEN"

// Servidor central propio (solo se usa si USAR_API es true)
#define SECRETO_API_HOST "IP_O_DOMINIO_DEL_SERVIDOR"
#define SECRETO_API_KEY  "TU_API_KEY"
