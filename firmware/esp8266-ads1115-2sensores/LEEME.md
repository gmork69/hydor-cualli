# Firmware 2 SENSORES A LA VEZ — ESP8266 + ADS1115

Lee **agua (TDS)** y **aire (MQ-135)** al mismo tiempo, sin jumper de modo.
El ESP8266 tiene una sola entrada analógica, así que se agrega un módulo
**ADS1115** (4 entradas analógicas por I2C).

> 🧪 **Estado: en prueba.** Este firmware está escrito y revisado, pero
> **todavía no se ha probado en hardware real**. Si lo armas, cuéntanos en un
> Issue qué te salió en el Monitor Serial. Si prefieres algo ya probado, usa
> [`esp8266-oficial-historial`](../esp8266-oficial-historial/).

## Qué necesitas
- NodeMCU **ESP8266** y módulo **ADS1115**
- Sensor **TDS SEN0244** y sensor **MQ-135**
- **2 resistencias de 10 kΩ** (divisor de voltaje, obligatorio)
- Protoboard, cables Dupont y cable USB **de datos** (los de solo carga no sirven)

## Cableado
Guía completa y diagramas: [`docs/armado/`](../../docs/armado/).

| De | A |
|---|---|
| ADS1115 VDD / GND | ESP 3V3 / GND |
| ADS1115 SCL / SDA | ESP D1 / D2 |
| ADS1115 ADDR | GND (dirección 0x48) |
| TDS señal | ADS canal **A0** (directo) |
| MQ-135 VCC | ESP **Vin** (5 V) |
| MQ-135 AO | **divisor 10k + 10k** → punto medio a ADS canal **A1** |

> ⚠️ **Sin el divisor quemas el ADS1115.** Antes de conectar el MQ-135, mide
> con multímetro: con 5 V a la entrada del divisor, el punto medio debe marcar
> ~2.5 V.

## Cómo cargarlo (VSCode + PlatformIO)
1. Abre **esta carpeta** en VSCode con PlatformIO.
2. En `src/`, **copia `secretos.ejemplo.h` como `secretos.h`** y pon ahí tu
   WiFi y los datos de tu Firebase. `secretos.h` **nunca se sube** a GitHub
   (está en `.gitignore`).
3. Conecta el ESP por USB → **Upload** (→) → **Monitor Serial** a 115200.
4. Debe aparecer `[OK] ADS1115 detectado.` y, cada 60 s, las lecturas de TDS
   y MQ-135 con su voltaje.

## Interruptores (arriba de `src/main.cpp`)
- `USAR_FIREBASE true` — publica a tu Firebase (lo de hoy), incluido el
  historial.
- `USAR_API false` — el servidor central todavía no existe. Cuando exista,
  se pone en `true` y se llenan `SECRETO_API_HOST` y `SECRETO_API_KEY` en
  `secretos.h`.

## Ojo con las lecturas
- **TDS:** usa la fórmula de DFRobot con temperatura fija de 25 °C. Falta
  calibrar con una solución patrón.
- **MQ-135:** manda un **índice en milivolts**, no ppm. Necesita 24-48 h
  encendido a 5 V (burn-in) y medir su R0 antes de dar ppm reales.
