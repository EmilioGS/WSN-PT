/// NODE CONFIGURATION - BRAVO ///
/*
First sensor node in the WSN
This node comunicates with the sink node, alfa, and with charlie.
PT: Sistema telemático para el monitoreo y control de huertos urbanos basado en una red inalámbrica de sensore
Karla Benitez
Emilio Gallegos
*/

#include <stdio.h> // Standard Input Output library
#include <SoftwareSerial.h> // Used to communicate serially with XBEE and NPK Sensor
#include <DHT.h> // Used to read DHT11 Sensor
#include <XBee.h> // Andrewrapp's XBee Library 
#include "LowPower.h" // Library to reduce current consuption

//RS485 to TTL Protocol converter control pins
#define RE 8
#define DE 7

#define DHTTYPE DHT11 // Specifying the tempertre and moisure sensor model
#define DHTPIN 10 //Pin connected to DHT11 sensor
DHT dht(DHTPIN, DHTTYPE); // Initializing object DHT11

// Valves control pins
#define valveNPK 12
#define valveWater 13

// Modbus RTU requests for reading NPK values
const byte all_values_request[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};

SoftwareSerial mod(4, 5); //R0, DI 

SoftwareSerial XBee_Serial(3, 2); //Rx, Tx from the XBee module

// Create the XBee object
XBee xbee = XBee();

uint8_t payload[40];

// SH + SL Address of Alpha
XBeeAddress64 addr64_Alpha = XBeeAddress64(0x0013a200, 0x4213da49);
ZBTxRequest zbTx_A = ZBTxRequest(addr64_Alpha, payload, sizeof(payload));
ZBTxStatusResponse txStatus_A = ZBTxStatusResponse();

// SH + SL Address of Charlie
XBeeAddress64 addr64_Charlie = XBeeAddress64(0x0013a200, 0x4213de41);
ZBTxRequest zbTx_C = ZBTxRequest(addr64_Charlie, payload, sizeof(payload));
ZBTxStatusResponse txStatus_C = ZBTxStatusResponse();

void setup() {
  // Set the baud rate for the Serial port
  Serial.begin(9600);
  // DHT11 Sensor serial comunication
  dht.begin();
  // Set the baud rate for the Humidity-pH-NPK sensor
  mod.begin(4800);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  // Set valves controls pins as output
  pinMode(valveNPK, OUTPUT);
  pinMode(valveWater, OUTPUT);
  // Set the baud rate for the Xbee serial comunication
  XBee_Serial.begin(9600);
  xbee.setSerial(XBee_Serial);
}

void loop() {
  Serial.println("---------- START ----------"); // Debugging print  
  
  //-/-/-/-// Sensors //-/-/-/-//
  // Variable used to store the temperature
  float temperature_value, humidity_value, temperature2_value, pH_value, nitrogen_value, phosphorus_value, potassium_value;
  // variable used to store the response of the humidity, pH, Nitrogen, Phosphorus and Potassium sensor
  byte all_values[19]; // The frame returned by the sensor is 19 bytes
  memset(all_values, 0, sizeof(all_values)); // Fill all with 0
  
  // Get temperature value from DHT11 sensor
  int counter1 = 0;
  int counter_attempts1 = 3;
  bool valid_measured_value = false;
  do{
    temperature_value = dht.readTemperature(); // Read temperature value
    
    // Check if the data measure is consistent
    if(isnan(temperature_value)){
      Serial.print("Invalid temperature value");   
    }else{
      Serial.print("Measured temperature: ");
      Serial.println(temperature_value);
      valid_measured_value = true;
    }

    counter1++;
    Serial.print("Counter 1: ");
    Serial.println(counter1); 

    delay(1000);
  }while(!valid_measured_value && counter1 < counter_attempts1)  ;
  
  // Get temperature value from the humidity, pH, Nitrogen, Phosphorus and Potassium sensor
  int counter2 = 0;
  int counter_attempts2 = 6;
  bool valid_frame = false;
  bool valid_values = false;
  mod.listen();
  do{// Cycle do-while to validate valid sensor frame
    //Define protocol converter to transmitter
    digitalWrite(DE,HIGH);
    digitalWrite(RE,HIGH);
        // Send request for all value
    if(mod.write(all_values_request,sizeof(all_values_request))==8){
      //Define protocol converter to receiver
      digitalWrite(DE,LOW);
      digitalWrite(RE,LOW);
      for(byte i=0;i<=18;i++){
        all_values[i] = mod.read();
        //Serial.print(all_values[i],HEX);
      }
      //Serial.println(" ");
    }

    do{// Empty softwareserial buffer (Flush)
      (void)mod.read();
      //Serial.println(".");
    }while(mod.available() != 0);

    // Check if the received frame starts with the expected bytes
    if (all_values[0] == 0x01 && all_values[1] == 0x03 && all_values[2] == 0x0E) {
      valid_frame = true;
      //Serial.println("Valid Frame");
    }else{
      //Serial.println("Invalid Frame");      
    }

    humidity_value = measure_value(all_values[3], all_values[4]);
    temperature2_value = measure_value(all_values[5], all_values[6]);
    pH_value = measure_value(all_values[9], all_values[10]);
    nitrogen_value = measure_value(all_values[11], all_values[12]);
    phosphorus_value = measure_value(all_values[13], all_values[14]);
    potassium_value = measure_value(all_values[15], all_values[16]);
    
    // Check if the data received is consistent
    if (pH_value >= 0 && pH_value <= 14 && temperature2_value >= 0 && temperature2_value <= 45){
      valid_values = true;
      //Serial.println("Valid Data");
    }else{
      //Serial.println("Invalid Data");      
    }
 
    counter2++;
    Serial.print("Counter 2: ");
    Serial.println(counter2); 
    
    delay(3000);
  } while ((!valid_frame || !valid_values) && counter2 < counter_attempts2); 

  Serial.print("Final values - temperature: ");
  Serial.print(temperature_value);
  Serial.print(", humidity: ");
  Serial.print(humidity_value);
  Serial.print(", temeprature2: ");
  Serial.print(temperature2_value);
  Serial.print(", pH: ");
  Serial.print(pH_value);
  Serial.print(", Nitrogen: ");
  Serial.print(nitrogen_value);
  Serial.print(", Phosphorus: ");
  Serial.print(phosphorus_value);
  Serial.print(", Potassium: ");
  Serial.println(potassium_value);
  
  //-/-/-/-// Actuators //-/-/-/-//

  // Activate nutrient valve
  Serial.println("NPK valve start");
  digitalWrite(valveNPK, HIGH);
  delay(3000);
  digitalWrite(valveNPK, LOW);
  Serial.println("NPK valve end");

  delay(500);
  // Activate water valve
  Serial.println("Water valve start");
  digitalWrite(valveWater, HIGH);
  delay(3000);
  digitalWrite(valveWater, LOW);
  Serial.println("Water valve end");
  
  //-/-/-/-// Comunication //-/-/-/-//
  Serial.println("XBee Comunication");
  /*if(XBee.available() > 0){
    int dato = XBee.read();
    Serial.write(char(dato));
  }*/
  // Variables
  char temperature_string[8];
  char humidity_string[8];
  char temperature2_string[8];
  char pH_string[8];
  char nitrogen_string[8];
  char phosphorus_string[8];
  char potassium_string[8];
  char message[16];
  
  //-/ Conversions /-//
  dtostrf(temperature_value, 5, 2, temperature_string);
  dtostrf(humidity_value, 5, 2, humidity_string);
  dtostrf(temperature2_value, 5, 2, temperature2_string);
  dtostrf(pH_value, 5, 2, pH_string);
  dtostrf(nitrogen_value, 5, 2, nitrogen_string);
  dtostrf(phosphorus_value, 5, 2, phosphorus_string);
  dtostrf(potassium_value, 5, 2, potassium_string);

  /*Serial.print("Measured values strings: ");
  Serial.println(temperature_string);
  Serial.println(humidity_string);
  Serial.println(temperature2_string);
  Serial.println(pH_string);
  Serial.println(nitrogen_string);
  Serial.println(phosphorus_string);
  Serial.println(potassium_string);  */      

  // String concatenation
  strcpy(message, temperature_string);
  strcat(message, " ");
  strcat(message, humidity_string);
  strcat(message, " ");
  strcat(message, temperature2_string);
  strcat(message, " ");
  strcat(message, pH_string);
  strcat(message, " ");
  strcat(message, nitrogen_string);
  strcat(message, " ");
  strcat(message, potassium_string);
  strcat(message, " ");
  strcat(message, phosphorus_string);
  
  //Send message
  Serial.print("Concatenated string: '");
  Serial.print(message);  
  Serial.print("' and sizeoff: ");
  Serial.println(strlen(message));  
  delay(500);
  
  /*XBee.listen();
  delay(1000);
  XBee.print(message);*/
  // Convertir cada carácter de la cadena a su representación hexadecimal
  for (size_t i = 0; i < sizeof(message) - 1; i++) {
    payload[i] = message[i];
  }

  xbee.send(zbTx_A);
  
  if (xbee.readPacket(1000)) {
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      xbee.getResponse().getZBTxStatusResponse(txStatus_A);

      uint8_t deliveryStatus = txStatus_A.getDeliveryStatus();
      if (deliveryStatus == SUCCESS) {
        Serial.println("Transmission successful.");
      } else {
        Serial.print("Transmission failed. Error code: ");
        Serial.println(deliveryStatus, HEX);
        describeError(deliveryStatus);
      }
    } else {
      Serial.println("Response received, but not a ZB_TX_STATUS_RESPONSE.");
    }
  } else if (xbee.getResponse().isError()) {
    Serial.print("Error reading packet. Error code: ");
    Serial.println(xbee.getResponse().getErrorCode(), HEX);
  } else {
    Serial.println("No response received in time. Possible reasons:");
    Serial.println("- Destination XBee not reachable.");
    Serial.println("- Signal interference.");
    Serial.println("- Incorrect XBee address.");
    Serial.println("- Power issues.");
  }
  
  /* Sleep Mode*/
  /*Serial.println("Sleep for 2 minutes");
  delay(200);
  for (int i = 0 ;  i  <  16 ; i++){
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  delay(200);*/

  Serial.println("---------- END ----------"); 
  delay(5000);
}

float measure_value(byte temp_high_byte, byte temp_low_byte){
  // Combine first byte with the second one
  int combined_value = (temp_high_byte << 8) | temp_low_byte;
    
  // Convert value to float with decimal shift
  float decimal_value = (float)combined_value/10;

  // Print the returned value
  //Serial.print("Valor en flotante: ");
  //Serial.println(decimal_value);  

  return decimal_value;
}

void describeError(uint8_t code) {
  switch (code) {
    case 0x01:
      Serial.println("MAC ACK failure");
      break;
    case 0x02:
      Serial.println("CCA failure");
      break;
    case 0x15:
      Serial.println("Invalid destination endpoint");
      break;
    case 0x21:
      Serial.println("Network ACK failure");
      break;
    case 0x22:
      Serial.println("Not joined to network");
      break;
    case 0x23:
      Serial.println("Self-addressed");
      break;
    case 0x24:
      Serial.println("Address not found");
      break;
    case 0x25:
      Serial.println("Route not found");
      break;
    default:
      Serial.println("Unknown error");
      break;
  }
}
