# 🛠️ Guía de armado — Nodo ESP8266 con 2 sensores a la vez (agua + aire)

> Para quien lo va a construir. Explicado sencillo, sin dar por hecho que ya sabes.

---

## ¿Qué vamos a construir?

Un **nodo** que mide **calidad del agua (TDS)** y **calidad del aire (MQ-135)**
**al mismo tiempo**, y manda los datos por WiFi. El "cerebro" es un ESP8266.

**El problema y la solución (esto es lo importante):**
El ESP8266 tiene **una sola entrada analógica** (el pin A0), pero nosotros
tenemos **dos** sensores analógicos. No caben los dos en un solo pin.
👉 Por eso usamos un módulo **ADS1115**: es un chip que agrega **4 entradas
analógicas** y se conecta al ESP por solo **2 cables** (I2C). Con él, los dos
sensores se leen **a la vez**, cada uno en su propio canal.

---

## 🧾 Lista de componentes

| # | Componente | Cant. | ¿Para qué sirve? |
|---|-----------|:-----:|------------------|
| 1 | **NodeMCU ESP8266** | 1 | El cerebro. Lee los sensores y envía los datos por WiFi. |
| 2 | **Módulo ADS1115** | 1 | **La pieza clave.** Le da al ESP 4 canales analógicos por I2C, para leer los 2 sensores a la vez. |
| 3 | **Sensor TDS (SEN0244)** | 1 | Mide sólidos disueltos en el **agua** (ppm). |
| 4 | **Sensor MQ-135** | 1 | Mide gases / calidad del **aire**. |
| 5 | **Resistencias 10 kΩ 1/4 W** (de patitas) | 2 | Forman un **divisor de voltaje** que protege al ADS del MQ-135. |
| 6 | **Protoboard** | 1 | Para armar la prueba **sin soldar**. |
| 7 | **Cables Dupont** (macho-macho y macho-hembra) | ~15 | Las conexiones. |
| 8 | **Cable USB** | 1 | Alimentar y programar el ESP. |
| — | *(Opcional)* Base HW-389 + adaptador DC | 1 | Para **dejarlo instalado fijo 24/7** (fase de instalación, no de prueba). |

---

## 🔌 Cómo se conecta (paso a paso)

### 1) El ADS1115 al ESP8266 (los 2 cables de datos + poder)
| ADS1115 | va a → | ESP8266 |
|---------|:------:|---------|
| VDD | → | **3V3** |
| GND | → | **GND** |
| SCL | → | **D1** |
| SDA | → | **D2** |
| ADDR | → | **GND**  *(fija la dirección 0x48)* |

### 2) El sensor de AGUA (TDS)
| TDS | va a → | |
|-----|:------:|--|
| VCC | → | **3V3** |
| GND | → | **GND** |
| Señal (A) | → | **canal A0 del ADS** *(directo, es seguro)* |

### 3) El sensor de AIRE (MQ-135) — este lleva el divisor
| MQ-135 | va a → | |
|--------|:------:|--|
| VCC | → | **5V** (pin Vin del ESP / VUSB) |
| GND | → | **GND** |
| AO (señal) | → | **al divisor**, y la salida del divisor → **canal A1 del ADS** |

**El divisor (2 resistencias de 10 kΩ):**
```
MQ (AO) → [R1] → punto medio → [R2] → GND
                     |
                     └──→ A1 del ADS
```
Es como un tobogán: la señal del MQ entra "fuerte" (hasta 5V), y en el **punto
medio** ya vale **la mitad** (~2.5V), que es lo seguro para el ADS. Ese punto
medio es el que va a **A1**.

---

## ⚠️ 4 cosas que NO puedes saltarte

1. **El divisor es obligatorio.** El MQ-135 trabaja a 5V y su salida puede
   llegar casi a 5V. El ADS solo aguanta ~3.6V: **sin el divisor, lo quemas.**
   Las dos resistencias deben ser **iguales (10 kΩ)** para partir el voltaje justo a la mitad.
2. **El MQ-135 necesita 5V** (su calentador interno). A 3.3V no calienta bien y
   las lecturas no sirven. Toma los 5V del pin **Vin/VUSB**.
3. **Todas las tierras (GND) juntas.** ESP, ADS, TDS, MQ-135 y la R2 deben
   compartir la misma tierra. En la protoboard el riel – de **arriba** y el de
   **abajo** casi nunca están unidos: si usas los dos, **pon un cable entre ellos**.
4. **Quema-inicial del MQ-135 (burn-in):** déjalo **encendido 24-48 h seguidas**
   la primera vez, antes de confiar en sus lecturas. Es normal.

---

## 🧑‍💻 El código (qué hace)

El firmware lee **A0 = agua** y **A1 = aire** desde el ADS1115 (por I2C en D1/D2),
convierte el agua a ppm y aplica el ×2 del divisor al aire, y publica los datos.
Está en [`firmware/esp8266-ads1115-2sensores`](../../firmware/esp8266-ads1115-2sensores/) — **no cambia** si usas
protoboard o la base HW-389, porque los pines de I2C siguen siendo D1 y D2.

```cpp
Wire.begin(4, 5);              // SDA = D2 (GPIO4), SCL = D1 (GPIO5)
ads.begin(0x48);              // ADDR a GND
int16_t a0 = ads.readADC_SingleEnded(0);   // A0 = agua (TDS)
int16_t a1 = ads.readADC_SingleEnded(1);   // A1 = aire (MQ-135)
float vAire = ads.computeVolts(a1) * 2.0;  // ×2 deshace el divisor
```

---

## ✅ Orden sugerido para armarlo

1. Encaja/conecta el ESP y el ADS. Sube el código y confirma en el Monitor
   Serial que el ADS se detecta.
2. Conecta el **TDS** (A0) y prueba con agua. Debe dar lecturas.
3. Arma el **divisor** y conecta el **MQ-135** (A1). Deja el MQ en burn-in.
4. Cuando los dos leen bien, conéctalo al servidor y listo.

> 💡 Para **probar**: protoboard normal. Para **instalar fijo**: base HW-389
> (el divisor soldado o en un mini-protoboard).
