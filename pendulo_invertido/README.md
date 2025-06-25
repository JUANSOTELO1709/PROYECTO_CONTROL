
# 📌 Control LQR de Péndulo Invertido sobre Carro

Este proyecto implementa un **controlador LQR** (Linear Quadratic Regulator) para estabilizar un péndulo invertido montado sobre un carro controlado mediante un motor paso a paso. El sistema lee el ángulo del péndulo mediante un encoder y controla la posición del carro para mantener el péndulo en equilibrio.

---

## ⚙️ Características

- ✅ Control LQR en tiempo real
- 📡 Ajuste de parámetros desde el monitor serial
- 📈 Envío de datos en tiempo real para graficar
- 🏁 Homing automático del carro al iniciar
- 📏 Lectura de ángulo del péndulo con encoder incremental

---

## 🔧 Hardware requerido

| Componente                  | Cantidad |
|----------------------------|----------|
| Motor paso a paso + Driver | 1        |
| Encoder incremental        | 1        |
| Pulsador de homing         | 1        |
| Arduino UNO/Nano           | 1        |
| Fuente de alimentación     | 1        |

---

## 🔌 Conexiones

| Arduino | Componente             |
|---------|------------------------|
| D6      | stepPinCarro           |
| D7      | dirPinCarro            |
| D4      | homeSwitchPin          |
| D2      | Encoder A (interrupt)  |
| D3      | Encoder B              |

---


## 🖥️ Comando Serial

Puedes enviar los siguientes comandos desde el **monitor serial** a 115200 baudios para ajustar parámetros en tiempo real:

| Comando     | Descripción                                      | Ejemplo       |
|-------------|--------------------------------------------------|---------------|
| `v=XXXX`    | Cambia la velocidad máxima del carro             | `v=48000`     |
| `sp=XX`     | Cambia el ángulo deseado del péndulo (°)         | `sp=180`      |
| `spp=XXXX`  | Cambia la posición deseada del carro             | `spp=7500`    |
| `k_aXX`     | Cambia la ganancia del ángulo                    | `k_a19.34`    |
| `k_vaXX`    | Cambia la ganancia de la velocidad angular       | `k_va1.39`    |
| `k_pcXX`    | Cambia la ganancia del error de posición del carro | `k_pc0.39`  |
| `k_vcXX`    | Cambia la ganancia de la velocidad del carro     | `k_vc0.12`    |

---

## 📤 Datos enviados al serial

Cada segundo se imprime en el serial (separado por comas):

```
ángulo, setpointÁngulo, salidaLQR, posiciónCarro, errorPosición, velocidadAngular, velocidadCarro
```

Puedes usar Python, MATLAB u otro software para graficar estos datos en tiempo real.

---

## 🚀 Cómo usar

1. Carga el código en el Arduino.
2. Abre el monitor serial a 115200 baudios.
3. El carro se moverá al **punto de homing**.
4. Ajusta los valores de control usando los comandos anteriores.
5. Observa los datos en tiempo real o gráficalos.

---
