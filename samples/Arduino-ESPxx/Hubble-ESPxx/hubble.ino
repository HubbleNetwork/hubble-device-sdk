bool hubbleadv(uint8_t sensor_data[13]) {
  bool res = false;

  decodeKey(HUBBLE_KEY, key256);
  uint64_t uptime_sec = esp_timer_get_time() / 1000000ULL;
  int ret = hubble_init(uptime_sec, hubble_key);

  Serial.printf("hubble_init=%d\r\n", ret);

  if (ret != 0) Serial.println("Hubble init failed");
  else res = true;

  if (res == true) {
    uint8_t adv_data[25];

    size_t adv_len = sizeof(adv_data);

    Serial.printf("sizeof(h_data)=%u\n", sizeof(h_data));

    ret = hubble_ble_advertise_get(h_data, sizeof(h_data), adv_data, &adv_len);

    Serial.printf("hubble_advertise=%d\r\n", ret);
    Serial.printf("adv_len=%u\r\n", (unsigned)adv_len);
    Serial.println();

    //debug
    /*
    for (size_t i = 0; i < adv_len; i++) {
      Serial.printf("%02X ", adv_data[i]);
    }
    Serial.println();
    */
    
    //1st Inits NimBLE
    NimBLEDevice::init("Hubble x SGA's Example");

    // 1. Get the global advertising pointer
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();

    // 2. Instantiate the container object
    NimBLEAdvertisementData advData;

    // 3. Populate data fields
    // Note: remove 2 bytes from left not UUID FCA6
    std::string serviceData((char *)&adv_data[2], adv_len - 2);
    //std::string serviceData((char *)&adv_data, adv_len);
    //advData.setName("SGA IoT Device"); //This does not work with Hubble
    //advData.setManufacturerData("BLE Test"); //This does not work with Hubble
    advData.setServiceData(NimBLEUUID((uint16_t)0xFCA6), serviceData);

    // 4. Assign the container payload to the advertisement engine
    adv->setAdvertisementData(advData);

    // 5. Set min/max intervals. note 1ms = 0.625unit. This is only an example
    adv->setMinInterval(1600);
    adv->setMaxInterval(8000);

    // 6. Start broadcasting
    adv->start();

    Serial.println("Advertising Hubble");
  }

  return res;
}
