/**
 * A BLE client example that is rich in capabilities.
 */

#include <BLEDevice.h>
//#include "BLEScan.h"

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

uint8_t bothOff = 48; // correspond to 0
uint8_t bothOn = 49; // correspond to 1
uint8_t bothBlink = 50; // correspond to 2
uint8_t ledSyncState = bothOff; // Initializing to bothOff first

static BLEUUID serviceUUID("5775f66a-86c2-46fc-b4fa-1973e65ab36f");
static BLEUUID    charUUID("63ba0619-a2a7-43f4-acf0-d650e34cc6a5");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static boolean isRed = true;

static void notifyCallback(
	BLERemoteCharacteristic* pBLERemoteCharacteristic,
	uint8_t* pData,
	size_t length,
	bool isNotify) {
	Serial.print("data: ");
	Serial.println((char*)pData);
	if (*pData == '0') {
		Serial.println("Client has received a turn-off notification");
		ledState = LOW;
		digitalWrite(ledpin, LOW);
		count = 0;
		startCountdown = false;
		ledSyncState = bothOff;
	} else if (*pData == '1') {
		Serial.println("Client has received a turn-on notification");
		ledState = HIGH;
		digitalWrite(ledpin, HIGH);
		ledSyncState = bothOn;
	} else if (*pData == '2') {
		Serial.println("Client has received a blink notification");
		ledSyncState = bothBlink;
		Serial.println("And so it is blinking own LED before turning off");
		blink();
		ledSyncState = bothOff;
	}
}

class MyClientCallback : public BLEClientCallbacks {
	void onConnect(BLEClient* pclient) {
	}

	void onDisconnect(BLEClient* pclient) {
	connected = false;
	Serial.println("onDisconnect");
	}
};

bool connectToServer() {
	Serial.print("Forming a connection to ");
	Serial.println(myDevice->getAddress().toString().c_str());
	
	BLEClient*  pClient  = BLEDevice::createClient();
	Serial.println(" - Created client");

	pClient->setClientCallbacks(new MyClientCallback());

	// Connect to the remove BLE Server.
	pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
	Serial.println(" - Connected to server");

	// Obtain a reference to the service we are after in the remote BLE server.
	BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
	if (pRemoteService == nullptr) {
		Serial.print("Failed to find our service UUID: ");
		Serial.println(serviceUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our service");


	// Obtain a reference to the characteristic in the service of the remote BLE server.
	pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
	if (pRemoteCharacteristic == nullptr) {
		Serial.print("Failed to find our characteristic UUID: ");
		Serial.println(charUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our characteristic");

	// Read the value of the characteristic.
	if(pRemoteCharacteristic->canRead()) {
		std::string value = pRemoteCharacteristic->readValue();
		Serial.print("The characteristic value was: ");
		Serial.println(value.c_str());
	}

	if(pRemoteCharacteristic->canNotify())
		pRemoteCharacteristic->registerForNotify(notifyCallback);

	connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
	 * Called for each advertising BLE server.
	 */
	void onResult(BLEAdvertisedDevice advertisedDevice) {
	Serial.print("BLE Advertised Device found: ");
	Serial.println(advertisedDevice.toString().c_str());

	// We have found a device, let us now see if it contains the service we are looking for.
	if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

		BLEDevice::getScan()->stop();
		myDevice = new BLEAdvertisedDevice(advertisedDevice);
		doConnect = true;
		doScan = true;

	} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


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

void onButtonPress() {
	if (ledState == HIGH) {
		ledState = LOW;
		startCountdown = false;
		count = 0;
		Serial.println("Turning off own LED and resetting countdown");
		if (connected) {
			ledSyncState = bothOff;
			pRemoteCharacteristic->writeValue(ledSyncState);
			Serial.println("Client has sent bothOff write");
		} else {
			Serial.println("Disconnected and write not sent");
		}
	} else {
		ledState = HIGH;
		startCountdown = true;
		Serial.println("Turning on own LED and starting countdown");
		if (connected) {
			ledSyncState = bothOn;
			pRemoteCharacteristic->writeValue(ledSyncState);
			Serial.println("Client has sent bothOn write");
		} else {
			Serial.println("Disconnected and write not sent");
		}
	}
	digitalWrite(ledpin, ledState);
}


void setup() {
	Serial.begin(115200);
	pinMode(ledpin, OUTPUT);
	pinMode(button, INPUT);
	Serial.println("Starting Arduino BLE Client application...");
	BLEDevice::init("");

	// Retrieve a Scanner and set the callback we want to use to be informed when we
	// have detected a new device.  Specify that we want active scanning and start the
	// scan to run for 5 seconds.
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setInterval(1349);
	pBLEScan->setWindow(449);
	pBLEScan->setActiveScan(true);
	pBLEScan->start(5, false);

	digitalWrite(ledpin, ledState);
} // End of setup.


// This is the Arduino main loop function.
void loop() {
	buttonCurrentState = digitalRead(button);
	// If the flag "doConnect" is true then we have scanned for and found the desired
	// BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
	// connected we set the connected flag to be true.
	if (doConnect == true) {
		if (connectToServer()) {
			Serial.println("We are now connected to the BLE Server.");
		} else {
			Serial.println("We have failed to connect to the server; there is nothing more we will do.");
		}
		doConnect = false;
	}

	// If we are connected to a peer BLE Server:
	if (connected) {
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
				pRemoteCharacteristic->writeValue(ledSyncState);
				Serial.println("Client has sent bothBlink write");
				blink();
			}
		}
	} else if (doScan) {
		BLEDevice::getScan()->start(0);
	}
} // End of void loop
