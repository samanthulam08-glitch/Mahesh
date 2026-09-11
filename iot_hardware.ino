#include <WiFi.h>
#include <ArduinoOTA.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================

#define BUTTON_PIN 18

#define FAN_PIN    21
#define PUMP_PIN   22
#define LED_PIN    23

// ==========================================
// WIFI DETAILS FOR OTA
// ==========================================

const char* ssid = "bob";
const char* password = "123456789";

// ==========================================
// BLE UUIDs
// ==========================================

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ==========================================
// OPERATION
// ==========================================

int operation = 0;

// 0 = OFF
// 1 = FAN
// 2 = PUMP
// 3 = LED

// ==========================================
// BUTTON VARIABLES
// ==========================================

int lastButtonState = HIGH;
int currentButtonState = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// ==========================================
// FUNCTION: SET OPERATION
// ==========================================

void setOperation(int newOperation)
{
  // Make sure value is valid
  if (newOperation < 0 || newOperation > 3)
  {
    Serial.println("Invalid operation");
    return;
  }

  operation = newOperation;

  // First turn everything OFF
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // ========================================
  // OPERATION 0 - ALL OFF
  // ========================================

  if (operation == 0)
  {
    Serial.println("BLE/CONTROL: ALL OFF");
  }

  // ========================================
  // OPERATION 1 - FAN
  // ========================================

  else if (operation == 1)
  {
    digitalWrite(FAN_PIN, HIGH);

    Serial.println("BLE/CONTROL: FAN ON");
  }

  // ========================================
  // OPERATION 2 - PUMP
  // ========================================

  else if (operation == 2)
  {
    digitalWrite(PUMP_PIN, HIGH);

    Serial.println("BLE/CONTROL: WATER PUMP ON");
  }

  // ========================================
  // OPERATION 3 - LED
  // ========================================

  else if (operation == 3)
  {
    digitalWrite(LED_PIN, HIGH);

    Serial.println("BLE/CONTROL: LED ON");
  }
}

// ==========================================
// BLE CALLBACK
// ==========================================

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String value = pCharacteristic->getValue();

    if (value.length() == 0)
    {
      return;
    }

    Serial.print("BLE Received: ");
    Serial.println(value);

    // Remove spaces/new lines
    value.trim();

    // ======================================
    // BLE COMMANDS
    // ======================================

    if (value == "0")
    {
      setOperation(0);
    }

    else if (value == "1")
    {
      setOperation(1);
    }

    else if (value == "2")
    {
      setOperation(2);
    }

    else if (value == "3")
    {
      setOperation(3);
    }

    else if (value == "OFF" || value == "off")
    {
      setOperation(0);
    }

    else if (value == "FAN" || value == "fan")
    {
      setOperation(1);
    }

    else if (value == "PUMP" || value == "pump")
    {
      setOperation(2);
    }

    else if (value == "LED" || value == "led")
    {
      setOperation(3);
    }

    else
    {
      Serial.println("Unknown BLE command");
    }
  }
};

// ==========================================
// SETUP
// ==========================================

void setup()
{
  Serial.begin(115200);

  // ========================================
  // BUTTON
  // ========================================

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ========================================
  // OUTPUTS
  // ========================================

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Everything OFF initially
  setOperation(0);

  Serial.println();
  Serial.println("==============================");
  Serial.println("IOT SYSTEM STARTED");
  Serial.println("==============================");

  Serial.println("Button : GPIO 18");
  Serial.println("Fan    : GPIO 21");
  Serial.println("Pump   : GPIO 22");
  Serial.println("LED    : GPIO 23");

  // ========================================
  // BLE SETUP
  // ========================================

  Serial.println();
  Serial.println("Starting BLE...");

  BLEDevice::init("ESP32-IOT");

  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService =
    pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic =
    pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE
    );

  pCharacteristic->setCallbacks(new MyCallbacks());

  pCharacteristic->setValue("0");

  pService->start();

  BLEAdvertising *pAdvertising =
    BLEDevice::getAdvertising();

  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);

  BLEDevice::startAdvertising();

  Serial.println("BLE Started");
  Serial.println("BLE Name: ESP32-IOT");

  // ========================================
  // WIFI
  // ========================================

  Serial.println();
  Serial.println("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 10000)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  // ========================================
  // WIFI CONNECTED
  // ========================================

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi Connected");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // ======================================
    // OTA
    // ======================================

    ArduinoOTA.setHostname("ESP32-IOT");

    ArduinoOTA.onStart([]()
    {
      Serial.println("OTA Update Started");
    });

    ArduinoOTA.onEnd([]()
    {
      Serial.println("OTA Update Finished");
    });

    ArduinoOTA.onProgress([](unsigned int progress,
                             unsigned int total)
    {
      Serial.printf(
        "OTA Progress: %u%%\r",
        (progress * 100) / total
      );
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
      Serial.printf(
        "OTA Error: %u\n",
        error
      );
    });

    ArduinoOTA.begin();

    Serial.println("OTA Ready");
  }

  else
  {
    Serial.println("WiFi not connected");
    Serial.println("BLE and button control still work");
  }

  // ========================================
  // READY
  // ========================================

  Serial.println();
  Serial.println("==============================");
  Serial.println("READY");
  Serial.println("==============================");

  Serial.println();
  Serial.println("BLE COMMANDS:");
  Serial.println("0 = ALL OFF");
  Serial.println("1 = FAN");
  Serial.println("2 = PUMP");
  Serial.println("3 = LED");
}

// ==========================================
// LOOP
// ==========================================

void loop()
{
  // ========================================
  // OTA
  // ========================================

  if (WiFi.status() == WL_CONNECTED)
  {
    ArduinoOTA.handle();
  }

  // ========================================
  // PHYSICAL BUTTON
  // ========================================

  int reading = digitalRead(BUTTON_PIN);

  // Button state changed
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  // Debounce
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != currentButtonState)
    {
      currentButtonState = reading;

      // INPUT_PULLUP
      // LOW = button pressed

      if (currentButtonState == LOW)
      {
        operation++;

        if (operation > 3)
        {
          operation = 0;
        }

        setOperation(operation);
      }
    }
  }

  lastButtonState = reading;

  delay(10);
}