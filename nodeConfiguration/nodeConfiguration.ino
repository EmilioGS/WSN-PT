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

// Switch to enable node components
#define enable 6

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

uint8_t payload[34];

/* Tx XBee */
uint32_t msb = 0x0013a200;
uint32_t lsb = 0x4213da49; // Alpha's 64bits address
XBeeAddress64 addr64 = XBeeAddress64(msb, lsb);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
/* Rx XBee */
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse(); // create reusable response objects for responses we expect to handle 
ModemStatusResponse msr = ModemStatusResponse();

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
  // Set enable
  pinMode(enable, OUTPUT);  
  // Set the baud rate for the Xbee serial comunication
  XBee_Serial.begin(9600);
  xbee.setSerial(XBee_Serial);
}

void loop() {
  Serial.println("---------- START ----------"); // Debugging print  

  digitalWrite(enable, HIGH); // Enable all node component

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
      //Serial.print("Invalid temperature value");   
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
  mod.listen();// Change Software Serial
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

    humidity_value = measure_value(all_values[3], all_values[4], 1);
    pH_value = measure_value(all_values[9], all_values[10], 1);
    nitrogen_value = measure_value(all_values[11], all_values[12], 2);
    phosphorus_value = measure_value(all_values[13], all_values[14], 2);
    potassium_value = measure_value(all_values[15], all_values[16], 2);
    
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
  Serial.print(", pH: ");
  Serial.print(pH_value);
  Serial.print(", Nitrogen: ");
  Serial.print(nitrogen_value);
  Serial.print(", Phosphorus: ");
  Serial.print(phosphorus_value);
  Serial.print(", Potassium: ");
  Serial.println(potassium_value);
  
  //-/-/-/-// Comunication //-/-/-/-//
  // Variables
  char temperature_string[8];
  char humidity_string[8];
  char pH_string[8];
  char nitrogen_string[8];
  char phosphorus_string[8];
  char potassium_string[8];
  char message[36];
  
  //-/ Conversions /-//
  dtostrf(temperature_value, 5, 2, temperature_string);
  dtostrf(humidity_value, 5, 2, humidity_string);
  dtostrf(pH_value, 5, 2, pH_string);
  dtostrf(nitrogen_value, 5, 2, nitrogen_string);
  dtostrf(phosphorus_value, 5, 2, phosphorus_string);
  dtostrf(potassium_value, 5, 2, potassium_string);
 
  // String concatenation
  /*strcpy(message, temperature_string);
  strcat(message, "|");
  strcat(message, humidity_string);
  strcat(message, "|");
  strcat(message, pH_string);
  strcat(message, "|");
  strcat(message, nitrogen_string);
  strcat(message, "|");
  strcat(message, potassium_string);
  strcat(message, "|");
  strcat(message, phosphorus_string);
  strcat(message, "|");

  Serial.print("Concatenated string: '");
  Serial.print(message);  
  Serial.print("' and strlen: ");
  Serial.print(strlen(message)); 
  //Serial.print("' and sizeof: "); 
  //Serial.println(sizeof(message));
  delay(500);
  
  memset(payload, 0, sizeof(payload)); //Agregado
  // Convertir cada carácter de la cadena a su representación hexadecimal
  for (size_t i = 0; i < strlen(message); i++) {
    if(message[i] != ' ' && message[i] != 0x00){
      payload[i] = message[i];
    }
  }*/
  // Quitar espacios en blanco al principio de los valores convertidos
  trimLeadingSpaces(temperature_string);
  trimLeadingSpaces(humidity_string);
  trimLeadingSpaces(pH_string);
  trimLeadingSpaces(nitrogen_string);
  trimLeadingSpaces(phosphorus_string);
  trimLeadingSpaces(potassium_string);
  
  // Concatenación de las cadenas
  snprintf(message, sizeof(message), "%s|%s|%s|%s|%s|%s|", 
          temperature_string, humidity_string, pH_string, 
          nitrogen_string, phosphorus_string, potassium_string);

  Serial.print("Concatenated string: '");
  Serial.print(message);  
  Serial.print("', strlen: ");
  Serial.print(strlen(message)); 
  Serial.print(", sizeof: ");
  Serial.print(sizeof(message));
  Serial.print(" and sizeof payload: ");
  Serial.println(sizeof(payload));

  // Copiar los datos al payload
  memset(payload, 0, sizeof(payload));
  strncpy((char*)payload, message, sizeof(payload));

  Serial.println("---------- TX Start ----------");
  //Send message
  XBee_Serial.listen();
  bool sinkConfirmation =  false; 
  do{
    Serial.println("Sending payload to Alpha");
    //zbTx = ZBTxRequest(addr64, payload, sizeof(payload));   
    //zbTx = ZBTxRequest(addr64, payload, strlen(message));
    //zbTx = ZBTxRequest(addr64, payload, sizeof(message));
    xbee.send(zbTx);
  
    /*if (xbee.readPacket(200)){
      if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
        xbee.getResponse().getZBTxStatusResponse(txStatus);

        uint8_t deliveryStatus = txStatus.getDeliveryStatus();
        if (deliveryStatus == SUCCESS) {
          Serial.println("Transmission successful.");
          sinkConfirmation =  true;          
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
      Serial.println("No response received in time");
    }*/

    Serial.println("Waiting Alpha's confirmation");

    xbee.readPacket(1000);
    
    if (xbee.getResponse().isAvailable()) {
      // got something
      uint8_t apId = xbee.getResponse().getApiId();
      //if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      if (apId == ZB_RX_RESPONSE) {
        // got a zb rx packet
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
            
        if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
            Serial.println("The sender got an ACK");
            // Access the payload
            int payloadLength = rx.getDataLength();
            uint8_t* receivedData = rx.getData();
            
            Serial.print("Received data: ");
            for (int i = 0; i < payloadLength; i++) {
              Serial.print((char)receivedData[i]);
            }
            Serial.println();

            if((char)receivedData[0] == 'O'){
              // Continue with Loop
              sinkConfirmation = true;
            } else{
              Serial.println("Unknown action");              
            }
         
        } else {
            Serial.println("This is a ZigBee Receive Packet, but sender didn't get an ACK");       
        }
      } /*else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
        xbee.getResponse().getModemStatusResponse(msr);
        // the local XBee sends this response on certain events, like association/dissociation
        if (msr.getStatus() == ASSOCIATED) {
          Serial.println("This is a ZigBee Modem Status Frame with status 2");
        } else if (msr.getStatus() == DISASSOCIATED) {
          Serial.println("This is a ZigBee Modem Status Frame with status 3");
        } else {
          Serial.println("This is a ZigBee Modem Status Frame with a differnt status");
        }
      } *//*else if (apId == ZB_TX_STATUS_RESPONSE) {
        xbee.getResponse().getZBTxStatusResponse(txStatus);

        uint8_t deliveryStatus = txStatus.getDeliveryStatus();
        if (deliveryStatus == SUCCESS) {
          Serial.println("Transmission successful.");
          sinkConfirmation =  true;          
          Serial.println("Got a Tx Response from Alpha");
        } else {
          Serial.print("Transmission failed. Error code: ");
          Serial.println(deliveryStatus, HEX);
          describeError(deliveryStatus);
        }
      } */else {///termino
        Serial.println("It's an unexpected ZigBee Frame");    
      }
    } else if (xbee.getResponse().isError()) {
      //nss.print("Error reading packet.  Error code: ");  
      //nss.println(xbee.getResponse().getErrorCode());
    }

    Serial.println("End of TX loop iteration."); 
    delay(3000);
  }while(!sinkConfirmation);

  Serial.println("---------- TX End ----------");

  Serial.println("---------- RX ----------");
  
  bool continue_to_sleep = false;
  do{

    xbee.readPacket(5000);
    
    if (xbee.getResponse().isAvailable()) {
      // got something
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        // got a zb rx packet
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
            
        if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
            Serial.println("The sender got an ACK");
            // Access the payload
            int payloadLength = rx.getDataLength();
            uint8_t* receivedData = rx.getData();
            
            Serial.print("Received data: ");
            for (int i = 0; i < payloadLength; i++) {
              Serial.print((char)receivedData[i]);
            }
            Serial.println();

            //-/-/-/-// Actuators //-/-/-/-//

            if((char)receivedData[0] == 'N'){
              // Activate nutrient valve
              actuatorActivation(1);
            } else if((char)receivedData[0] == 'W'){
              // Activate nutrient valve
              actuatorActivation(0);
            } else if((char)receivedData[0] == 'S'){
              //Can get out of the do-while
              continue_to_sleep = true;
              Serial.println("Continue to Sleep Mode");
            } else{
              Serial.println("Unknown action");              
            }
         
        } else {
            Serial.println("This is a ZigBee Receive Packet, but sender didn't get an ACK");       
        }
      } else {
        Serial.println("It's an unknown ZigBee Frame");    
      }
    } else if (xbee.getResponse().isError()) {
      //nss.print("Error reading packet.  Error code: ");  
      //nss.println(xbee.getResponse().getErrorCode());
    }
  
    Serial.println("End of RX loop iteration."); 

    delay(200);
  }while(!continue_to_sleep);

  /* Sleep Mode*/
  digitalWrite(enable, LOW); // Disenable all node component
  Serial.println("Sleep for 2 minutes");
  delay(200);
  //for (int i = 0 ;  i  <  16 ; i++){
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  //}
  delay(200);

  Serial.println("---------- END ----------"); 
  delay(5000);
}

float measure_value(byte temp_high_byte, byte temp_low_byte, int flag){
  // Combine first byte with the second one
  int combined_value = (temp_high_byte << 8) | temp_low_byte;
  float decimal_value = 0;

  if(flag == 1){
    // Convert value to float with decimal shift
    decimal_value = (float)combined_value/10;    
  } else if(flag == 2){  
    // Convert value to float
    decimal_value = (float)combined_value;
  }

  // Print the returned value
  //Serial.print("Valor en flotante: ");
  //Serial.println(decimal_value);  

  return decimal_value;
}
/*
float measure_value(byte high, byte low, int decimal) {
  return ((high << 8) + low) / pow(10, decimal);
}*/

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

void actuatorActivation(int valve){
    
  if(valve == 1){
    Serial.println("NPK valve start");   
    digitalWrite(valveNPK, HIGH);
    delay(5000);
    digitalWrite(valveNPK, LOW);
    Serial.println("Valve end"); 
  }else if(valve == 0){
    Serial.println("Water valve start");
    digitalWrite(valveWater, HIGH);
    delay(5000);
    digitalWrite(valveWater, LOW);
    Serial.println("Valve end");
  }else{
    Serial.println("Valve error");
  }
}

void trimLeadingSpaces(char* str) {
  char* start = str;
  while (*start == ' ') {
    start++;
  }
  if (start != str) {
    char* dst = str;
    while (*start) {
      *dst++ = *start++;
    }
    *dst = '\0';
  }
}
