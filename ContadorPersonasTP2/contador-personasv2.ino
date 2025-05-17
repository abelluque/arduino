// --- Pin Definitions ---
// Definición de pines para el Sensor 1 (Considerado el sensor de "entrada" o el primero en la secuencia de ingreso)
const int SENSOR1_TRIG_PIN = 2; // Pin TRIG del sensor 1, conectado al pin digital 2 del Arduino
const int SENSOR1_ECHO_PIN = 3; // Pin ECHO del sensor 1, conectado al pin digital 3 del Arduino

// Definición de pines para el Sensor 2 (Considerado el sensor de "salida" o el segundo en la secuencia de ingreso)
const int SENSOR2_TRIG_PIN = 4; // Pin TRIG del sensor 2, conectado al pin digital 4 del Arduino
const int SENSOR2_ECHO_PIN = 5; // Pin ECHO del sensor 2, conectado al pin digital 5 del Arduino

// --- Program Variables ---
// Variables para la lógica del contador de personas y estados de los sensores
int peopleCount = 0;          // Acumulador que guarda la cantidad actual de personas dentro del local.
long sensor1DetectionTime = 0; // Almacena el tiempo (en milisegundos desde el inicio del programa) en que el sensor 1 detectó un objeto por última vez.
long sensor2DetectionTime = 0; // Almacena el tiempo (en milisegundos) en que el sensor 2 detectó un objeto por última vez.

// Estados de los sensores:
// 0: Sensor libre o inactivo, no ha detectado nada recientemente en la secuencia actual.
// 1: Sensor ha detectado un objeto y está "activo" esperando la posible activación del otro sensor.
int sensor1State = 0;
int sensor2State = 0;

// --- Configuration Parameters ---
// Estos parámetros son cruciales para el correcto funcionamiento y deben ajustarse según el entorno de instalación.
const int DETECTION_THRESHOLD_DISTANCE = 50; // Distancia en centímetros. Si un objeto es detectado a una distancia MENOR que este valor,
                                             // se considera una detección válida (una persona pasando).
                                             // Ajustar según la altura de montaje de los sensores y el ancho del pasillo.

const long MAX_TIME_BETWEEN_DETECTIONS = 700; // Tiempo máximo en milisegundos que puede transcurrir entre la activación del primer sensor
                                              // y la activación del segundo para que se considere un paso válido de una persona.
                                              // Este valor depende de la distancia física entre los sensores (10cm en este caso)
                                              // y una velocidad de paso estimada. Por ejemplo, si una persona camina a 0.5 m/s,
                                              // tardaría 200ms en cubrir 10cm. El valor de 700ms da un margen considerable.

const long COOLDOWN_PERIOD = 1500;         // Tiempo de "enfriamiento" en milisegundos. Después de registrar un ingreso o egreso,
                                           // el sistema esperará este tiempo antes de poder registrar un nuevo evento.
                                           // Esto ayuda a prevenir múltiples conteos de la misma persona si se mueve lentamente
                                           // o si los sensores generan lecturas erráticas momentáneas.
long lastEventTimestamp = 0;               // Almacena el tiempo (en milisegundos) del último evento de conteo (ingreso/egreso) registrado.

void setup() {
  Serial.begin(9600); // Iniciar la comunicación serial a 9600 baudios para enviar mensajes de depuración al Monitor Serie.

  // Configurar los pines TRIG como SALIDA y los pines ECHO como ENTRADA para ambos sensores.
  pinMode(SENSOR1_TRIG_PIN, OUTPUT);
  pinMode(SENSOR1_ECHO_PIN, INPUT);
  pinMode(SENSOR2_TRIG_PIN, OUTPUT);
  pinMode(SENSOR2_ECHO_PIN, INPUT);

  // Mensajes informativos al iniciar el sistema, visibles en el Monitor Serie.
  Serial.println("--- Sistema Contador de Personas Iniciado ---");
  Serial.print("Umbral de distancia para deteccion: ");
  Serial.print(DETECTION_THRESHOLD_DISTANCE);
  Serial.println(" cm");
  Serial.print("Tiempo maximo entre detecciones de sensores: ");
  Serial.print(MAX_TIME_BETWEEN_DETECTIONS);
  Serial.println(" ms");
  Serial.print("Periodo de enfriamiento post-evento: ");
  Serial.print(COOLDOWN_PERIOD);
  Serial.println(" ms");
  Serial.println("-------------------------------------------");
  Serial.print("Conteo inicial de personas: ");
  Serial.println(peopleCount); // Mostrar el conteo inicial (debería ser 0).
}

void loop() {
  // --- Lógica de Enfriamiento ---
  // Comprobar si el sistema está actualmente en un período de enfriamiento después del último evento.
  if (millis() - lastEventTimestamp < COOLDOWN_PERIOD) {
    // Si estamos en enfriamiento, es una buena práctica asegurarse de que los estados
    // de los sensores se reinicien para evitar que una detección "pegada" de la secuencia anterior
    // interfiera justo cuando termina el enfriamiento.
    if (sensor1State == 1 || sensor2State == 1) { // Si alguno de los sensores quedó activo de la secuencia anterior
        sensor1State = 0; // Resetear estado del sensor 1
        sensor2State = 0; // Resetear estado del sensor 2
        sensor1DetectionTime = 0; // Resetear tiempo de detección del sensor 1
        sensor2DetectionTime = 0; // Resetear tiempo de detección del sensor 2
    }
    return; // Salir de la función loop() temporalmente si estamos en enfriamiento, no procesar nuevas lecturas.
  }

  // Leer la distancia actual de ambos sensores.
  long distanceS1 = getDistance(SENSOR1_TRIG_PIN, SENSOR1_ECHO_PIN);
  long distanceS2 = getDistance(SENSOR2_TRIG_PIN, SENSOR2_ECHO_PIN);

  // --- Lógica de detección del Sensor 1 ---
  // Comprobar si se detecta un objeto dentro del umbral de distancia del sensor 1.
  // También se verifica que distanceS1 > 0 para ignorar posibles lecturas erróneas de 0cm.
  if (distanceS1 < DETECTION_THRESHOLD_DISTANCE && distanceS1 > 0) {
    if (sensor1State == 0) { // Si es la primera vez que el sensor 1 detecta algo en esta nueva secuencia potencial.
      sensor1State = 1;      // Marcar el sensor 1 como activo (estado 1).
      sensor1DetectionTime = millis(); // Registrar el momento exacto de esta detección.
      Serial.println("Evento: Sensor 1 ACTIVADO");
    }
  } else { // No hay objeto en el sensor 1 o está fuera del umbral de detección.
    // Lógica de timeout para el sensor 1:
    // Si el sensor 1 estaba activo (sensor1State == 1), pero ya ha pasado más tiempo que MAX_TIME_BETWEEN_DETECTIONS
    // desde su activación, y el sensor 2 NUNCA se activó (sensor2State == 0),
    // significa que la persona solo pasó por el sensor 1 y no completó la secuencia, o fue una falsa alarma.
    // En este caso, se resetea el estado del sensor 1 para permitir una nueva secuencia.
    if (sensor1State == 1 && (millis() - sensor1DetectionTime > MAX_TIME_BETWEEN_DETECTIONS) && sensor2State == 0) {
      sensor1State = 0; // Resetear estado.
      sensor1DetectionTime = 0; // Resetear tiempo.
      // Serial.println("Debug: Sensor 1 reseteado por timeout (Sensor 2 no se activo).");
    }
  }

  // --- Lógica de detección del Sensor 2 ---
  // Similar a la lógica del sensor 1.
  if (distanceS2 < DETECTION_THRESHOLD_DISTANCE && distanceS2 > 0) {
    if (sensor2State == 0) {
      sensor2State = 1;
      sensor2DetectionTime = millis();
      Serial.println("Evento: Sensor 2 ACTIVADO");
    }
  } else {
    // Lógica de timeout para el sensor 2 (análoga a la del sensor 1).
    if (sensor2State == 1 && (millis() - sensor2DetectionTime > MAX_TIME_BETWEEN_DETECTIONS) && sensor1State == 0) {
      sensor2State = 0;
      sensor2DetectionTime = 0;
      // Serial.println("Debug: Sensor 2 reseteado por timeout (Sensor 1 no se activo).");
    }
  }

  // --- Lógica de Conteo (Ingreso y Egreso) ---
  // Comprobar si AMBOS sensores han sido activados en la secuencia actual (sus estados son 1)
  // y sus tiempos de detección son válidos (mayores que 0, indicando que realmente se registraron).
  if (sensor1State == 1 && sensor2State == 1 && sensor1DetectionTime > 0 && sensor2DetectionTime > 0) {
    // Caso de INGRESO: Sensor 1 se activó ANTES que Sensor 2.
    if (sensor1DetectionTime < sensor2DetectionTime) {
      // Verificar que el tiempo transcurrido entre la activación de S1 y S2 sea menor que el máximo permitido.
      if ((sensor2DetectionTime - sensor1DetectionTime) < MAX_TIME_BETWEEN_DETECTIONS) {
        peopleCount++; // Incrementar el contador de personas.
        Serial.print("Evento: INGRESO DETECTADO. Personas dentro: ");
        Serial.println(peopleCount);
        lastEventTimestamp = millis(); // Registrar el tiempo de este evento para iniciar el período de enfriamiento.
        resetSensorStates();           // Resetear los estados de los sensores para prepararse para el próximo evento.
      } else {
        // Si el tiempo entre detecciones es demasiado largo, se considera un evento inválido o dos eventos separados no concluyentes.
        // Serial.println("Debug: Intento de INGRESO - Tiempo entre S1 y S2 excedido.");
        resetSensorStates(); // Resetear igualmente para evitar falsos conteos y permitir una nueva secuencia.
      }
    }
    // Caso de EGRESO: Sensor 2 se activó ANTES que Sensor 1.
    else if (sensor2DetectionTime < sensor1DetectionTime) {
      // Verificar que el tiempo transcurrido entre la activación de S2 y S1 sea menor que el máximo permitido.
      if ((sensor1DetectionTime - sensor2DetectionTime) < MAX_TIME_BETWEEN_DETECTIONS) {
        if (peopleCount > 0) { // Solo decrementar si el contador es mayor que cero. Nunca debe ser negativo.
          peopleCount--;
        }
        Serial.print("Evento: EGRESO DETECTADO. Personas dentro: ");
        Serial.println(peopleCount);
        lastEventTimestamp = millis(); // Iniciar período de enfriamiento.
        resetSensorStates();           // Resetear estados.
      } else {
        // Serial.println("Debug: Intento de EGRESO - Tiempo entre S2 y S1 excedido.");
        resetSensorStates();
      }
    }
    // Nota: Si sensor1DetectionTime == sensor2DetectionTime, es una condición muy improbable y se ignoraría aquí.
    // Podría considerarse un error o una detección simultánea no concluyente.
  }

  // --- Lógica de Timeout Individual Adicional ---
  // Esta sección maneja el caso donde una persona se detiene solo frente a un sensor por un tiempo prolongado.
  // Si un sensor permanece activo por un tiempo significativamente mayor que MAX_TIME_BETWEEN_DETECTIONS
  // y el otro sensor nunca se activó, se asume que la secuencia no se completará y se resetea el sensor activo.
  // Se usa un margen un poco mayor que MAX_TIME_BETWEEN_DETECTIONS (ej. +500ms) para esta lógica de timeout individual.
   if (sensor1State == 1 && sensor2State == 0 && (millis() - sensor1DetectionTime > MAX_TIME_BETWEEN_DETECTIONS + 500)){
      Serial.println("Advertencia: Timeout extendido para Sensor 1 solo. Reseteando S1.");
      sensor1State = 0;
      sensor1DetectionTime = 0;
   }
   if (sensor2State == 1 && sensor1State == 0 && (millis() - sensor2DetectionTime > MAX_TIME_BETWEEN_DETECTIONS + 500)){
      Serial.println("Advertencia: Timeout extendido para Sensor 2 solo. Reseteando S2.");
      sensor2State = 0;
      sensor2DetectionTime = 0;
   }

  // Pequeña pausa al final del loop para dar estabilidad a las lecturas y no sobrecargar los sensores o el procesador.
  // También ayuda a que el Monitor Serie sea más legible.
  delay(50); // 50 milisegundos de pausa.
}

// --- Funciones Auxiliares ---

/**
 * @brief Mide la distancia utilizando un sensor ultrasónico HC-SR04.
 * @param triggerPin El pin TRIG del sensor que se va a utilizar.
 * @param echoPin El pin ECHO del sensor que se va a utilizar.
 * @return La distancia medida en centímetros. Retorna un valor grande (ej. 9999) si hay un error o si pulseIn() hace timeout.
 */
long getDistance(int triggerPin, int echoPin) {
  // Paso 1: Asegurar que el pin Trig esté en estado bajo (LOW) por un corto período para limpiar cualquier señal previa.
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2); // Esperar 2 microsegundos.

  // Paso 2: Enviar un pulso ultrasónico. Poner el pin Trig en estado alto (HIGH) por 10 microsegundos.
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10); // Mantener en alto por 10 microsegundos.
  digitalWrite(triggerPin, LOW); // Volver el pin Trig a estado bajo.

  // Paso 3: Medir la duración del pulso de eco.
  // La función pulseIn() espera a que el pin especificado (echoPin) pase a HIGH,
  // comienza a contar el tiempo, y luego espera a que el pin vuelva a LOW.
  // Devuelve la duración del pulso en microsegundos.
  // Se añade un timeout (30000 µs) a pulseIn() para evitar que el programa se bloquee indefinidamente
  // si no se recibe un eco (ej. objeto demasiado lejos o absorbente de sonido).
  // 30000 µs corresponde a una distancia máxima teórica de aproximadamente 5 metros.
  long duration = pulseIn(echoPin, HIGH, 30000);

  // Paso 4: Calcular la distancia.
  // La velocidad del sonido es aproximadamente 343 metros por segundo, lo que equivale a 0.0343 centímetros por microsegundo.
  // La distancia al objeto es (tiempo_del_eco * velocidad_del_sonido) / 2.
  // Se divide por 2 porque el sonido viaja desde el sensor al objeto y luego de regreso (ida y vuelta).
  // Distancia (cm) = (duration (µs) * 0.0343 cm/µs) / 2
  // Esto se simplifica a: Distancia (cm) = duration / 58.309...
  // Usamos 58 para una aproximación entera.
  if (duration == 0) { // Si pulseIn() hizo timeout (no recibió eco o el pulso fue demasiado corto/largo).
      return 9999;     // Retornar un valor grande para indicar que está fuera de rango o hubo un error.
  }
  return duration / 58; // Convertir la duración del pulso de eco a distancia en centímetros.
}

/**
 * Resetea los estados y los tiempos de detección de ambos sensores.
 * Esta función se llama típicamente después de que un evento de conteo (ingreso/egreso) ha sido procesado exitosamente,
 * o cuando una secuencia de detección se considera inválida (ej. por timeout),
 * para preparar el sistema para la próxima detección potencial.
 */
void resetSensorStates() {
  sensor1State = 0; // Poner el estado del sensor 1 en inactivo.
  sensor2State = 0; // Poner el estado del sensor 2 en inactivo.
  sensor1DetectionTime = 0; // Resetear el tiempo de la última detección del sensor 1.
  sensor2DetectionTime = 0; // Resetear el tiempo de la última detección del sensor 2.
  Serial.println("Info: Estados de los sensores reseteados para el proximo evento.");
}
