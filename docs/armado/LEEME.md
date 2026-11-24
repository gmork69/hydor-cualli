# Guías y diagramas para armar tu nodo

Hay **dos diseños** de nodo. Elige según cuántos sensores quieras leer a la vez.

| Diseño | Placas | Sensores | Firmware | Estado |
|---|---|---|---|---|
| **Original (1 sensor)** — diseño de **Emily Acosta** | ESP8266 | TDS **o** MQ-135, uno a la vez (A0 + jumper D7) | [`esp8266-oficial-historial`](../../firmware/esp8266-oficial-historial/) | ✅ Probado en hardware. ⚠️ Histórico: no sirve para más sensores |
| **2 sensores a la vez** ⭐ recomendado | ESP8266 **+ ADS1115** | TDS **y** MQ-135 a la vez | [`esp8266-ads1115-2sensores`](../../firmware/esp8266-ads1115-2sensores/) | 🧪 Escrito y revisado; **falta probarlo en hardware** |

## Diseño original (1 sensor) — histórico

Carpeta [`original-1sensor/`](original-1sensor/) (el diagrama es una imagen y se ve directo
en GitHub). Es el montaje con el que arrancó el proyecto y **funciona**, pero el ESP8266
tiene **una sola entrada analógica (A0)**: solo puedes medir un sensor a la vez y cambiar
moviendo cables. **No lo uses como base para agregar sensores**; para eso está el de 2 sensores.

## Diseño de 2 sensores (ESP8266 + ADS1115) — recomendado

El **ADS1115** le agrega al ESP8266 4 entradas analógicas por I2C (**SCL→D1, SDA→D2,
ADDR→GND = 0x48**). TDS va al canal **A0** del ADS; el MQ-135 pasa por un **divisor de
2 resistencias de 10 kΩ** y entra al canal **A1**.

1. **Lee primero** [`GUIA_ARMADO_2SENSORES.md`](GUIA_ARMADO_2SENSORES.md): lista de piezas,
   conexiones paso a paso y las 4 cosas que no te puedes saltar. Se lee directo aquí en GitHub.
2. Luego abre los diagramas, en este orden:

| Archivo | Qué te enseña |
|---|---|
| [`DIAGRAMA_ESP8266_2SENSORES.html`](DIAGRAMA_ESP8266_2SENSORES.html) | El diagrama principal: cómo van los 2 sensores al ESP8266 en protoboard, con zoom del divisor. |
| [`COMO_VAN_LAS_RESISTENCIAS.html`](COMO_VAN_LAS_RESISTENCIAS.html) | Explicación muy sencilla del divisor de 2 resistencias de 10 kΩ (el que protege al ADS1115). |
| [`PROTOBOARD_ARMADO.html`](PROTOBOARD_ARMADO.html) | Dónde va cada cosa en la protoboard, ya armada. |
| [`DIAGRAMA_BASE_NODEMCU.html`](DIAGRAMA_BASE_NODEMCU.html) | Versión para dejar el nodo **fijo** sobre la base HW-389 (fase de instalación; sin probar). |

> ⚠️ **Tierra común.** En la protoboard, el riel – (GND) de arriba y el de abajo casi
> nunca están unidos. Los diagramas ya muestran **un cable negro que los une**: no te lo
> saltes, o el MQ-135 no compartirá tierra con el ADS1115.

## Cómo ver los diagramas `.html`

GitHub muestra el código de los `.html`, no el dibujo. Para verlos:

- **Opción fácil:** descarga el proyecto (`Code → Download ZIP`), descomprímelo y dale
  **doble clic** al `.html`. Se abre en tu navegador. Funcionan sin internet.
- **Sin descargar nada:** pega la dirección del archivo después de
  `https://htmlpreview.github.io/?`, por ejemplo:
  `https://htmlpreview.github.io/?https://github.com/gmork69/hydor-cualli/blob/main/docs/armado/DIAGRAMA_ESP8266_2SENSORES.html`

> Los colores de cable de los diagramas son sugerencia. Lo que importa es **a qué pin**
> llega cada cable; verifícalo con el texto de la guía, no solo con el color.
