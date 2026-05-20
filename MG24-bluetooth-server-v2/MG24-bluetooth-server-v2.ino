#define RF_SW_PW_PIN PB5
#define RF_SW_PIN PB4
#define LED PA7
bool notification_enabled = false;
long blueTimer= 0;
long ledTimer= 0;
int ledControl= 0;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LED_BUILTIN_INACTIVE);
  Serial.begin(115200);
  Serial.println("Silicon Labs BLE temperature server");

  // turn on the antenna function
  pinMode(RF_SW_PW_PIN, OUTPUT);
  digitalWrite(RF_SW_PW_PIN, HIGH);

  delay(100);

  // HIGH -> Use external antenna / LOW -> Use built-in antenna
  pinMode(RF_SW_PIN, OUTPUT);
  digitalWrite(RF_SW_PIN, LOW);
}

void loop() {
  if(millis() - blueTimer > 3000){
    blueTimer += 3000;
  
    if(notification_enabled) {
      send_temperature_notification();
    }
  }
  if(millis() - ledTimer > 300){
    ledTimer += 300;
    if(ledControl == 0){
      ledControl= 1;
      digitalWrite(LED, HIGH);
    }else{
      ledControl= 0;
      digitalWrite(LED, LOW);
    }
  }
}

static void ble_initialize_gatt_db();
static void ble_start_advertising();

static const uint8_t advertised_name[] = "XIAO_MG24 Server";
static uint16_t gattdb_session_id;
static uint16_t generic_access_service_handle;
static uint16_t name_characteristic_handle;
static uint16_t my_service_handle;
static uint16_t led_control_characteristic_handle;
static uint16_t notify_characteristic_handle;

/**************************************************************************//**
 * Bluetooth stack event handler
 * Called when an event happens on BLE the stack
 *
 * @param[in] evt Event coming from the Bluetooth stack
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt) {
  switch (SL_BT_MSG_ID(evt->header)) {

    case sl_bt_evt_system_boot_id:
      {
        Serial.println("BLE stack booted");
        ble_initialize_gatt_db();
        ble_start_advertising();
        Serial.println("BLE advertisement started");
      }
      break;

    case sl_bt_evt_connection_opened_id:
      Serial.println("BLE connection opened");
      break;

    case sl_bt_evt_connection_closed_id:
      Serial.println("BLE connection closed");
      ble_start_advertising();
      Serial.println("BLE advertisement restarted");
      break;

    case sl_bt_evt_gatt_server_attribute_value_id:
      if (led_control_characteristic_handle == evt->data.evt_gatt_server_attribute_value.attribute) {
        Serial.println("LED control characteristic data received");
        if (evt->data.evt_gatt_server_attribute_value.value.len == 0) {
          break;
        }
        uint8_t received_data = evt->data.evt_gatt_server_attribute_value.value.data[0];
        if (received_data == 0x00) {
          digitalWrite(LED_BUILTIN, LED_BUILTIN_INACTIVE);
          Serial.println("LED off");
        } else if (received_data == 0x01) {
          Serial.println("LED on");
          digitalWrite(LED_BUILTIN, LED_BUILTIN_ACTIVE);
        }
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.characteristic == notify_characteristic_handle) {
        if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_notification) {
          Serial.println("Notification enabled");
          notification_enabled = true;
        } else {
          Serial.println("Notification disabled");
          notification_enabled = false;
        }
      }
      break;

    default:
      break;
  }
}

/**************************************************************************//**
 * Reads the internal CPU temperature and sends it as a plain text BLE notification
 * getCPUTemp() docs: https://docs.silabs.com/bluetooth/latest/bluetooth-stack-api/
 * sl_bt_gatt_server_notify_all docs: https://docs.silabs.com/bluetooth/latest/bluetooth-stack-api/sl-bt-gatt-server
 *****************************************************************************/
static void send_temperature_notification() {
  float temperature = getCPUTemp();

  // Format as human-readable string, e.g. "27.50 C"
  char buf[16];
  snprintf(buf, sizeof(buf), "%.2f C", temperature);

  sl_status_t sc = sl_bt_gatt_server_notify_all(notify_characteristic_handle,
                                                strlen(buf),
                                                (const uint8_t *)buf);
  if (sc == SL_STATUS_OK) {
    Serial.print("Temperature sent: ");
    Serial.println(buf);
  }
}

/**************************************************************************//**
 * Starts BLE advertisement
 * Initializes advertising if it's called for the first time
 *****************************************************************************/
static void ble_start_advertising() {
  static uint8_t advertising_set_handle = 0xff;
  static bool init = true;
  sl_status_t sc;

  if (init) {
    sc = sl_bt_advertiser_create_set(&advertising_set_handle);
    app_assert_status(sc);

    // Set advertising interval to 100ms (value * 0.625ms)
    // https://docs.silabs.com/bluetooth/latest/bluetooth-stack-api/sl-bt-advertiser
    sc = sl_bt_advertiser_set_timing(
      advertising_set_handle,
      160,  // minimum advertisement interval (milliseconds * 1.6)
      160,  // maximum advertisement interval (milliseconds * 1.6)
      0,    // advertisement duration
      0);   // maximum number of advertisement events
    app_assert_status(sc);

    init = false;
  }

  sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle, sl_bt_advertiser_general_discoverable);
  app_assert_status(sc);

  sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);
  app_assert_status(sc);
}

/**************************************************************************//**
 * Initializes the GATT database
 * Creates a new GATT session and adds certain services and characteristics
 * https://docs.silabs.com/bluetooth/latest/bluetooth-stack-api/sl-bt-gattdb
 *****************************************************************************/
static void ble_initialize_gatt_db() {
  sl_status_t sc;

  sc = sl_bt_gattdb_new_session(&gattdb_session_id);
  app_assert_status(sc);

  // --- Generic Access service (UUID 0x1800) ---
  const uint8_t generic_access_service_uuid[] = { 0x00, 0x18 };
  sc = sl_bt_gattdb_add_service(gattdb_session_id,
                                sl_bt_gattdb_primary_service,
                                SL_BT_GATTDB_ADVERTISED_SERVICE,
                                sizeof(generic_access_service_uuid),
                                generic_access_service_uuid,
                                &generic_access_service_handle);
  app_assert_status(sc);

  // Device Name characteristic (UUID 0x2A00)
  const sl_bt_uuid_16_t device_name_characteristic_uuid = { .data = { 0x00, 0x2A } };
  sc = sl_bt_gattdb_add_uuid16_characteristic(gattdb_session_id,
                                              generic_access_service_handle,
                                              SL_BT_GATTDB_CHARACTERISTIC_READ,
                                              0x00,
                                              0x00,
                                              device_name_characteristic_uuid,
                                              sl_bt_gattdb_fixed_length_value,
                                              sizeof(advertised_name) - 1,
                                              sizeof(advertised_name) - 1,
                                              advertised_name,
                                              &name_characteristic_handle);
  app_assert_status(sc);

  sc = sl_bt_gattdb_start_service(gattdb_session_id, generic_access_service_handle);
  app_assert_status(sc);

  // --- Custom service (UUID: de8a5aac-a99b-c315-0c80-60d4cbb51224) ---
  const uuid_128 my_service_uuid = {
    .data = { 0x24, 0x12, 0xb5, 0xcb, 0xd4, 0x60, 0x80, 0x0c, 0x15, 0xc3, 0x9b, 0xa9, 0xac, 0x5a, 0x8a, 0xde }
  };
  sc = sl_bt_gattdb_add_service(gattdb_session_id,
                                sl_bt_gattdb_primary_service,
                                SL_BT_GATTDB_ADVERTISED_SERVICE,
                                sizeof(my_service_uuid),
                                my_service_uuid.data,
                                &my_service_handle);
  app_assert_status(sc);

  // LED Control characteristic (UUID: 5b026510-4088-c297-46d8-be6c736a087a)
  const uuid_128 led_control_characteristic_uuid = {
    .data = { 0x7a, 0x08, 0x6a, 0x73, 0x6c, 0xbe, 0xd8, 0x46, 0x97, 0xc2, 0x88, 0x40, 0x10, 0x65, 0x02, 0x5b }
  };
  uint8_t led_char_init_value = 0;
  sc = sl_bt_gattdb_add_uuid128_characteristic(gattdb_session_id,
                                               my_service_handle,
                                               SL_BT_GATTDB_CHARACTERISTIC_READ | SL_BT_GATTDB_CHARACTERISTIC_WRITE,
                                               0x00,
                                               0x00,
                                               led_control_characteristic_uuid,
                                               sl_bt_gattdb_fixed_length_value,
                                               1,                            // max length
                                               sizeof(led_char_init_value),  // initial value length
                                               &led_char_init_value,         // initial value
                                               &led_control_characteristic_handle);
  app_assert_status(sc);

  sc = sl_bt_gattdb_start_service(gattdb_session_id, my_service_handle);
  app_assert_status(sc);

  // Notify/Temperature characteristic (UUID: 61a885a4-41c3-60d0-9a53-6d652a70d29c)
  const uuid_128 btn_report_characteristic_uuid = {
    .data = { 0x9c, 0xd2, 0x70, 0x2a, 0x65, 0x6d, 0x53, 0x9a, 0xd0, 0x60, 0xc3, 0x41, 0xa4, 0x85, 0xa8, 0x61 }
  };
  uint8_t notify_char_init_value = 0;
  sc = sl_bt_gattdb_add_uuid128_characteristic(gattdb_session_id,
                                               my_service_handle,
                                               SL_BT_GATTDB_CHARACTERISTIC_READ | SL_BT_GATTDB_CHARACTERISTIC_NOTIFY,
                                               0x00,
                                               0x00,
                                               btn_report_characteristic_uuid,
                                               sl_bt_gattdb_fixed_length_value,
                                               16,                              // max length — fits "xx.xx C" text payload
                                               sizeof(notify_char_init_value),  // initial value length
                                               &notify_char_init_value,         // initial value
                                               &notify_characteristic_handle);
  app_assert_status(sc);

  sc = sl_bt_gattdb_start_service(gattdb_session_id, my_service_handle);
  app_assert_status(sc);

  sc = sl_bt_gattdb_commit(gattdb_session_id);
  app_assert_status(sc);
}
