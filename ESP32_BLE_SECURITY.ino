
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>


BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
#define SERVICE_UUID        "8c30f045-683a-4777-8d21-87def63e4ef5"
#define CHARACTERISTIC_UUID "e6eae575-4d89-4750-bf3e-c82d6a1cd299"
#define PASSKEY 338027

#define LED_BUILTIN 0


void setup() {
  Serial.begin(115200);

  pinMode (LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  bleInit();
}

void loop() {
  Serial.println(digitalRead(LED_BUILTIN));
  delay(1000);
}

/////////////////////
//BLE Secure Server//
/////////////////////

class ServerCallback: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println(" - ServerCallback - onConnect");
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println(" - ServerCallback - onDisconnect");
      digitalWrite(LED_BUILTIN, LOW);
    }
};

class SecurityCallback : public BLESecurityCallbacks {

    uint32_t onPassKeyRequest() {
      return 000000;
    }

    void onPassKeyNotify(uint32_t pass_key) {}

    bool onConfirmPIN(uint32_t pass_key) {
      vTaskDelay(5000);
      return true;
    }

    bool onSecurityRequest() {
      return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
      if (cmpl.success) {
        Serial.println("   - SecurityCallback - Authentication Success");
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        Serial.println("   - SecurityCallback - Authentication Failure*");
        pServer->removePeerDevice(pServer->getConnId(), true);
      }
      BLEDevice::startAdvertising();
    }
};

void bleSecurity() {
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
  esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;
  uint8_t key_size = 16;
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint32_t passkey = PASSKEY;
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

void bleInit() {
  BLEDevice::init("BLE-Secure-Server");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new SecurityCallback());

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallback());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

  bleSecurity();
}