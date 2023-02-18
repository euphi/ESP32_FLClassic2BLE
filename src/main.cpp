#include <Arduino.h>

//Wifi - not used, by WiFi.h needed to disable it
#include <WiFi.h>

// Bluetooth Classic
#include <BluetoothSerial.h>
BluetoothSerial SerialBT;
uint8_t FLClassicAddress[6] = { 0x20, 0x13, 0x01, 0x18, 0x02, 0x26 }; // Forumslader
//const BTAddress FLClassicAddress({ 0x20, 0x13, 0x01, 0x18, 0x02, 0x26 });

enum CONN_STATE {STATE_INIT, STATE_CONNECTING, STATE_CONNECTED, STATE_DISCONNECTED} ;
CONN_STATE cstate = STATE_INIT;

String bufferSerial;


// BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool deviceConnected_old = false;
//uint8_t txValue = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


// Scheduler (Ticker)
#include <Ticker.h>
Ticker reconnTicker;

void sendStringToBLE(const String bleStr) {  // pass by value
	uint16_t strStart=0;
    if (deviceConnected) {
    	do {
            uint16_t strEnd = strStart+20;		// Max default size for string in notification
            if (strEnd > bleStr.length()) strEnd = bleStr.length();
    		String subStr = bleStr.substring(strStart,strEnd);
            strStart+=20;
    		Serial.printf("Send sub-string \"%s\" to BLE by notify\n", subStr.c_str());
        	pTxCharacteristic->setValue((uint8_t*)subStr.c_str(), subStr.length());
            pTxCharacteristic->notify();
    		delay(5); // bluetooth stack will go into congestion, if too many packets are sent
    	} while (strStart < bleStr.length());
	}
}

void FLClassicReadFromSerial() {
	while (SerialBT.available()) {
		char read = SerialBT.read();
		if (read == '\n') {
			Serial.print("Read from FL Classic: ");
			Serial.println(bufferSerial);
			sendStringToBLE(bufferSerial);
			bufferSerial.clear();
		} else {
			bufferSerial += read;
		}
	}
}


class FLC2BLE_SCB: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class FLC2BLE_CharCB: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};


void createBLEServer() {
	  // Create the BLE Device
	  BLEDevice::init("UART Service ForumsLader");

	  // Create the BLE Server
	  pServer = BLEDevice::createServer();
	  pServer->setCallbacks(new FLC2BLE_SCB());

	  // Create the BLE Service
	  BLEService *pService = pServer->createService(SERVICE_UUID);

	  // Create a BLE Characteristic
	  pTxCharacteristic = pService->createCharacteristic(
											CHARACTERISTIC_UUID_TX,
											BLECharacteristic::PROPERTY_NOTIFY
										);

	  pTxCharacteristic->addDescriptor(new BLE2902());

	  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
												 CHARACTERISTIC_UUID_RX,
												BLECharacteristic::PROPERTY_WRITE
											);

	  pRxCharacteristic->setCallbacks(new FLC2BLE_CharCB());

	  // Start the service
	  pService->start();

	  // Start advertising
	  pServer->getAdvertising()->start();
	  Serial.println("Waiting a client connection to notify...");
}


void connectToFL() {

#ifdef BT_SCAN
  Serial.print("Starting discoverAsync...");
  if (SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
      Serial.println("Findings will be reported in \"btAdvertisedDeviceFound\"");
      delay(10000);
      Serial.print("Stopping discoverAsync... ");
      SerialBT.discoverAsyncStop();
      Serial.println("stopped");
  } else {
      Serial.println("Error on discoverAsync f.e. not working after a \"connect\"");
  }
#endif

	  SerialBT.begin("ESP32FL2BLE", true); //Bluetooth device name
	  //SerialBT.enableSSP();
	  SerialBT.setPin("1234");
	  //SerialBT.setPin("0000");
	  Serial.println("The device started in master mode, make sure remote BT device is on!");

	  SerialBT.setPin("1234");
	  bool connected = SerialBT.connect(FLClassicAddress);
	  if (connected) {
			cstate = STATE_CONNECTED;
			Serial.println("Forumslader Classic connected immediately");
	  } else {
		  Serial.println("Connecting ...");
		cstate = STATE_CONNECTING;
	    if (!SerialBT.connected(10000)) {
	      Serial.println("Failed to connect. Retry in 10 seconds");
	    } else {
	    	Serial.println("Forumslader Classic connected");
	    	cstate = STATE_CONNECTED;
	    }
	  }

}

void checkReconn() {
	Serial.print("ReConnCheck: ");
	if (cstate == STATE_DISCONNECTED) {
		Serial.println("❌ Reconnect necessary!");
	    if (!SerialBT.connected(10000)) {
	      Serial.println("Failed to connect. Retry in 10 seconds");
	    } else {
	    	cstate = STATE_CONNECTED;
	    }
	} else {
		Serial.println("✅");
	}

}

void setup() {
	Serial.begin(115200);
	Serial.println("Setup Serial");
	WiFi.mode(WIFI_MODE_NULL);
	WiFi.setSleep(WIFI_PS_MAX_MODEM);
	WiFi.setSleep(true);
	reconnTicker.attach_ms(10000, checkReconn);
	createBLEServer();
	connectToFL();
}


void loop() {
	delay(5);
	static char txValue[128];
	static uint8_t count=0;
	count++;

	if (cstate == STATE_CONNECTED) {
		FLClassicReadFromSerial();
	}

    if (!deviceConnected && deviceConnected_old) {
    	Serial.println("Lost device");
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        deviceConnected_old = deviceConnected;
    } else if (deviceConnected && !deviceConnected_old) {
		Serial.println("New device connected");
    	deviceConnected_old = deviceConnected;
    }
    if (deviceConnected) {
//    	uint16_t strSize = snprintf(txValue, 127, "Notify number %d at %ul!\n", count, millis());
//        pTxCharacteristic->setValue((uint8_t*)txValue, strSize);
//        pTxCharacteristic->notify();
//		delay(10); // bluetooth stack will go into congestion, if too many packets are sent
	}

}