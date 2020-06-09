/*
 * PROYECTO PIP
 - COMPONENTES
    + Sensado de Temperatura
    + Sensado de Humedad
    + Sensado de Ph
    + SD card read/write

  This project .ino shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
    Using the first SPI port (SPI_1)
      SS    <-->  PA4 <-->  BOARD_SPI1_NSS_PIN
      SCK   <-->  PA5 <-->  BOARD_SPI1_SCK_PIN
      MISO  <-->  PA6 <-->  BOARD_SPI1_MISO_PIN
      MOSI  <-->  PA7 <-->  BOARD_SPI1_MOSI_PIN

    Using the second SPI port (SPI_2)
      SS    <-->  PB12 <-->  BOARD_SPI2_NSS_PIN
      SCK   <-->  PB13 <-->  BOARD_SPI2_SCK_PIN
      MISO  <-->  PB14 <-->  BOARD_SPI2_MISO_PIN
      MOSI  <-->  PB15 <-->  BOARD_SPI2_MOSI_PIN
*/
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>
#include <libmaple/usart.h>

File myFile;
#define RX PA10 // 
#define TX PA9 //

#define BOARD_LED PB12 //this is for STM32 BLACK PILL
const int SPI1_NSS_PIN = PA4;    //SPI_1 Chip Select pin is PA4. You can change it to the STM32 pin you want.
const int SPI2_NSS_PIN = PB12;   //SPI_2 Chip Select pin is PB12. You can change it to the STM32 pin you want.
//  Channels to be acquired. //
// TERRENO 1
const int S1_TEM_PIN = PA13;
const int S1_HUM_PIN = PA12;
const int S1_PH_PIN = PA11;
// TERRENO 2
const int S2_TEM_PIN = PA10;
const int S2_HUM_PIN = PA9;
const int S2_PH_PIN = PA8;
int inputUser;
int sensorValue = 0; // Variable to store the value coming from the sensor

unsigned long time;
int everyHour;
int dy;
int mth;
int yr;
int hr;
int mt;
int snd;

// declaración de variables de medicion
float voltaje;
float humedad1;
float temperatura1;
float ph1;
float humedad2;
float temperatura2;
float ph2;
float tr;  
float y;  
float temperaturaAlta;
float temperaturaBaja;
float humedadAlta;
float humedadBaja;
float phReferencia;

bool enviar; 

void setup(){
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  ///////////////////////////////////
  // PINES USART
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);
  
  //Start up blink from Pig-O-Scope
  pinMode(BOARD_LED, OUTPUT);
  pinMode(S1_TEM_PIN, INPUT_ANALOG);
  pinMode(S1_HUM_PIN, INPUT_ANALOG);
  pinMode(S1_PH_PIN, INPUT_ANALOG);
  pinMode(S2_TEM_PIN, INPUT_ANALOG);
  pinMode(S2_HUM_PIN, INPUT_ANALOG);
  pinMode(S2_PH_PIN, INPUT_ANALOG);
  //////////////////////////////////
  /////////////// VARIABLES LIMITES ///////////////
  temperaturaAlta = 30.0 ;
  temperaturaBaja = 20.0 ;
  humedadAlta = 80.0 ;
  humedadBaja = 60.0  ;
  phReferencia = 7.0 ;
  /////////////////////////////////////////////////

  time = millis();
  everyHour = 3600000;
  digitalWrite(BOARD_LED, HIGH);
  delay(1000);
  digitalWrite(BOARD_LED, LOW);
  delay(1000);

  initialization();
  setTime(hr,mt,snd,dy,mth,yr);
}


void loop() {
  // put your main code here, to run repeatedly:
  time_t t = now();
  // SENSANDO TERRENO 1
  Serial.println("SENSANDO TERRENO 1");
  sensar(S1_TEM_PIN);
  sensar(S1_HUM_PIN);
  sensar(S1_PH_PIN);
  // SENSANDO TERRENO 2
  Serial.println("SENSANDO TERRENO 2");
  sensar(S2_TEM_PIN);
  sensar(S2_HUM_PIN);
  sensar(S2_PH_PIN);
  // COMPARACIONES PARA VERIFICAR SI LAS VARIABLES ESTAN EN EL RANGO ESTABLECIDO
  comparaciones();
  // SE MUESTRA AL USUARIO LOS VALORES SENSADOS
  printSensed();
  // Store
  if (time >= everyHour){ //1 hora = 3'600.000 milisegundos
      saveSD(SPI1_NSS_PIN, t); // Guardar en una memoria, se puede añadir otra memoria ya que el mocro tiene más pines para comunicacion SPI SPI
      everyHour = everyHour + 3600000;
    }
  enviar=digitalRead(RX);
  if (enviar == true){ //1 hora = 3'600.000 milisegundos
    analogWrite(TX, temperatura1);
    analogWrite(TX, humedad1);
    analogWrite(TX, ph1);
    analogWrite(TX, temperatura2);
    analogWrite(TX, humedad2);
    analogWrite(TX, ph2);
  }
  delay(1000); // 1 segundo más para el tiempo t // PD: se puede calcular con el time tambien
}

void initialization(){
  ///////////////////////////////////
  //      RECIBIR FECHA Y HORA     //
  ///////////////////////////////////
  Serial.println("BIENVENIDO AL SISTEMA");
  Serial.println("Para inciar el proceso se debe ingresar por teclado la fecha y hora");
  Serial.println("FECHA...(INGRESAR NUMEROS)");
  Serial.println("Ejemplo: 6/6/2020 para 6 de junio del 2020");
  Serial.println("DÍA:");
  if (Serial.available()>0){
    dy=Serial.read();
  }
  Serial.println("MES:");
  if (Serial.available()>0){
    mth=Serial.read();
  }
  Serial.println("AÑO:");
  if (Serial.available()>0){
    yr=Serial.read();
  }
  Serial.println("HORA...(INGRESAR NUMEROS - Hora militar)");
  Serial.println("Ejemplo: 12:23:55 para 12 del medio día con 23 minutos y 55 segundos");
  Serial.println("HORA:");
  if (Serial.available()>0){
    hr=Serial.read();
  }
  Serial.println("MINUTOS:");
  if (Serial.available()>0){
    mt=Serial.read();
  }
  Serial.println("SEGUNDOS:");
  if (Serial.available()>0){
    snd=Serial.read();
  }
}

void sensar(int sensorPin){
  // Read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  voltaje=5.0*sensorValue/1023.0;
  // SI ESTOY SENSADNO TEMPERATURA
  if ((sensorPin == S1_TEM_PIN) || (sensorPin == S2_TEM_PIN)){
    tr = voltaje * 10000.0 / (5.0 - voltaje);   
    y = log(tr/10000.0);
    y = (1.0/298.15) + (y *(1.0/4050.0));
    // QUE SECCION SE ESTÁ SENSANDO
    if (sensorPin == S1_TEM_PIN){ //TERRENO 1
      temperatura1 =1.0/y; // en  kelvin
      temperatura1 = temperatura1 -273.15;  
    }
    else if (sensorPin == S2_TEM_PIN){ //TERRENO 2
      temperatura2 =1.0/y; // en  kelvin
      temperatura2 = temperatura2 -273.15;  
    }   
    else {
      Serial.println("PIN DE LECTURA DE TEMPERATURA INCORRECTO");
    }
  }
  else if ((sensorPin == S1_HUM_PIN) || (sensorPin == S2_HUM_PIN)){
    // QUE SECCION SE ESTÁ SENSANDO
    if (sensorPin == S1_HUM_PIN){ //TERRENO 1
      humedad1= -35.7*voltaje+178.57;
    }
    else if (sensorPin == S2_HUM_PIN){ //TERRENO 2
      humedad2= -35.7*voltaje+178.57;
    }
    else {
      Serial.println("PIN DE LECTURA DE HUMEDAD INCORRECTO");
    }  
  }
  else if ((sensorPin == S1_PH_PIN) || (sensorPin == S2_PH_PIN)){
    // QUE SECCION SE ESTÁ SENSANDO
    if (sensorPin == S1_PH_PIN){ //TERRENO 1
      ph1 = voltaje * 5.1 / 0.11;
    }
    else if (sensorPin == S2_PH_PIN){ //TERRENO 2
      ph2 = voltaje * 5.1 / 0.11;
    }
    else {
      Serial.println("PIN DE LECTURA DE PH INCORRECTO");
    }  
  }
}

void comparaciones(){
  // SECCION 1 (TERRENO 2) ///////////////////////////////////
  if (temperatura1 > temperaturaAlta ){
    Serial.println("TEMPERATURA SECCION 1 ALTA ");
  }
  else if (temperatura1 < temperaturaBaja ){//si se presiono una tecla se envia el mensaje tecla presionada
    Serial.println("TEMPERATURA SECCION 1 BAJA ");
  }    
  else{
    Serial.println("TEMPERATURA SECCION 1 OPTIMA ");
  }
  if (humedad1 > humedadAlta  ){//si se presiono una tecla se envia el mensaje tecla presionada
    Serial.println("HUMEDAD SECCION 1 ALTA ");
  }
  else if (humedad1 < humedadBaja ){//si se presiono una tecla se envia el mensaje tecla presionada 
    Serial.println("HUMEDAD SECCION 1 BAJA ");
  }
  else{
    Serial.println("HUMEDAD SECCION 1 OPTIMA ");
  }
  if (ph1 == phReferencia){
    Serial.println("PH SECCION 1 NEUTRO ");
  }
  else if (ph1 < phReferencia){
    Serial.println("PH SECCION 1 ÁCIDO ");
  }
  else if (ph1 > phReferencia){
    Serial.println("PH SECCION 1 ALCALINO ");
  }
  // SECCION 2 (TERRENO 2) //////////////////////////////    
  if (temperatura2 > temperaturaAlta ){
    Serial.println("TEMPERATURA SECCION 2 ALTA ");
  }
  else if (temperatura2 < temperaturaBaja ){//si se presiono una tecla se envia el mensaje tecla presionada
    Serial.println("TEMPERATURA SECCION 2 BAJA ");
  }    
  else{
    Serial.println("TEMPERATURA SECCION 2 OPTIMA ");
  }
  if (humedad2 > humedadAlta  ){//si se presiono una tecla se envia el mensaje tecla presionada
    Serial.println("HUMEDAD SECCION 2 ALTA ");
  }
  else if (humedad2 < humedadBaja ){//si se presiono una tecla se envia el mensaje tecla presionada 
    Serial.println("HUMEDAD SECCION 2 BAJA ");
  }
  else{
    Serial.println("HUMEDAD SECCION 2 OPTIMA ");
  }
  if (ph2 == phReferencia){
    Serial.println("PH SECCION 2 NEUTRO ");
  }
  else if (ph2 < phReferencia){
    Serial.println("PH SECCION 2 ÁCIDO ");
  }
  else if (ph2 > phReferencia){
    Serial.println("PH SECCION 2 ALCALINO ");
  }
}

void printSensed(){
  // IMPRIMIR VALORES SENSADOS EN EL TERRENO 1
  Serial.println("VALORES SENSADOS TERRENO 1");
  Serial.print("TEMPERATURA : ");
  Serial.println(temperatura1);
  Serial.print("HUMEDAD : ");
  Serial.println(humedad1);
  Serial.print("PH : ");
  Serial.println(ph1);
  // IMPRIMIR VALORES SENSADOS EN EL TERRENO 2
  Serial.println("VALORES SENSADOS TERRENO 2");
  Serial.print("TEMPERATURA : ");
  Serial.println(temperatura2);
  Serial.print("HUMEDAD : ");
  Serial.println(humedad2);
  Serial.print("PH : ");
  Serial.println(ph2);
}

void saveSD(int SS, time_t t){
  Serial.print("Initializing SD card...");

  if (!SD.begin(SS)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    // TERRENO 1
    myFile.println("/nVALORES TERRENO 1: /n TEMPERATURA = ");
    myFile.println(temperatura1);
    myFile.println("/n HUMEDAD = ");
    myFile.println(humedad1);
    myFile.println("/n PH = ");
    myFile.println(ph1);
    // TERRENO 2
    myFile.println("/n/nVALORES TERRENO 2: /n TEMPERATURA = ");
    myFile.println(temperatura2);
    myFile.println("/n HUMEDAD = ");
    myFile.println(humedad2);
    myFile.println("/n PH = ");
    myFile.println(ph2);
    // FEHCA Y HORA DE LA MEDICIÓN ////////////////////////////////
    myFile.println("/n/nFECHA Y HORA DE LA MEDICION");
    myFile.print(day(t));
    myFile.print(+ "/") ;
    myFile.print(month(t));
    myFile.print(+ "/") ;
    myFile.print(year(t)); 
    myFile.print( " ") ;
    myFile.print(hour(t));  
    myFile.print(+ ":") ;
    myFile.print(minute(t));
    myFile.print(":") ;
    myFile.println(second(t));
    
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}
