#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

int ledpin = 4;
int button = 5;
int ledState = LOW;
int buttonCurrentState = 1;
int buttonPreviousState = 1;
long timeElapsed = 0;
long debounce = 100;
// For our button, 0 means pressed and 1 means released!

bool startCountdown = false;
long count = 0;
long countMax = 8000;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

uint8_t bothOff = 48; // correspond to 0
uint8_t bothOn = 49; // correspond to 1
uint8_t bothBlink = 50; // correspond to 2
uint8_t ledSyncState = bothOff; // Initializing to bothOff first

// Store the characteristic that is written from the client
std::string characteristicValue;

#define SERVICE_UUID        "5775f66a-86c2-46fc-b4fa-1973e65ab36f"
#define CHARACTERISTIC_UUID "63ba0619-a2a7-43f4-acf0-d650e34cc6a5"

class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		deviceConnected = true;
	};

	void onDisconnect(BLEServer* pServer) {
		deviceConnected = false;
	}
};

void blink() {
	Serial.println("Starting to blink own LED");
	ledState = HIGH;
	digitalWrite(ledpin, HIGH);
	delay(500);
	digitalWrite(ledpin, LOW);
	delay(500);
	digitalWrite(ledpin, HIGH);
	delay(500);
	digitalWrite(ledpin, LOW);
	delay(500);
	digitalWrite(ledpin, HIGH);
	delay(500);
	digitalWrite(ledpin, LOW);
	delay(500);
	digitalWrite(ledpin, HIGH);
	delay(500);
	digitalWrite(ledpin, LOW);
	ledState = LOW;
	Serial.println("Blinking complete, turning own LED off");
}

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic* pCharacteristic) {
		characteristicValue = pCharacteristic->getValue();
		if (characteristicValue == "0") {
			Serial.println("Server has received a turn-off write");
			ledState = LOW;
			digitalWrite(ledpin, LOW);
			count = 0;
			startCountdown = false;
			ledSyncState = bothOff;
			Serial.println("And so it is turning own LED off");
		} else if (characteristicValue == "1") {
			Serial.println("Server has received a turn-on write");
			ledState = HIGH;
			digitalWrite(ledpin, HIGH);
			ledSyncState = bothOn;
			Serial.println("And so it is turning own LED on");
		} else if (characteristicValue == "2") {
			Serial.println("Server has received a blink write");
			ledSyncState = bothBlink;
			Serial.println("And so it is blinking own LED before turning off");
			blink();
			ledSyncState = bothOff;
		}
	}
};

void onButtonPress() {
	if (ledState == HIGH) {
		ledState = LOW;
		startCountdown = false;
		count = 0;
		Serial.println("Turning off own LED and resetting countdown");
		if (deviceConnected) {
			ledSyncState = bothOff;
			pCharacteristic->setValue((uint8_t*)&ledSyncState, 4);
			pCharacteristic->notify();
			Serial.println("Server has sent bothOff notification");
		} else {
			Serial.println("Disconnected and notification not sent");
		}
	} else {
		ledState = HIGH;
		startCountdown = true;
		Serial.println("Turning on own LED and starting countdown");
		if (deviceConnected) {
			ledSyncState = bothOn;
			pCharacteristic->setValue((uint8_t*)&ledSyncState, 4);
			pCharacteristic->notify();
			Serial.println("Server has sent bothOn notification");
		} else {
			Serial.println("Disconnected and notification not sent");
		}
	}
	digitalWrite(ledpin, ledState);
}

void setup() {
	pinMode(ledpin, OUTPUT);
	pinMode(button, INPUT);
	Serial.begin(115200);
	
	// Create the BLE Device
	BLEDevice::init("ESP32");

	// Create the BLE Server
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pService = pServer->createService(SERVICE_UUID);

	// Create a BLE Characteristic
	pCharacteristic = pService->createCharacteristic(
						CHARACTERISTIC_UUID,
						BLECharacteristic::PROPERTY_READ   |
						BLECharacteristic::PROPERTY_WRITE  |
						BLECharacteristic::PROPERTY_NOTIFY |
						BLECharacteristic::PROPERTY_INDICATE
					);

	// Create the Characteristic Callback
	pCharacteristic->setCallbacks(new MyCharacteristicCallbacks);

	// https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
	// Create a BLE Descriptor
	pCharacteristic->addDescriptor(new BLE2902());

	// Start the service
	pService->start();

	// Start advertising
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();
	Serial.println("Waiting a client connection to notify...");

	// Set the initial value of the characteristic to send in notification
	pCharacteristic->setValue((uint8_t*)&bothOff, 4);
}

void loop() {
	buttonCurrentState = digitalRead(button);
	if (buttonCurrentState == 0 && 
		buttonPreviousState == 1 && 
		millis() - timeElapsed > debounce) {
		onButtonPress();
		timeElapsed = millis();
	}
	buttonPreviousState = buttonCurrentState;
	if (startCountdown == true) {
		Serial.println(count);
		count++;
		if (count >= countMax) {
			count = 0;
			startCountdown = false;
			ledSyncState = bothBlink;
			pCharacteristic->setValue((uint8_t*)&ledSyncState, 4);
			pCharacteristic->notify();
			Serial.println("Server has sent bothBlink notification");
			blink();
		}
	}
	
	// disconnecting
	if (!deviceConnected && oldDeviceConnected) {
		delay(500); // give the bluetooth stack the chance to get things ready
		pServer->startAdvertising(); // restart advertising
		Serial.println("start advertising");
		oldDeviceConnected = deviceConnected;
	}
	// connecting
	if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
		oldDeviceConnected = deviceConnected;
	}
}