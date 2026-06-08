//LED blink
#define LED GPIO_NUM_8
uint32_t led_timer = millis();
uint16_t led_trigger = 0;
bool led_on = LOW;

void led_blink() {
  if ((millis() - led_trigger >= led_timer) or (millis() < led_timer)) {
    if (led_on == LOW) led_trigger = 4975;
    else led_trigger = 25;

    led_timer = millis();
    led_on = !led_on;

    pinMode(LED, OUTPUT);
    digitalWrite(LED, led_on);
  }
}


bool decodeKey(const char *key_b64,
                     size_t expected_size) {
  size_t out_len;

  int ret =
    mbedtls_base64_decode(
      hubble_key,
      sizeof(hubble_key),
      &out_len,
      (const unsigned char *)key_b64,
      strlen(key_b64));

  Serial.printf(
    "decode=%d len=%u\n",
    ret,
    (unsigned)out_len);

  return (ret == 0) && (out_len == expected_size);
}
