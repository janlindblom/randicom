// Copyright (c) 2023 Jan Lindblom <jan@namnlos.co>

// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

#include "display.h"
#include "network.h"
#include "randi.h"
#include "util.h"

// #include <Adafruit_I2CDevice.h>
#include <Adafruit_SSD1327.h>

Adafruit_NeoPixel strip   = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB);
Adafruit_SSD1327  monitor = Adafruit_SSD1327(128, 128, &Wire, RESET_PIN, 100000u, 100000u);
Adafruit_SSD1327  console = Adafruit_SSD1327(128, 128, &Wire1, RESET_PIN, 100000u, 100000u);

// U8G2_SSD1327_WS_128X128_F_SW_I2C console(U8G2_R0, /* reset=*/U8X8_PIN_NONE,
// /* clock=*/27, /* data=*/26); U8G2_SSD1327_WS_128X128_F_SW_I2C
// monitor(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/5, /* data=*/4);
// U8G2_SSD1327_WS_128X128_F_HW_I2C console(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SSD1327_WS_128X128_F_2ND_HW_I2C console(U8G2_R0, U8X8_PIN_NONE);

// U8G2_SSD1327_WS_128X128_F_2ND_HW_I2C monitor(U8G2_R0, /*
// reset=*/U8X8_PIN_NONE); U8G2_SSD1327_WS_128X128_F_2ND_HW_I2C monitor(U8G2_R0,
// U8X8_PIN_NONE);

// U8G2_SSD1327_MIDAS_128X128_F_HW_I2C     console(U8G2_R0, /*
// reset=*/U8X8_PIN_NONE); // , /* clock=*/5, /* data=*/4);
// U8G2_SSD1327_MIDAS_128X128_F_2ND_HW_I2C monitor(U8G2_R0, /*
// reset=*/U8X8_PIN_NONE); //, /* clock=*/27, /* data=*/26);

volatile uint_fast16_t hue      = 0;
volatile uint32_t      colors[] = {black, black, black, black, black, black, black, black};

extern volatile bool wifi_setup_running;
extern volatile bool wifi_configured;
extern volatile bool wifi_connected;
extern volatile bool wifi_ap_configured;
extern volatile bool ntp_setup_running;
extern volatile bool ntp_configured;
volatile bool        neopixel_setup_running = false;
volatile bool        neopixel_configured    = false;
extern volatile bool display_configured;

volatile bool first_boot = true;

extern const uint_fast8_t wifi_led;
extern const uint_fast8_t ntp_led;
const uint_fast8_t        display_led = 5;

#define boot_complete (wifi_configured && wifi_ap_configured && ntp_configured && neopixel_configured && display_configured)

void status_led_update(void *param) {
    if (ntp_configured) {
        colors[ntp_led] = col_good;
    } else {
        colors[ntp_led] = col_bad;
    }
    if (wifi_configured && wifi_connected) {
        colors[wifi_led] = col_good;
    } else {
        colors[wifi_led] = col_bad;
    }
    if (display_configured) {
        colors[display_led] = col_good;
    } else {
        colors[display_led] = col_bad;
    }
}

/**
 * @brief Main startup sequence.
 */
void setup() {
    if (!Serial) {
        Serial.begin(115200);
    }
    display_setup(NULL);
    neopixel_setup(NULL);
    wifi_setup(NULL);
    ntp_setup(NULL);
}

/**
 * @brief Secondary startup sequence.
 *
 */
void setup1() {
    delay(300);
}

unsigned long t = 0;

/**
 * @brief Fast main loop.
 */
void loop() {
    delay(200);
    status_led_update(NULL);
    if (neopixel_configured) {
        neopixel_update(NULL);
    }

    monitor.clearDisplay();
    console.clearDisplay();
    console.setCursor(0, 10);
    console.print("WiFi: ");
    if (wifi_connected) {
        console.print(WiFi.SSID());
    } else {
        console.print("none");
    }
    console.display();
    monitor.display();
}

/**
 *  @brief Slow main loop.
 */
void loop1() {
    delay(5000);
    if (!Serial) {
        Serial.begin(9600);
    }
    if (wifi_configured) {
        wifi_check_status(NULL);
        if (!wifi_connected) {
            wifi_setup(NULL);
        }
        if (!ntp_configured) {
            ntp_setup(NULL);
        }
    } else {
        wifi_setup(NULL);
    }

    if (first_boot && boot_complete) {
        first_boot = false;
    }
}

void neopixel_setup(void *param) {
    (void)param;
    if (neopixel_setup_running) {
        return;
    }
    neopixel_setup_running = true;

    strip.begin();
    strip.setBrightness(20);
    neopixel_configured = true;
    strip.show();
    neopixel_setup_running = false;
}

void neopixel_update(void *param) {
    for (uint_fast8_t i = 0; i < NUM_LEDS; i++) {
        if (hue >= 65503) {
            hue = 0;
        }
        if (i != wifi_led && i != ntp_led && i != display_led) {
            uint_fast32_t color = strip.ColorHSV(hue, 255, 255);
            colors[i]           = color;
        }
        strip.setPixelColor(i, colors[i]);
        hue += 32;
        strip.show();
    }
}
