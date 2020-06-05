    
/*
  Smart Thermostat

  The circuit:
  * Arduino Nano 33 BLE Sense

  Created 31 may 2020
  By Leonardo Cavagnis
  
*/

#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>
 
const int     ledPin              = LED_BUILTIN;
const String  MANUFACTURER_NAME   = "Arduino"; 
const String  MODEL_NUMBER        = "Nano 33 BLE Sense"; 

// create LED service
BLEService              ledService("277eaf48-6698-4da9-8329-335d05343490"); 
BLEByteCharacteristic   ledStatusCharacteristic("277eaf49-6698-4da9-8329-335d05343490", BLERead | BLEWrite);

// create DeviceInfo service
BLEService              deviceInformationService("180A"); 
BLEStringCharacteristic manufacturerNameCharacteristic("2A29", BLERead, sizeof(MANUFACTURER_NAME));
BLEStringCharacteristic modelNumberCharacteristic("2A24", BLERead, sizeof(MODEL_NUMBER));

// create EnvironmentalSensing service
BLEService              environmentalSensingService("181A");
BLEShortCharacteristic  temperatureCharacteristic("2A6E", BLERead | BLENotify);

unsigned long temperatureScanPeriod = 1000;
unsigned long timeNow = 0;

void setup() {
  // init serial
  Serial.begin(9600);
  while (!Serial);

  // init LED
  ledInit();

  // init temperature sensor
  if (!HTS.begin()) {
    Serial.println("Starting temperature sensor failed!");
    while (1);
  }
  
  // init BLE stack
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  // assign BLE event handlers
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  ledStatusCharacteristic.setEventHandler(BLEWritten, ledStatusCharacteristicWrittenHandler);
 
  // set peripheral local name
  BLE.setLocalName("SmartThermostat");
  // set the UUID for the service peripheral advertises
  BLE.setAdvertisedService(environmentalSensingService);
 
  // add characteristics to services
  environmentalSensingService.addCharacteristic(temperatureCharacteristic);
  BLE.addService(environmentalSensingService);
  
  ledService.addCharacteristic(ledStatusCharacteristic);
  BLE.addService(ledService);

  deviceInformationService.addCharacteristic(manufacturerNameCharacteristic);
  deviceInformationService.addCharacteristic(modelNumberCharacteristic);
  BLE.addService(deviceInformationService);

  // init read-only characteristics values
  manufacturerNameCharacteristic.writeValue(MANUFACTURER_NAME);
  modelNumberCharacteristic.writeValue(MODEL_NUMBER);

  // start advertising
  BLE.advertise();
 
  Serial.println("Bluetooth device active, waiting for central connections...");

  timeNow = millis();
}
 
void loop() {
  if (millis() >= (timeNow + temperatureScanPeriod)) {
    timeNow = millis();
    
    float temperature     = HTS.readTemperature();
    short temperatureBle  = temperature*100; //NOTE: unit is in degrees Celsius with a resolution of 0.01 degrees Celsius (according to BLE Specs)
    
    temperatureCharacteristic.writeValue(temperatureBle);
  }
  
  // Poll BLE events
  BLE.poll();
}

void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

void ledStatusCharacteristicWrittenHandler(BLEDevice central, BLECharacteristic characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print("ledStatus characteristic event, written: ");

  if (ledStatusCharacteristic.value()) {
    Serial.println("Led On");
    ledOn();
  } else {
    Serial.println("Led Off");
    ledOff();
  }
}

void ledInit(){
  pinMode(ledPin, OUTPUT);
  ledOff();
}

void ledOn(){
  digitalWrite(ledPin, HIGH);
}

void ledOff(){
  digitalWrite(ledPin, LOW);
}

