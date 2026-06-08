#include <Arduino.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "hubble/port/sys.h"

extern "C" {

  uint64_t hubble_uptime_get(void) {
    return millis();
  }

  int hubble_lock_init(void) {
    return 0;
  }

  void hubble_lock(void) {
  }

  void hubble_unlock(void) {
  }

  int hubble_log(enum hubble_log_level level,
                 const char *fmt,
                 ...) {
    char buffer[256];

    va_list args;

    va_start(args, fmt);

    vsnprintf(buffer,
              sizeof(buffer),
              fmt,
              args);

    va_end(args);

    Serial.println(buffer);

    return 0;
  }
}