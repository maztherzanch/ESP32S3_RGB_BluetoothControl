#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Adafruit_NeoPixel.h>

//Define los UUID de servicio y caracteristica necesarios como credenciales
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-1234567890ab"

//Inicializa el led integrado en el pin48 como un neo pixel para mejor y más simple control
#define RGB_LED_PIN 48
#define NUMPIXELS 1
Adafruit_NeoPixel rgbLed(NUMPIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

//Establece parametros para la coneccion con BLE
BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;

//Setea valores iniciales
float redValue = 0.0;
float greenValue = 0.0;
float blueValue = 0.0;
float brightValue = 125.0;

//Clase que establece los cambios de color
void actualizarLED() {
  float factorBrillo = constrain(brightValue, 0.0, 255.0) / 255.0;

  uint8_t r = (uint8_t)constrain(redValue * factorBrillo, 0.0, 255.0);
  uint8_t g = (uint8_t)constrain(greenValue * factorBrillo, 0.0, 255.0);
  uint8_t b = (uint8_t)constrain(blueValue * factorBrillo, 0.0, 255.0);

  rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
  rgbLed.show();
  //Muestra los valores obtenidos en el monitor serial
  Serial.println("Valores actuales:");
  Serial.print("R = "); Serial.println(redValue, 2);
  Serial.print("G = "); Serial.println(greenValue, 2);
  Serial.print("B = "); Serial.println(blueValue, 2);
  Serial.print("Br = "); Serial.println(brightValue, 2);
  Serial.print("R aplicado = "); Serial.println(r);
  Serial.print("G aplicado = "); Serial.println(g);
  Serial.print("B aplicado = "); Serial.println(b);
  Serial.println("---------------------");
}

//Establece la coneccion con el servidor BLE (Smartphone)
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("Celular conectado por BLE");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("Celular desconectado");
  }
};

//Revisa si los datos obtenidos por BLE coinciden con el formato valido para procesado
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String data = pCharacteristic->getValue();
    if (data.length() == 0) return;

    data.trim();

    Serial.print("Dato recibido por BLE: ");
    Serial.println(data);

    int separador = data.indexOf(':');
    if (separador == -1) {
      Serial.println("Formato invalido. Usa R:valor, G:valor, B:valor o Br:valor");
      return;
    }
    //Analiza los fatos obtenidos para aislar el valor numerico
    String canal = data.substring(0, separador);
    canal.trim();

    String textoValor = data.substring(separador + 1);
    textoValor.trim();

    float valor = textoValor.toFloat();
    valor = constrain(valor, 0.0, 255.0);
    //Asigna los valores aislados para actualizazr los valores usados por la libreria
    //Adafruit para hacer los cambios de color
    if (canal.equalsIgnoreCase("R")) {
      redValue = valor;
    } else if (canal.equalsIgnoreCase("G")) {
      greenValue = valor;
    } else if (canal.equalsIgnoreCase("B")) {
      blueValue = valor;
    } else if (canal.equalsIgnoreCase("Br")) {
      brightValue = valor;
    } else {
      Serial.println("Canal invalido. Usa R, G, B o Br");
      return;
    }

    actualizarLED();
  }
};

void setup() {
  //Inicializa el puerto serial para entrada y salida de información
  Serial.begin(115200);
  delay(1000);
  //Inicializa las funciones del LED RGB por la libreria Neopixel
  rgbLed.begin();
  rgbLed.clear();
  rgbLed.show();

  //Inicializa el nombre del Bluetoot a mostrar
  BLEDevice::init("ESP32S3_RGB_BLE");

  //Crea el servidor de comuinicacion basado en los UUID asignados
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );

  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  //Realiza un muestreo en el monitor serial como control
  Serial.println("ESP32-S3 lista y esperando conexion BLE");
  Serial.println("Formato esperado:");
  Serial.println("R:120");
  Serial.println("G:80");
  Serial.println("B:255");
  Serial.println("Br:128");
}

//Lectura continua del canal para actualizaciones en tiempo real
void loop() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(300);
    pServer->startAdvertising();
    Serial.println("Publicidad BLE reiniciada, esperando reconexion...");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(20);
}
