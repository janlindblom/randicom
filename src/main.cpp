// Copyright (c) 2022 Jan Lindblom <jan@namnlos.co>

// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include "randi.h"

#include <Wire.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <WiFiNTP.h>

#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

#include <Adafruit_NeoPixel.h>
//#include <Adafruit_I2CDevice.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SSD1327.h>
#include <splash.h>

Adafruit_NeoPixel strip   = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB);
Adafruit_SSD1327  monitor = Adafruit_SSD1327(128, 128, &Wire, RESET_PIN, 100000u, 100000u);
Adafruit_SSD1327  console = Adafruit_SSD1327(128, 128, &Wire1, RESET_PIN, 100000u, 100000u);

// U8G2_SSD1327_WS_128X128_F_SW_I2C console(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/27, /* data=*/26);
// U8G2_SSD1327_WS_128X128_F_SW_I2C monitor(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/5, /* data=*/4);
// U8G2_SSD1327_WS_128X128_F_HW_I2C console(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SSD1327_WS_128X128_F_2ND_HW_I2C console(U8G2_R0, U8X8_PIN_NONE);

// U8G2_SSD1327_WS_128X128_F_2ND_HW_I2C monitor(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SSD1327_WS_128X128_F_2ND_HW_I2C monitor(U8G2_R0, U8X8_PIN_NONE);

// U8G2_SSD1327_MIDAS_128X128_F_HW_I2C     console(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // , /* clock=*/5, /* data=*/4);
// U8G2_SSD1327_MIDAS_128X128_F_2ND_HW_I2C monitor(U8G2_R0, /* reset=*/U8X8_PIN_NONE); //, /* clock=*/27, /* data=*/26);

volatile uint_fast16_t hue      = 0;
volatile uint32_t      colors[] = {black, black, black, black, black, black, black, black};

volatile bool wifi_setup_running     = false;
volatile bool wifi_configured        = false;
volatile bool wifi_connected         = false;
volatile bool wifi_ap_configured     = false;
volatile bool ntp_setup_running      = false;
volatile bool ntp_configured         = false;
volatile bool neopixel_setup_running = false;
volatile bool neopixel_configured    = false;
volatile bool console_configured     = false;
volatile bool monitor_configured     = false;
volatile bool display_configured     = false;
volatile bool first_boot             = true;

const uint_fast8_t wifi_led    = 7;
const uint_fast8_t ntp_led     = 6;
const uint_fast8_t display_led = 5;

const char* ssid     = STASSID;
const char* password = STAPSK;
#ifdef SECSSID
const char* ssid2     = SECSSID;
const char* password2 = SECPSK;
#endif
WiFiMulti multi;

#define boot_complete (wifi_configured && wifi_ap_configured && ntp_configured && neopixel_configured && display_configured)

void display_setup(void* param) {
    if (Serial) {
        Serial.println("Setting up display...");
    }

    console.begin(OLED_ADDR);
    monitor.begin(OLED_ADDR);

    console.setTextSize(1);
    monitor.setTextSize(1);
    //  console.setFont(&DisplayFont);
    //  monitor.setFont(&DisplayFont);
    console.setTextColor(SSD1327_WHITE, SSD1327_BLACK);
    monitor.setTextColor(SSD1327_WHITE, SSD1327_BLACK);

    console.clearDisplay();
    monitor.clearDisplay();

    console.drawBitmap(monitor.width() / 2 - splash2_width / 2, monitor.height() / 2 - splash2_height / 2, splash2_data, splash2_width, splash2_height, SSD1327_WHITE);
    monitor.drawBitmap(monitor.width() / 2 - splash2_width / 2, monitor.height() / 2 - splash2_height / 2, splash2_data, splash2_width, splash2_height, SSD1327_WHITE);
    console.display();
    monitor.display();

    delay(150);
    console.clearDisplay();
    monitor.clearDisplay();
    console.display();
    monitor.display();

    // con.setBusClock(400000);
    /*

    mon.begin();
    mon.setContrast(160);
    */

    // console_configured = console.begin();
    //   console.clearBuffer();
    //   console.setContrast(160);
    /*
        console.setFont(u8g2_font_5x8_mf);
        // console.setFont(u8g2_font_helvR08_tf); // set the font for the terminal window
        u8g2log.begin(U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
        u8g2log.setLineHeightOffset(1); // set extra space between lines in pixel, this can be negative
        u8g2log.setRedrawMode(0);       // 0: Update screen with newline, 1: Update screen for every char

        console.print("Console initialised.\n");
        // monitor_configured = monitor.begin();
        // console.print("Monitor initialised.\n");
        console.print("Displays initialised.\n");
        */
    display_configured = true;
}

void status_led_update(void* param) {
    // WiFi connectivity is controlledc by its' own subroutine.
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
        // console.print("System Initialised.\n");
        first_boot = false;
    }
}

void neopixel_setup(void* param) {
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

void neopixel_update(void* param) {
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

void wifi_check_status(void* param) {
    (void)param;
    colors[wifi_led] = col_working;

    if (WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED) {
        // console.print("[N] WiFi connection lost...\n");
        wifi_connected   = false;
        colors[wifi_led] = col_waiting;
    }

    if (multi.run() != WL_CONNECTED) {
        // console.print("[N] Unable to connect to network.\n");
        colors[wifi_led] = col_bad;
        wifi_connected   = false;
        // wifi_setup(NULL);
    } else {
        wifi_connected = true;
        int ping       = WiFi.ping(WiFi.gatewayIP());
        if (ping > 0) {
            colors[wifi_led] = col_good;

        } else {
            // console.print("[N] Unable to reach network gateway...\n");
            colors[wifi_led] = col_bad;
            wifi_connected   = false;
            // wifi_setup(NULL);
        }
    }
}

void wifi_setup(void* param) {
    (void)param;
    if (wifi_setup_running) {
        return;
    }
    wifi_setup_running = true;

    // console.print("[N] Configuring WiFi...\n");
    colors[wifi_led] = col_waiting;
    if (WiFi.status() == WL_NO_SHIELD) {
        // console.print("[N] WiFi shield not present.\n");
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    }
    // console.print("[N] Scanning for networks...\n");
    auto cnt = WiFi.scanNetworks();
    if (!cnt) {
        // console.print("[N] No networks found.\n");
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    } else {
        colors[wifi_led] = col_working;
        // console.printf("[N] Found %d networks\n", cnt);
        if (Serial) {
            Serial.printf("%32s %5s %17s %2s %4s\n", "SSID", "ENC", "BSSID        ", "CH", "RSSI");
        }
        for (auto i = 0; i < cnt; i++) {
            uint8_t bssid[6];
            WiFi.BSSID(i, bssid);
            if (Serial) {
                Serial.printf("%32s %5s %17s %2d %4d\n", WiFi.SSID(i), encToString(WiFi.encryptionType(i)), macToString(bssid), WiFi.channel(i), WiFi.RSSI(i));
            }
        }
    }

    if (!wifi_ap_configured) {
#ifdef STASSID
        // console.printf("[N] Adding network: %s\n", ssid);
        if (multi.addAP(ssid, password)) {
#    ifdef SECSSID
            // console.printf("[N] Adding network: %s\n", ssid2);
            if (multi.addAP(ssid2, password2)) {
#    endif
                wifi_ap_configured = true;
#    ifdef SECSSID
            }
#    endif
        }
#endif
    }

    if (multi.run() != WL_CONNECTED) {
        // console.print("[N] Unable to connect to network.\n");
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    }
    wifi_connected   = true;
    colors[wifi_led] = col_good;
    // console.print("[N] WiFi connected.\n");
    wifi_configured    = true;
    wifi_setup_running = false;
}

void ntp_setup(void* param) {
    (void)param;
    if (ntp_setup_running) {
        return;
    }
    colors[ntp_led]   = col_waiting;
    ntp_setup_running = true;
    if (wifi_configured && !ntp_configured) {
        colors[ntp_led] = col_working;
        // console.print("[T] Configuring NTP...\n");
        NTP.begin(NTP_SERVER1, NTP_SERVER2, NTP_TIMEOUT);
        if (NTP.running()) {
            colors[ntp_led] = col_good;
        } else {
            colors[ntp_led] = col_bad;
        }
        if (NTP.waitSet()) {
            time_t    now = time(nullptr);
            struct tm timeinfo;
            gmtime_r(&now, &timeinfo);
            Serial.print("Current time: ");
            Serial.print(asctime(&timeinfo));
        }
        ntp_configured = true;
        // console.print("[T] NTP comfigured.\n");
    }
    ntp_setup_running = false;
}

const char* macToString(uint8_t mac[6]) {
    static char s[20];
    sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return s;
}

const char* encToString(uint8_t enc) {
    switch (enc) {
        case ENC_TYPE_NONE:
            return "NONE";
        case ENC_TYPE_TKIP:
            return "WPA";
        case ENC_TYPE_CCMP:
            return "WPA2";
        case ENC_TYPE_AUTO:
            return "AUTO";
    }
    return "UNKN";
}
