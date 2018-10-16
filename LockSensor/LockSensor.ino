/**
Developer: Abel Luque
E-mail: abel.luque@gmail.com

Sensor de barrera, con indicadores LED rojo y verde
A ser utilizado para indicar si la cerradura domiciliaria se encuentra cerrada con llave
*/

/*Inicializo el PIM para el sensor*/
const int sensorPin = 9;

/*Inicializo el PIM para el LED rojo*/
const int redLightPin = 11;

/*Inicializo el PIM para el LED verde*/
const int greenLightPin = 12;

void setup() {
  Serial.begin(9600);   //iniciar puerto serie
  pinMode(sensorPin , INPUT);  //definir pin como entrada
  pinMode(redLightPin, OUTPUT);//definir pin como salida
  pinMode(greenLightPin, OUTPUT);//definir pin como salida
}
 
void loop(){
  int value = 0;
  value = digitalRead(sensorPin );  //lectura digital de pin
 
  if (value == HIGH) {
      Serial.println("Abierto");
      digitalWrite(greenLightPin, LOW);
      digitalWrite(redLightPin, HIGH);
  }else{
      Serial.println("Cerrado");
      digitalWrite(redLightPin, LOW);
      digitalWrite(greenLightPin, HIGH);
  }
  //delay(500);
}

