# ESP32-LED-Sync
 Using BLE on two ESP32 Boards to synchronize LEDs

## Introduction
Two ESP32 boards are each connected with a push button and an LED. When one board’s button is pressed, its own LED as well as the other’s LED would light up. Pressing the button again turns off both LEDs. If the LEDs are lit up for long enough, they automatically turn off before blinking four times.

## BLE Server
One board serves as the BLE server, with an arbitrarily defined `Service UUID` and an `Characteristic UUID`. Once it is connected to by the client, it can set values of its Characteristic, and send the client this Characteristic using `notify()`.

To use the BLE notifications feature, we need to add a Descriptor to our server stating that it should be capable of sending notifications using `addDescriptor (new BLE2902())`.

After the board’s own LED is turned on, we start a counter whose ending will trigger both its own LED blinking and another notification being sent to the client, asking it to blink. Having the initiating party to control the blinking can help reduce latency issues.

The server receives data from the client using the `BLECharacteristicCallbacks` class. Every time the client writes into the server’s Characteristic, the LED will change depending on what value is being written.

## BLE Client
Another board serves as the BLE client using the exact `Service UUID	` and `Characteristic UUID`. Once it is paired up with the server, it can write values directly to the server’s characteristic using `writeValue()`.

When the server receives notifications, it can trigger LEDs with the `notifyCallback` class.

## Credits
Based on Neil Kolban’s ESP32 BLE examples ([https://github.com/nkolban/ESP32\_BLE\_Arduino/tree/master/examples][1]).

[1]:	https://github.com/nkolban/ESP32_BLE_Arduino/tree/master/examples