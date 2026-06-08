#include <Arduino.h>

#include "hw.h"
#include <NimBLEDevice.h>
#include "hubble/hubble.h"
#include "hubble/ble.h"

//decode base64
#include <mbedtls/base64.h>

uint8_t hubble_key[32];
size_t key128 = 16;
size_t key256 = 32;

uint8_t h_data[13] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00}; //"Hello World!"

void setup() {
  Serial.begin(115200);
  delay(1000);

  hubbleadv(h_data);
}

void loop() {
  led_blink();
}

