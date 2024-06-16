#include <HardwareSerial.h> //
#include <XBee.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Inicializar HardwareSerial para la comunicación con XBee
HardwareSerial XBeeSerial(1); // Usando UART1 del ESP32 (GPIO 16, 17)
XBee xbee = XBee();

// Definir el payload
uint8_t payload[20];

// Dirección del XBee receptor
uint32_t msb = 0x0013a200;
uint32_t lsb = 0x4213dbb5; // Cambia esto según tu configuración
XBeeAddress64 addr64 = XBeeAddress64(msb, lsb);
ZBTxRequest zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

/* Configuration required to connect to DB MySQL */
#include "defines.h"
#include "Credentials.h"

#include <MySQL_Generic.h>
#define USING_HOST_NAME     true

#if USING_HOST_NAME
  char server[] = "huertalia.mysql.database.azure.com"; // Azure URL
#else
  IPAddress server(192, 168, 2, 112);
#endif

uint16_t server_port = 3306; //Azure DB Port

char default_database[] = "huertalia"; // DB Name
char default_table[]    = "Suministro2"; // DB Table

String column1   = "idSuministro";
String column2   = "idNodo"; 
String column3   = "bandera_tipo_suministro";

String query = String("SELECT ") + column1 + ", " + column2 + ", " + column3 + " FROM " + default_database + "." + default_table;

MySQL_Connection conn((Client *)&client);

// Create an instance of the cursor passing in the connection
MySQL_Query sql_query = MySQL_Query(&conn);

/* Configuration for HTTP method POST*/
const char* serverName = "https://proyecto-terminal-896bb.web.app/update-sensor";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

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
  if (WiFi.status() == WL_NO_SHIELD)
  {
    MYSQL_DISPLAY(F("WiFi shield not present"));
    // don't continue
    while (true);
  }
#endif

  // Begin WiFi section
  MYSQL_DISPLAY1("Connecting to", ssid);

  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
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

  char message[] = {'H', 'o', 'l', 'a', ' ', 'd', 'e', 's', 'd', 'e', ' ', 'E', 'S', 'P', '3', '2'};
  
  Serial.println(message);
  Serial.println(sizeof(message));

  for (size_t i = 0; i < sizeof(message); i++) {
    payload[i] = message[i];
  }

  Serial.println("Sending payload via XBee...");
  zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
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
/*
  Serial.println("---------- RX ----------");

  xbee.readPacket(5000);
    
  if (xbee.getResponse().isAvailable()) {
    // got something
    
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      // got a zb rx packet
      
      // now fill our zb rx class
      xbee.getResponse().getZBRxResponse(rx);
          
      if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
          // the sender got an ACK
          flashLed(statusLed, 10, 10);
      } else {
          // we got it (obviously) but sender didn't get an ACK
          flashLed(errorLed, 2, 20);
      }
      // set dataLed PWM to value of the first byte in the data
      analogWrite(dataLed, rx.getData(0));
    } else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
      xbee.getResponse().getModemStatusResponse(msr);
      // the local XBee sends this response on certain events, like association/dissociation
      
      if (msr.getStatus() == ASSOCIATED) {
        // yay this is great.  flash led
        flashLed(statusLed, 10, 10);
      } else if (msr.getStatus() == DISASSOCIATED) {
        // this is awful.. flash led to show our discontent
        flashLed(errorLed, 10, 10);
      } else {
        // another status
        flashLed(statusLed, 5, 10);
      }
    } else {
      // not something we were expecting
      flashLed(errorLed, 1, 25);    
    }
  } else if (xbee.getResponse().isError()) {
    //nss.print("Error reading packet.  Error code: ");  
    //nss.println(xbee.getResponse().getErrorCode());
  }
*/

 /* PSOT*/
if ((millis() - lastTime) > timerDelay) {
    if(WiFi.status() == WL_CONNECTED){
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient https;

      https.begin(client, serverName);
      https.addHeader("Content-Type", "application/json");

      char cadena1[] = "{\"node\":\"2\",\"sensorTemp\":\"22.5\",\"sensorHum\":\"100.0\",\"sensorPH\":\"7.5\",\"N\":\"123\",\"P\":\"123\",\"K\":\"123\"}";
      int httpResponseCode = https.POST(cadena1);

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
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
    runQuery();
    conn.close();// close the connection
  }else{
    MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
  }

  MYSQL_DISPLAY("\nSleeping...");
  MYSQL_DISPLAY("================================================");
 
  delay(60000);
  Serial.println("---------- END ----------");
  delay(10000);
}

void describeError(uint8_t code) {//For XBee
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

void runQuery(){
  row_values *row = NULL;
  int idSuministro[100];
  int idNodo[100];
  int bandera_tipo_suministro[100];
  int row_count = 0;

  // Initiate the query class instance
  MySQL_Query query_mem = MySQL_Query(&conn);
  
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
  do 
  {
    row = query_mem.get_next_row();
    
    if (row != NULL) 
    {
      // Assuming there are three columns in the result
      idSuministro[row_count] = atoi(row->values[0]);
      idNodo[row_count] = atoi(row->values[1]);
      bandera_tipo_suministro[row_count] = atoi(row->values[2]);

      MYSQL_DISPLAY1("idSuministro =", idSuministro[row_count]);
      MYSQL_DISPLAY1("idNodo =", idNodo[row_count]); 
      MYSQL_DISPLAY1("bandera_tipo_suministro =", bandera_tipo_suministro[row_count]);  

      row_count++;     
    }
  } while (row != NULL);

  
  delay(500);

  // Delete the records that were read
  for (int i = 0; i < row_count; i++) 
  {
    String delete_query = String("DELETE FROM ") + default_database + "." + default_table + " WHERE idSuministro = " + String(idSuministro[i]);
    runDelete(delete_query);
  }

  sql_query.close();
}

void runDelete(String delete_query)
{
  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected())
  {
    MYSQL_DISPLAY(delete_query);
    
    // Execute the query
    if ( !query_mem.execute(delete_query.c_str()) )
    {
      MYSQL_DISPLAY("Delete error");
    }
    else
    {
      MYSQL_DISPLAY("Data Deleted.");
    }
  }
  else
  {
    MYSQL_DISPLAY("Disconnected from Server. Can't delete.");
  }
}
