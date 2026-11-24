# Guías y diagramas para armar tu nodo

Aquí están las guías del **nodo de 2 sensores** (ESP8266 + ADS1115 + TDS + MQ-135).
El firmware que va con ellas es
[`firmware/esp8266-ads1115-2sensores`](../../firmware/esp8266-ads1115-2sensores/).

## Por dónde empezar

1. **Lee primero** [`GUIA_ARMADO_2SENSORES.md`](GUIA_ARMADO_2SENSORES.md): lista de piezas,
   conexiones paso a paso y las 3 cosas que no te puedes saltar. Se lee directo aquí en GitHub.
2. Luego abre los diagramas, en este orden:

| Archivo | Qué te enseña |
|---|---|
| [`DIAGRAMA_ESP8266_2SENSORES.html`](DIAGRAMA_ESP8266_2SENSORES.html) | El diagrama principal: cómo van los 2 sensores al ESP8266 en protoboard. |
| [`COMO_VAN_LAS_RESISTENCIAS.html`](COMO_VAN_LAS_RESISTENCIAS.html) | Explicación muy sencilla del divisor de 2 resistencias de 10 kΩ (el que protege al ADS1115). |
| [`PROTOBOARD_ARMADO.html`](PROTOBOARD_ARMADO.html) | Dónde va cada cosa en la protoboard, ya armada. |
| [`DIAGRAMA_BASE_NODEMCU.html`](DIAGRAMA_BASE_NODEMCU.html) | Versión para dejar el nodo **fijo** sobre la base HW-389 (fase de instalación). |

## Cómo ver los diagramas `.html`

GitHub muestra el código de los `.html`, no el dibujo. Para verlos:

- **Opción fácil:** descarga el proyecto (`Code → Download ZIP`), descomprímelo y dale
  **doble clic** al `.html`. Se abre en tu navegador. Funcionan sin internet.
- **Sin descargar nada:** pega la dirección del archivo después de
  `https://htmlpreview.github.io/?`, por ejemplo:
  `https://htmlpreview.github.io/?https://github.com/gmork69/hydor-cualli/blob/main/docs/armado/DIAGRAMA_ESP8266_2SENSORES.html`

> Los colores de cable de los diagramas son sugerencia. Lo que importa es **a qué pin**
> llega cada cable; verifícalo con el texto de la guía, no solo con el color.
