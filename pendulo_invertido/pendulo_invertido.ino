// --- Pines ---
const int stepPinCarro = 6;
const int dirPinCarro = 7;
const int homeSwitchPin = 4;
const int pinEncoderA = 2;
const int pinEncoderB = 3;

// --- Variables encoder pêndulo ---
volatile long contadorPulsosPendulo = 0;
volatile long contadorPulsosPenduloAnterior = 0;
// Para calcular velocidad angular
const int PPR_PENDULO = 600;

// --- Variables motor paso a paso carro ---
const int PPR_MOTOR = 1600;
long posicionCarroAnterior = 0; // Para calcular velocidad del carro
const long MAX_PASOS = 15000;

// --- Parámetros LQR (ganancias K) ---
float K_angulo = 19.3496; // Ganancia para el error de ángulo
float K_velocidad_angulo = 1.3912; // Ganancia para velocidad angular
float K_posicion_carro = 0.3957; // Ganancia para el error de posición del carro
float K_velocidad_carro = 0.1265; // Ganancia para velocidad del carro

// --- Setpoint y rango de control ---
float setpointAngulo = 180.0; // Ángulo deseado del péndulo (vertical hacia arriba)
float setpointPosicionCarro = MAX_PASOS / 2.0; // Posición central deseada para el carro
float margenAngulo = 40.0; // Margen angular para activar el control

// --- Tiempo de control ---
unsigned long tiempoAnterior = 0;
float dt = 0.005; // 5 ms

// --- Velocidad máxima y rampa ---
int velocidadMaximaControl = 48000; // Valor de salida máximo para el control del carro
long posicionCarro = 0;

// --- Setup ---
void setup() {
  Serial.begin(115200);

  pinMode(stepPinCarro, OUTPUT);
  pinMode(dirPinCarro, OUTPUT);
  pinMode(homeSwitchPin, INPUT_PULLUP);
  pinMode(pinEncoderA, INPUT_PULLUP);
  pinMode(pinEncoderB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinEncoderA), leerEncoderPendulo, RISING);

  hacerHoming();
  delay(1000);

  Serial.println("Control_LQR_iniciado.");
  Serial.println("Comandos: _vXX, _spXX, _aXX, _vaXX, _k_aXX, _k_vaXX, _k_pcXX, _k_vcXX");
}


void loop() {
  leerComandosSerial();

  unsigned long ahora = millis();
  if (ahora - tiempoAnterior >= 1000) {
    // Calcular el dt real para mayor precisión en velocidades
    float real_dt = (ahora - tiempoAnterior) / 1000.0;
    tiempoAnterior = ahora;

    // --- Lectura de estados ---
    float angulo = leerAnguloPendulo();
    float anguloInvertido = fmod((360.0 - angulo), 360.0);

    // Calcular velocidad angular del péndulo
    float velocidadAngular = ((contadorPulsosPendulo - contadorPulsosPenduloAnterior) * (360.0 / PPR_PENDULO)) / real_dt;
    contadorPulsosPenduloAnterior = contadorPulsosPendulo;

    // Calcular velocidad del carro (en pasos/segundo)
    float velocidadCarro = (posicionCarro - posicionCarroAnterior) / real_dt;
    posicionCarroAnterior = posicionCarro;

    // --- Cálculo de errores de estado ---
    float errorAngulo = setpointAngulo - anguloInvertido;
    float errorPosicionCarro = setpointPosicionCarro - posicionCarro;

    // --- Control LQR ---
    float salidaLQR = 0;
    bool dentroRango = abs(anguloInvertido - 180.0) < margenAngulo;

    if (dentroRango) {
      // La salida de control LQR es la suma ponderada de los estados
      salidaLQR = K_angulo * errorAngulo +
                  K_velocidad_angulo * velocidadAngular +
                  K_posicion_carro * errorPosicionCarro +
                  K_velocidad_carro * velocidadCarro;
    } else {
      // Si el péndulo se cae, la salida de control es 0 (o puede ir a un control de búsqueda)
      salidaLQR = 0;
    }

    // Mover el carro con la salida del LQR
    moverCarroControl(salidaLQR);

    // --- Enviar datos al serial ---
    Serial.print(anguloInvertido); Serial.print(",");
    Serial.print(setpointAngulo); Serial.print(",");
    Serial.print(salidaLQR); Serial.print(",");
    Serial.print(posicionCarro); Serial.print(",");
    Serial.print(errorPosicionCarro); Serial.print(",");
    Serial.print(velocidadAngular); Serial.print(",");
    Serial.println(velocidadCarro);
  }
}


// --- Función para interrupción del encoder del péndulo ---
void leerEncoderPendulo() {
  int b = digitalRead(pinEncoderB);
  if (b == LOW)
    contadorPulsosPendulo++;
  else
    contadorPulsosPendulo--;
}

// --- Calcular ángulo del péndulo ---
float leerAnguloPendulo() {
  long pulsos = contadorPulsosPendulo;
  float grados = (360.0 * pulsos) / PPR_PENDULO;
  grados = fmod(grados, 360.0);
  if (grados < 0) grados += 360;
  return grados;
}

// --- Homing del carro ---
void hacerHoming() {
  Serial.println("Haciendo_homing...");
  digitalWrite(dirPinCarro, LOW);
  while (digitalRead(homeSwitchPin) == HIGH) {
    digitalWrite(stepPinCarro, HIGH);
    delayMicroseconds(1000);
    digitalWrite(stepPinCarro, LOW);
    delayMicroseconds(1000);
  }
  posicionCarro = 0;
  contadorPulsosPendulo = 0;
  Serial.println("Homing_completado... Posicion_0.");
}

// --- Mover el carro según salida de control ---
void moverCarroControl(float salida) {
  int pasos = constrain((int)salida, -velocidadMaximaControl, velocidadMaximaControl);
  if (pasos == 0) return;

  bool direccion = pasos > 0;
  digitalWrite(dirPinCarro, direccion ? HIGH : LOW);

  // Simular “velocidad” como cantidad de pasos a mover
  for (int i = 0; i < abs(pasos); i++) {
    if ((direccion && posicionCarro < MAX_PASOS) || (!direccion && posicionCarro > 0)) {
      digitalWrite(stepPinCarro, HIGH);
      delayMicroseconds(110);
      digitalWrite(stepPinCarro, LOW);
      delayMicroseconds(110);
      posicionCarro += direccion ? 1 : -1;
    }
  }
}

void leerComandosSerial() {
  static String inputString = "";
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      inputString.trim();

      // Cambiar velocidad máxima
      if (inputString.startsWith("v=")) {
        int val = inputString.substring(2).toInt();
        if (val >= 0 && val <= 100000) {
          velocidadMaximaControl = val;
          Serial.print("Velocidad_maxima_de_control=");
          Serial.println(velocidadMaximaControl);
        } else {
          Serial.println("Valor_de_velocidad_fuera_de_rango_[0-100000]");
        }

      // Cambiar setpoint de ángulo
      } else if (inputString.startsWith("sp=")) {
        float nuevoSetpointAngulo = inputString.substring(3).toFloat();
        if (nuevoSetpointAngulo >= 0 && nuevoSetpointAngulo <= 360) {
          setpointAngulo = nuevoSetpointAngulo;
          Serial.print("Setpoint_de_angulo_actualizado_A_");
          Serial.println(setpointAngulo);
        } else {
          Serial.println("Setpoint_de_angulo_fuera_de_rango_[0-360]");
        }

      // Cambiar setpoint de posición del carro
      } else if (inputString.startsWith("spp=")) {
        float nuevoSetpointPosicion = inputString.substring(4).toFloat();
        if (nuevoSetpointPosicion >= 0 && nuevoSetpointPosicion <= MAX_PASOS) {
          setpointPosicionCarro = nuevoSetpointPosicion;
          Serial.print("Setpoint_de_posicion_del_carro_actualizado_A_");
          Serial.println(setpointPosicionCarro);
        } else {
          Serial.print("Setpoint_de_posicion_del_carro_fuera_de_rango_[0-");
          Serial.print(MAX_PASOS);
          Serial.println("]");
        }

      // Cambiar ganancias K individualmente
      } else if (inputString.startsWith("k_a")) {
        K_angulo = inputString.substring(4).toFloat();
        Serial.print("K_angulo=");
        Serial.println(K_angulo);
      } else if (inputString.startsWith("k_va")) {
        K_velocidad_angulo = inputString.substring(5).toFloat();
        Serial.print("K_velocidad_angulo=");
        Serial.println(K_velocidad_angulo);
      } else if (inputString.startsWith("k_pc")) {
        K_posicion_carro = inputString.substring(5).toFloat();
        Serial.print("K_posicion_carro=");
        Serial.println(K_posicion_carro);
      } else if (inputString.startsWith("k_vc")) {
        K_velocidad_carro = inputString.substring(5).toFloat();
        Serial.print("K_velocidad_carro=");
        Serial.println(K_velocidad_carro);
      }

      // Comando inválido o consulta
      else {
        Serial.println("Comando_no_reconocido._Usa_v=XX, sp=XX, spp=XX, k_aXX, k_vaXX, k_pcXX, k_vcXX");
      }

      inputString = "";
    } else {
      inputString += inChar;
    }
  }
}
