/// SINK NODE CONFIGURATION - ALPHA ///
/*
This node comunicates with Bravo, Charlie, Delta and Echo.
PT: Sistema telemático para el monitoreo y control de huertos urbanos basado en una red inalámbrica de sensore
Karla Benitez
Emilio Gallegos
*/
#include <HardwareSerial.h>
#include <XBee.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "defines.h"
#include "Credentials.h"
#include <MySQL_Generic.h>

// Inicializar HardwareSerial para la comunicación con XBee
HardwareSerial XBeeSerial(1); // Usando UART1 del ESP32 (GPIO 16, 17)
XBee xbee = XBee();

// Use to define the XBee payload
uint8_t payload[5];

/* Tx XBee */
uint32_t msb = 0x0013a200;
uint32_t lsb = 0x4213dbb5; // Bravos 
XBeeAddress64 addr64 = XBeeAddress64(msb, lsb);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();
/* Rx XBee */
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse(); // create reusable response objects for responses we expect to handle 
ModemStatusResponse msr = ModemStatusResponse();

/* Configuration required to connect to DB MySQL */
#define USING_HOST_NAME     true

#if USING_HOST_NAME
  char server[] = "yourdatabse.mysql.database.azure.com"; // Azure URL
#else
  IPAddress server(192, 168, X, X);
#endif

uint16_t server_port = 3306; //Azure DB Port

char default_database[] = "Your_MySQL_Database"; // DB Name
char default_table[]    = "Your_table"; // DB Table

String column1   = "idSuministro";
String column2   = "idNodo"; 
String column3   = "bandera_tipo_suministro";

//String query = String("SELECT ") + column1 + ", " + column2 + ", " + column3 + " FROM " + default_database + "." + default_table;

MySQL_Connection conn((Client *)&client);

// Create an instance of the cursor passing in the connection
MySQL_Query sql_query = MySQL_Query(&conn);

/* Configuration for HTTP method POST*/
const char* serverName = "https://your_web_page.com";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;
/*
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
/*#define TIME_TO_SLEEP  8     

RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
*/
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000); // wait for serial port to connect
  /*XBee configuration*/
  XBeeSerial.begin(9600, SERIAL_8N1, 16, 17); // Configurar UART1 para pines 16 (RX) y 17 (TX)
  xbee.setSerial(XBeeSerial);
  Serial.println("Setup completed. Starting communication...");
  /* MySQL_MariaDB_Generic Configuration*/
  MYSQL_DISPLAY1("\nStarting Basic_Select_WiFi on", BOARD_NAME);
  MYSQL_DISPLAY(MYSQL_MARIADB_GENERIC_VERSION);

  // Initialize your WiFi module
  #if ( USING_WIFI_ESP8266_AT  || USING_WIFIESPAT_LIB ) 
    #if ( USING_WIFI_ESP8266_AT )
      MYSQL_DISPLAY("Using ESP8266_AT/ESP8266_AT_WebServer Library");
    #elif ( USING_WIFIESPAT_LIB )
      MYSQL_DISPLAY("Using WiFiEspAT Library");
    #endif
    
    // initialize serial for ESP module
    EspSerial.begin(115200);
    // initialize ESP module
    WiFi.init(&EspSerial);

    MYSQL_DISPLAY(F("WiFi shield init done"));

    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD){
      MYSQL_DISPLAY(F("WiFi shield not present"));
      // don't continue
      while (true);
    }
  #endif

  // Begin WiFi section
  MYSQL_DISPLAY1("Connecting to", ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    MYSQL_DISPLAY0(".");
  }

  // print out info about the connection:
  MYSQL_DISPLAY1("Connected to network. My IP address is:", WiFi.localIP());
  MYSQL_DISPLAY3("Connecting to SQL Server @", server, ", Port =", server_port);
  MYSQL_DISPLAY5("User =", user, ", PW =", password, ", DB =", default_database);
}

void loop() {
  Serial.println("---------- START ----------");
  //print_wakeup_reason();
  Serial.println("---------- RX ----------");
  xbee.readPacket(5000);
    
  if (xbee.getResponse().isAvailable()) {
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      xbee.getResponse().getZBRxResponse(rx);
          
      if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
          Serial.println("The sender got an ACK");
          // Get Address
          XBeeAddress64 rxAddress = rx.getRemoteAddress64();
          int idNode = identifyNode(rxAddress);
          Serial.print("ID from the sender node: ");
          Serial.println(idNode);          

          // Access the payload
          int payloadLength = rx.getDataLength();
          uint8_t* receivedData = rx.getData();
          
          Serial.print("Received data: ");
          for (int i = 0; i < payloadLength; i++) {
            Serial.print((char)receivedData[i]);
          }
          Serial.println();
          //Send OK to node
          char messageO[] = {'O', 'K'};

          for (size_t i = 0; i < sizeof(messageO); i++) {
            payload[i] = messageO[i];
          }  
          Serial.println("Sending OK to Node");
          zbTx = ZBTxRequest(rxAddress, payload, sizeof(payload));
          xbee.send(zbTx);

          if (xbee.readPacket(200)) {
            if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
              xbee.getResponse().getZBTxStatusResponse(txStatus);

              uint8_t deliveryStatus = txStatus.getDeliveryStatus();
              if (deliveryStatus == SUCCESS) {
                Serial.println("Transmission successful.");
              } else {
                Serial.print("Transmission failed. Error code: ");
                Serial.println(deliveryStatus, HEX);
              }
            } else {
              Serial.println("Response received, but not a ZB_TX_STATUS_RESPONSE.");
            }
          } else if (xbee.getResponse().isError()) {
            Serial.print("Error reading packet. Error code: ");
            Serial.println(xbee.getResponse().getErrorCode(), HEX);
          } else {
            Serial.println("No response received in time.");
          }
          ///////////////////
/*
          // Crear una cadena para almacenar el payload completo
          char payloadString[21];
          if (payloadLength < 22) {
            strncpy(payloadString, (char*)receivedData, payloadLength);
            payloadString[payloadLength] = '\0';
          } else {
            strncpy(payloadString, (char*)receivedData, 20);
            payloadString[22] = '\0';
          }

          // Extraer los valores de los sensores del payload
          char temp_string[6];
          char hum_string[6];
          char pH_string[4];
          char N_string[4];
          char P_string[4];
          char K_string[4];

          strncpy(temp_string, payloadString, 4);
          temp_string[4] = '\0';

          strncpy(hum_string, payloadString + 4, 5);
          hum_string[5] = '\0';

          strncpy(pH_string, payloadString + 9, 3);
          pH_string[3] = '\0';

          strncpy(N_string, payloadString + 12, 3);
          N_string[3] = '\0';

          strncpy(P_string, payloadString + 15, 3);
          P_string[3] = '\0';

          strncpy(K_string, payloadString + 18, 3);
          K_string[3] = '\0';

          // Imprimir los valores extraídos
          Serial.print("Temp: ");
          Serial.print(temp_string);
          Serial.print(", Hum: ");
          Serial.print(hum_string);
          Serial.print(", pH: ");
          Serial.print(pH_string);
          Serial.print(", N: ");
          Serial.print(N_string);
          Serial.print(", P: ");
          Serial.print(P_string);
          Serial.print(", K: ");
          Serial.println(K_string);

          // Concatenar los valores extraídos al JSON
          char cadena1[256];
          snprintf(cadena1, sizeof(cadena1), 
                   "{\"node\":\"%d\",\"sensorTemp\":\"%s\",\"sensorHum\":\"%s\",\"sensorPH\":\"%s\",\"N\":\"%s\",\"P\":\"%s\",\"K\":\"%s\"}",
                   idNode, temp_string, hum_string, pH_string, N_string, P_string, K_string);
*/
          // Crear una cadena para almacenar el payload completo
          char payloadString[65];
          if (payloadLength < 65) {
            strncpy(payloadString, (char*)receivedData, payloadLength);
            payloadString[payloadLength] = '\0';
          } else {
            strncpy(payloadString, (char*)receivedData, 64);
            payloadString[64] = '\0';
          }
          
          // Extraer los valores de los sensores del payload
          char temp_string[6];
          char hum_string[6];
          char pH_string[6];
          char N_string[6];
          char P_string[6];
          char K_string[6];

          sscanf(payloadString, "%5[^|]|%5[^|]|%5[^|]|%5[^|]|%5[^|]|%5[^|]|", temp_string, hum_string, pH_string, N_string, P_string, K_string);

          // Imprimir los valores extraídos
          Serial.print("Temp: ");
          Serial.print(temp_string);
          Serial.print(", Hum: ");
          Serial.print(hum_string);
          Serial.print(", pH: ");
          Serial.print(pH_string);
          Serial.print(", N: ");
          Serial.print(N_string);
          Serial.print(", P: ");
          Serial.print(P_string);
          Serial.print(", K: ");
          Serial.println(K_string);

          // Concatenar los valores extraídos al JSON
          char cadena1[256];
          snprintf(cadena1, sizeof(cadena1), 
                   "{\"node\":\"%d\",\"sensorTemp\":\"%s\",\"sensorHum\":\"%s\",\"sensorPH\":\"%s\",\"N\":\"%s\",\"P\":\"%s\",\"K\":\"%s\"}",
                   idNode, temp_string, hum_string, pH_string, N_string, P_string, K_string);
          // Imprimir el JSON formado
          Serial.print("JSON: ");
          Serial.println(cadena1);

          /* HTTP POST method */
          if ((millis() - lastTime) > timerDelay){
              if(WiFi.status() == WL_CONNECTED){
                WiFiClientSecure client;
                client.setInsecure();
                HTTPClient https;

                https.begin(client, serverName);
                https.addHeader("Content-Type", "application/json");
                //char cadena1[] = "{\"node\":\"2\",\"sensorTemp\":\"22.5\",\"sensorHum\":\"100.0\",\"sensorPH\":\"7.5\",\"N\":\"123\",\"P\":\"123\",\"K\":\"123\"}";
                int httpResponseCode = https.POST(cadena1);

                /*if (httpResponseCode > 0) {
                  Serial.print("HTTP Response code: ");
                  Serial.println(httpResponseCode);
                } else {
                  Serial.print("Error code: ");
                  Serial.println(httpResponseCode);
                }*/

                if (httpResponseCode == 200){
                  Serial.print("HTTP Response code: ");
                  Serial.println(httpResponseCode);
                  Serial.println("Firebase got the package");
                } else if(httpResponseCode > 0) {
                  Serial.print("HTTP Response code: ");
                  Serial.println(httpResponseCode);
                  Serial.println("POST Retrying ...");
                  httpResponseCode = https.POST(cadena1);
                  if (httpResponseCode > 0) {
                    Serial.print("Retrying HTTP Response code: ");
                    Serial.println(httpResponseCode);
                  } else {
                    Serial.print("Retrying Error code: ");
                    Serial.println(httpResponseCode);
                  }               
                } else {
                  Serial.print("Error code: ");
                  Serial.println(httpResponseCode);
                }

                https.end();
              } else {
                Serial.println("WiFi Disconnected");
              }
              lastTime = millis();
            }

            /*MySQL connections*/
            MYSQL_DISPLAY("Connecting...");
            if (conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL){
              delay(500);
              runQuery(rxAddress, idNode);
              conn.close();// close the connection
            }else{
              MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");

              MySQL_Connection conn((Client *)&client);
              MySQL_Query sql_query = MySQL_Query(&conn);              
            }
            MYSQL_DISPLAY("\nFinishing DB actions");       
      } else {
          Serial.println("This is a ZigBee Receive Packet, but sender didn't get an ACK");       
      }
    } else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
      xbee.getResponse().getModemStatusResponse(msr);
      // the local XBee sends this response on certain events, like association/dissociation
      
      if (msr.getStatus() == ASSOCIATED) {
        Serial.println("This is a ZigBee Modem Status Frame with status 2");
      } else if (msr.getStatus() == DISASSOCIATED) {
        Serial.println("This is a ZigBee Modem Status Frame with status 3");
      } else {
        Serial.println("This is a ZigBee Modem Status Frame with a differnt status");
      }
    } else {
      Serial.println("It's an unknown ZigBee Frame");    
    }
  } else if (xbee.getResponse().isError()) {
    //nss.print("Error reading packet.  Error code: ");  
    //nss.println(xbee.getResponse().getErrorCode());
  }
  
  /*esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");*/
  
  delay(5000);
  Serial.println("---------- END ----------");
  delay(3000);
}

int identifyNode(XBeeAddress64 address){
  int node = 0;
  switch(address.getLsb()){
    case 0x4213xxxx: //Bravo
      node = 2;
      break;
    case 0x4213xxxx: //Charlie
      node = 3;
      break;
    case 0x4213xxxx: //Delta
      node = 4;
      break;
    case 0x4213xxxx: //Echo
      node = 5;
  }
  return node;
}

void describeError(uint8_t code) {//For XBee
  switch (code){
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

void runQuery(XBeeAddress64 address, int idNode){
  row_values *row = NULL;
  int idSuministro[100];
  int idNodo[100];
  int bandera_tipo_suministro[100];
  int row_count = 0;

  // Initiate the query class instance
  MySQL_Query query_mem = MySQL_Query(&conn);

  String query = String("SELECT ") + column1 + ", " + column2 + ", " + column3 + " FROM " + default_database + "." + default_table + " WHERE idNodo = " + idNode;
  
  // Execute the query
  MYSQL_DISPLAY(query);

  // KH, check if valid before fetching
  if ( !query_mem.execute(query.c_str()) )
  {
    MYSQL_DISPLAY("Querying error");
    return;
  }

  query_mem.get_columns();

  // Read the row
  do{
    row = query_mem.get_next_row();
    
    if(row != NULL){
      // Assuming there are three columns in the result
      idSuministro[row_count] = atoi(row->values[0]);
      idNodo[row_count] = atoi(row->values[1]);
      bandera_tipo_suministro[row_count] = atoi(row->values[2]);
      MYSQL_DISPLAY1("idSuministro =", idSuministro[row_count]);
      MYSQL_DISPLAY1("idNodo =", idNodo[row_count]); 
      MYSQL_DISPLAY1("bandera_tipo_suministro =", bandera_tipo_suministro[row_count]);     

      /* Send indication to node */
      Serial.println("---------- TX Start ----------");
      char messageW[] = {'W', 'a', 't', 'e', 'r'};
      char messageN[] = {'N', 'u', 't', 'r', 'i'};

      if(bandera_tipo_suministro[row_count] == 0){
        Serial.println("Water Valve");
        for (size_t i = 0; i < sizeof(messageW); i++){
          payload[i] = messageW[i];
        } 
      }else if(bandera_tipo_suministro[row_count] == 1){
        Serial.println("Nutrient Valve");
        for (size_t i = 0; i < sizeof(messageN); i++){
          payload[i] = messageN[i];
        } 
      }  
      
      row_count++;           

      Serial.println("Sending payload with indication via XBee...");
      zbTx = ZBTxRequest(address, payload, sizeof(payload));
      xbee.send(zbTx);

      if (xbee.readPacket(200)) {
        if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
          xbee.getResponse().getZBTxStatusResponse(txStatus);

          uint8_t deliveryStatus = txStatus.getDeliveryStatus();
          if (deliveryStatus == SUCCESS) {
            Serial.println("Transmission successful.");
          } else {
            Serial.print("Transmission failed. Error code: ");
            Serial.println(deliveryStatus, HEX);
            describeError(deliveryStatus);
//////////////////////////////////////////////////////////
            Serial.println("Retring.....");
            xbee.send(zbTx);
            if (xbee.readPacket(200)) {
              if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
                xbee.getResponse().getZBTxStatusResponse(txStatus);

                uint8_t deliveryStatus = txStatus.getDeliveryStatus();
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
              Serial.println("No response received in time.");
            }
/////////////////////////////////////////////////////////
          }
        } else {
          Serial.println("Response received, but not a ZB_TX_STATUS_RESPONSE.");
        }
      } else if (xbee.getResponse().isError()) {
        Serial.print("Error reading packet. Error code: ");
        Serial.println(xbee.getResponse().getErrorCode(), HEX);
      } else {
        Serial.println("No response received in time.");
      }
      Serial.println("---------- TX End ----------");
      // Wait for Valve activation
      Serial.println("Pause");  
      delay(16000);
    }else{
      Serial.println("Empty data base");    

      //Send indication to Sleep
      char messageS[] = {'S', 'l', 'e', 'e', 'p'};

      for (size_t i = 0; i < sizeof(messageS); i++) {
        payload[i] = messageS[i];
      }    

      Serial.println("---------- TX Start ----------");   
      Serial.println("Sending payload with sleep indication via XBee...");
      zbTx = ZBTxRequest(address, payload, sizeof(payload));
      xbee.send(zbTx);

      if (xbee.readPacket(200)) {
        if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
          xbee.getResponse().getZBTxStatusResponse(txStatus);

          uint8_t deliveryStatus = txStatus.getDeliveryStatus();
          if (deliveryStatus == SUCCESS) {
            Serial.println("Transmission successful.");
          } else {
            Serial.print("Transmission failed. Error code: ");
            Serial.println(deliveryStatus, HEX);
            describeError(deliveryStatus);
/////////////////////////////////////////////////////////////////////////////////
            Serial.println("Retring.....");
            xbee.send(zbTx);
            if (xbee.readPacket(200)) {
              if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
                xbee.getResponse().getZBTxStatusResponse(txStatus);

                uint8_t deliveryStatus = txStatus.getDeliveryStatus();
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
              Serial.println("No response received in time.");
            }
/////////////////////////////////////////////////////////////////////////////////            
          }
        } else {
          Serial.println("Response received, but not a ZB_TX_STATUS_RESPONSE.");
        }
      } else if (xbee.getResponse().isError()) {
        Serial.print("Error reading packet. Error code: ");
        Serial.println(xbee.getResponse().getErrorCode(), HEX);
      } else {
        Serial.println("No response received in time.");
      }  
      Serial.println("---------- TX End ----------");
    }
  } while (row != NULL);

  delay(500);

  // Delete the records that were read
  /*for (int i = 0; i < row_count; i++){
    String delete_query = String("DELETE FROM ") + default_database + "." + default_table + " WHERE idSuministro = " + String(idSuministro[i]);
    runDelete(delete_query);
  }*/

  String delete_query = String("DELETE FROM ") + default_database + "." + default_table + " WHERE idNodo = " + idNode;
  runDelete(delete_query);

  sql_query.close();
}

void runDelete(String delete_query){
  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected()){
    MYSQL_DISPLAY(delete_query);
    
    // Execute the query
    if ( !query_mem.execute(delete_query.c_str()) ) {
      MYSQL_DISPLAY("Delete error");
    } else {
      MYSQL_DISPLAY("Data Deleted.");
    }
  } else {
    MYSQL_DISPLAY("Disconnected from Server. Can't delete.");
  }
}
