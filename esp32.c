// This is a mash-up of the Due show() code + insights from Michael Miller's
// ESP8266 work for the NeoPixelBus library: github.com/Makuna/NeoPixelBus
// Needs to be a separate .c file to enforce ICACHE_RAM_ATTR execution.

#if defined(ESP32)

#include <Arduino.h>

// esync from https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Esp.cpp
static uint32_t _getCycleCount(void) __attribute__((always_inline));
static inline uint32_t _getCycleCount(void) {
  uint32_t ccount;
  __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));
  return ccount;
}

void espShow(
  uint8_t pin, uint8_t *pixels, uint32_t numBytes, boolean is800KHz) {

// 12.500ns/cycle @  80MHz
//  4.167ns/cycle @ 240MHz
#define CYCLES_RESET    (F_CPU /  142800) // 2500000 = 7.0us
#define CYCLES_800_T0H  (F_CPU / 2500000) // 2500000 = 0.4us
#define CYCLES_800_T1H  (F_CPU / 1250000) // 1250000 = 0.8us
#define CYCLES_800      (F_CPU /  800000) //  800000 = 1.25us per bit
#define CYCLES_400_T0H  (F_CPU / 2000000) // 2000000 = 0.5uS
#define CYCLES_400_T1H  (F_CPU /  833333) //  833333 = 1.2us
#define CYCLES_400      (F_CPU /  400000) //  400000 = 2.5us per bit

  uint8_t *p, *end, pix, mask;
  uint32_t t, time0, time1, period, c, startTime, pinMask;

  pinMask   = _BV(pin);
  p         =  pixels;
  end       =  p + numBytes;
  pix       = *p++;
  mask      = 0x80;
  startTime = 0;

#ifdef NEO_KHZ400
  if(is800KHz) {
#endif
    time0  = CYCLES_800_T0H;
    time1  = CYCLES_800_T1H;
    period = CYCLES_800;
#ifdef NEO_KHZ400
  } else { // 400 KHz bitstream
    time0  = CYCLES_400_T0H;
    time1  = CYCLES_400_T1H;
    period = CYCLES_400;
  }
#endif

#ifdef TRY1
// Glitchy
  for (t = time0;; t = time0) {
    if (pix & mask) t = time1;                             // Bit high duration
    while (((c = _getCycleCount()) - startTime) < period); // Wait for bit start
    gpio_set_level(pin, HIGH);
    startTime = c;                                         // Save start time
    while (((c = _getCycleCount()) - startTime) < t);      // Wait high duration
    gpio_set_level(pin, LOW);
    if (!(mask >>= 1)) {                                   // Next bit/byte
      if (p >= end) break;
      pix  = *p++;
      mask = 0x80;
    }
  }
  while((_getCycleCount() - startTime) < period); // Wait for last bit
#endif

#define TRY2
#ifdef TRY2
// Derived from
// https://kbeckmann.github.io/2015/07/25/Bit-banging-two-pins-simultaneously-on-ESP8266/
// glitchy only on first pass thru loop. Weird.
  startTime = _getCycleCount() - period;
  while (true) {
    while (((c = _getCycleCount()) - startTime) < period); // Wait for bit start
    gpio_set_level(pin, HIGH);
    startTime = c;                                         // Save start time
    t = time0;
    if (pix & mask) t = time1;                             // Bit high duration
    while (((c = _getCycleCount()) - startTime) < t);      // Wait high duration
    gpio_set_level(pin, LOW);
    if (!(mask >>= 1)) {                                   // Next bit/byte
      if (p >= end) break;
      pix  = *p++;
      mask = 0x80;
    }
  }
  while (((c = _getCycleCount()) - startTime) < period); // Wait for last bit
#endif

//#define TRY3
#ifdef TRY3
// Derived from
// https://github.com/Makuna/NeoPixelBus/blob/master/src/internal/NeoPixelEsp8266.c
// https://kbeckmann.github.io/2015/07/25/Bit-banging-two-pins-simultaneously-on-ESP8266/
// https://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
  startTime = _getCycleCount();
  while (((c = _getCycleCount()) - startTime) < CYCLES_RESET*200); // VERY glitchy!
  startTime = _getCycleCount() - period;
  while (true) {
    while (((c = _getCycleCount()) - startTime) < period); // Wait for bit start
    gpio_set_level(pin, HIGH);
    startTime = c;                                         // Save start time
    t = time0;
    if (pix & mask) t = time1;                             // Bit high duration
    while (((c = _getCycleCount()) - startTime) < t);      // Wait high duration
    gpio_set_level(pin, LOW);
    if (!(mask >>= 1)) {                                   // Next bit/byte
      if (p >= end) break;
      pix  = *p++;
      mask = 0x80;
    }
  }
  while (((c = _getCycleCount()) - startTime) < period); // Wait for last bit
#endif
}

#endif // ESP32
