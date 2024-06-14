#include <HardwareSerial.h>
#include <XBee.h>

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

void setup() {
  Serial.begin(9600);
  XBeeSerial.begin(9600, SERIAL_8N1, 16, 17); // Configurar UART1 para pines 16 (RX) y 17 (TX)
  xbee.setSerial(XBeeSerial);
  Serial.println("Setup completed. Starting communication...");
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
  Serial.println("---------- END ----------");
  delay(10000);
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

