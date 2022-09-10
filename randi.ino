// Copyright (c) 2022 Jan Lindblom <jan@namnlos.io>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include "randi.h"

#include <FreeRTOS.h>
#include <map>
#include <atomic.h>
#include <croutine.h>
#include <event_groups.h>
#include <FreeRTOSConfig.h>
#include <rp2040_config.h>
#include <semphr.h>
#include <task.h>

#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <WiFiNTP.h>

#include <time.h>
#include <sys/time.h>

#include <Adafruit_NeoPixel.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SSD1327.h>
#include <splash.h>
#include <Fonts/Org_01.h>

void        neopixel_setup(void* param);
void        neopixel_update(void* param);
void        wifi_setup(void* param);
void        ntp_setup(void* param);
void        display_setup(void* param);
const char* macToString(uint8_t mac[6]);
const char* encToString(uint8_t enc);
void        testdrawline(Adafruit_SSD1327 display);
void        ps();

std::map<eTaskState, const char*> eTaskStateName{{eReady, "Ready"}, {eRunning, "Running"}, {eBlocked, "Blocked"}, {eSuspended, "Suspended"}, {eDeleted, "Deleted"}};

Adafruit_NeoPixel strip       = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB);
Adafruit_SSD1327  disp1       = Adafruit_SSD1327(128, 128, &Wire, RESET_PIN, 100000u, 100000u);
Adafruit_SSD1327  disp2       = Adafruit_SSD1327(128, 128, &Wire1, RESET_PIN, 100000u, 100000u);
const GFXfont     DisplayFont = Org_01;

//  Standard colours
const uint_fast32_t black  = 0x000000;
const uint_fast32_t red    = 0xff0000;
const uint_fast32_t green  = 0x00ff00;
const uint_fast32_t blue   = 0x0000ff;
const uint_fast32_t purple = 0xff00ff;

volatile uint_fast16_t hue      = 0;
const uint_fast8_t     sine[]   = {4, 3, 2, 1, 0, 5, 6, 7};
volatile uint32_t      colors[] = {black, black, black, black, black, black, black, black};

volatile bool wifi_setup_running     = false;
volatile bool wifi_configured        = false;
volatile bool wifi_connected         = false;
volatile bool wifi_ap_configured     = false;
volatile bool ntp_setup_running      = false;
volatile bool ntp_configured         = false;
volatile bool neopixel_setup_running = false;
volatile bool neopixel_configured    = false;
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

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16
static const unsigned char PROGMEM logo16_glcd_bmp[] = {
    // clang-format off
      0, 192,   1, 192,   1, 192,   3, 224,
    243, 224, 254, 248, 126, 255,  51, 159,
     31, 252,  13, 112,  27, 160,  63, 224,
     63, 240, 124, 240, 112, 112,   0,  48
    // clang-format on
};

#define boot_complete (wifi_configured && wifi_ap_configured && ntp_configured && neopixel_configured && display_configured)

#define oled_println(s)           \
    {                             \
        if (display_configured) { \
            disp1.println(s);     \
            disp1.display();      \
        }                         \
    }

#define oled_printf(s)            \
    {                             \
        if (display_configured) { \
            disp1.printf(s);      \
            disp1.display();      \
        }                         \
    }

#define oled_printf2(s, p)        \
    {                             \
        if (display_configured) { \
            disp1.printf(s, p);   \
            disp1.display();      \
        }                         \
    }

#define oled_print2(s, p)         \
    {                             \
        if (display_configured) { \
            disp1.print(s, p);    \
            disp1.display();      \
        }                         \
    }

void display_setup(void* param) {
    disp1.begin(OLED_ADDR);
    static const uint8_t oled_gray[] = {
        // clang-format off
        SSD1327_DISPLAYOFF,
        SSD1327_GRAYTABLE,
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x10, 0x18, 0x20,
        0x2f, 0x38, 0x3f,
        SSD1327_NORMALDISPLAY,
        SSD1327_DISPLAYON
        // clang-format on
    };
    disp1.oled_commandList(oled_gray, sizeof(oled_gray));
    disp1.setFont(&DisplayFont);
    disp1.setTextColor(SSD1327_WHITE);
    disp1.clearDisplay();
    disp1.drawBitmap(disp1.width() / 2 - splash2_width / 2, disp1.height() / 2 - splash2_height / 2, splash2_data, splash2_width, splash2_height, SSD1327_WHITE);
    disp1.display();
    delay(150);
    disp1.clearDisplay();
    disp1.display();
    display_configured = true;
}

/**
 * @brief Main startup sequence.
 */
void setup() {
    if (!Serial) {
        Serial.begin(9600);
    }

    display_setup(NULL);
    neopixel_setup(NULL);
}

/**
 * @brief Secondary startup sequence.
 *
 */
void setup1() {
    delay(300);
    wifi_setup(NULL);
    ntp_setup(NULL);
}

/**
 * @brief Fast main loop.
 */
void loop() {
    delay(100);
    if (ntp_configured) {
        colors[ntp_led] = green;
    } else {
        colors[ntp_led] = red;
    }
    if (display_configured) {
        colors[display_led] = green;
    } else {
        colors[display_led] = red;
    }
    if (neopixel_configured) {
        neopixel_update(NULL);
    } else if (!neopixel_configured) {
        neopixel_setup(NULL);
    }
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
        if (!ntp_configured) {
            ntp_setup(NULL);
        }
    } else if (!wifi_setup_running) {
        wifi_setup(NULL);
    }

    if (boot_complete && first_boot) {
        first_boot = false;
        disp1.clearDisplay();
    }

    ps();
}

void neopixel_setup(void* param) {
    (void)param;
    if (neopixel_setup_running) {
        return;
    }
    neopixel_setup_running = true;

    oled_printf("Black: 0x");
    oled_print2(black, HEX);
    oled_printf("\nRed: 0x");
    oled_print2(red, HEX);
    oled_printf("\nGreen: 0x");
    oled_print2(green, HEX);
    oled_printf("\n");

    strip.begin();
    strip.setBrightness(30);
    neopixel_configured = true;
    strip.show();
    neopixel_setup_running = false;
}

void neopixel_update(void* param) {
    for (uint_fast8_t i = 0; i < NUM_LEDS; i++) {
        if (hue >= 65503) {
            hue = 0;
        }
        if (sine[i] != wifi_led && sine[i] != ntp_led && sine[i] != display_led) {
            uint_fast32_t color = strip.ColorHSV(hue, 255, 255);
            colors[sine[i]]     = color;
        }
        strip.setPixelColor(sine[i], colors[sine[i]]);
        hue += 32;
        strip.show();
    }
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

void wifi_check_status(void* param) {
    (void)param;
    colors[wifi_led] = purple;

    if (WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED) {
        oled_println("WiFi connection lost...");
        colors[wifi_led] = blue;
    }

    if (multi.run() != WL_CONNECTED) {
        oled_println("Unable to connect to network.");
        colors[wifi_led] = red;
        wifi_configured  = false;
        wifi_setup(NULL);
    } else {
        int ping = WiFi.ping(WiFi.gatewayIP());
        if (ping > 0) {
            colors[wifi_led] = green;
        } else {
            oled_println("Unable to reach network gateway...");
            colors[wifi_led] = red;
            wifi_setup(NULL);
        }
    }
}

void wifi_setup(void* param) {
    (void)param;
    if (wifi_setup_running) {
        return;
    }
    wifi_setup_running = true;

    oled_println("Configuring WiFi...");
    colors[wifi_led] = purple;
    if (WiFi.status() == WL_NO_SHIELD) {
        oled_println("WiFi shield not present.");
        colors[wifi_led]   = red;
        wifi_setup_running = false;
        return;
    }
    auto cnt = WiFi.scanNetworks();
    if (!cnt) {
        oled_println("No networks found.");
        colors[wifi_led]   = red;
        wifi_setup_running = false;
        return;
    } else {
        colors[wifi_led] = purple;
        oled_printf2("Found %d networks\n", cnt);
        DEBUGV("%32s %5s %17s %2s %4s\n", "SSID", "ENC", "BSSID        ", "CH", "RSSI");
        for (auto i = 0; i < cnt; i++) {
            uint8_t bssid[6];
            WiFi.BSSID(i, bssid);
            DEBUGV("%32s %5s %17s %2d %4d\n", WiFi.SSID(i), encToString(WiFi.encryptionType(i)), macToString(bssid), WiFi.channel(i), WiFi.RSSI(i));
        }
    }

    if (!wifi_ap_configured) {
        oled_printf2("Adding network: %s\n", ssid);
        if (multi.addAP(ssid, password)) {
#ifdef SECSSID
            oled_printf2("Adding network: %s\n", ssid2);
            if (multi.addAP(ssid2, password2)) {
#endif
                wifi_ap_configured = true;
#ifdef SECSSID
            }
#endif
        }
    }

    if (multi.run() != WL_CONNECTED) {
        oled_println("Unable to connect to network.");
        colors[wifi_led]   = red;
        wifi_setup_running = false;
        return;
    }
    colors[wifi_led] = green;
    oled_println("WiFi connected.");
    wifi_configured    = true;
    wifi_connected     = true;
    wifi_setup_running = false;
}

void ntp_setup(void* param) {
    (void)param;
    if (ntp_setup_running) {
        return;
    }
    ntp_setup_running = true;
    if (wifi_configured && !ntp_configured) {
        oled_println("Configuring NTP...");
        NTP.begin(NTP_SERVER1, NTP_SERVER2, NTP_TIMEOUT);
        ntp_configured = true;
    }
    ntp_setup_running = false;
}

void testdrawline(Adafruit_SSD1327 display) {
    for (uint8_t i = 0; i < display.width(); i += 4) {
        display.drawLine(0, 0, i, display.height() - 1, SSD1327_WHITE);
        display.display();
    }
    for (uint8_t i = 0; i < display.height(); i += 4) {
        display.drawLine(0, 0, display.width() - 1, i, SSD1327_WHITE);
        display.display();
    }
    delay(250);

    display.clearDisplay();
    for (uint8_t i = 0; i < display.width(); i += 4) {
        display.drawLine(0, display.height() - 1, i, 0, SSD1327_WHITE);
        display.display();
    }
    for (int8_t i = display.height() - 1; i >= 0; i -= 4) {
        display.drawLine(0, display.height() - 1, display.width() - 1, i, SSD1327_WHITE);
        display.display();
    }
    delay(250);

    display.clearDisplay();
    for (int8_t i = display.width() - 1; i >= 0; i -= 4) {
        display.drawLine(display.width() - 1, display.height() - 1, i, 0, SSD1327_WHITE);
        display.display();
    }
    for (int8_t i = display.height() - 1; i >= 0; i -= 4) {
        display.drawLine(display.width() - 1, display.height() - 1, 0, i, SSD1327_WHITE);
        display.display();
    }
    delay(250);

    display.clearDisplay();
    for (uint8_t i = 0; i < display.height(); i += 4) {
        display.drawLine(display.width() - 1, 0, 0, i, SSD1327_WHITE);
        display.display();
    }
    for (uint8_t i = 0; i < display.width(); i += 4) {
        display.drawLine(display.width() - 1, 0, i, display.height() - 1, SSD1327_WHITE);
        display.display();
    }
    delay(250);
}

void ps() {
    int           tasks             = uxTaskGetNumberOfTasks();
    TaskStatus_t* pxTaskStatusArray = new TaskStatus_t[tasks];
    unsigned long runtime;
    tasks = uxTaskGetSystemState(pxTaskStatusArray, tasks, &runtime);
    DEBUGV("# Tasks: %d\n", tasks);
    DEBUGV("%2s: %-16s %-10s %-4s %-12s\n", "ID", "NAME", "STATE", "PRIO", "CYCLES");
    for (int i = 0; i < tasks; i++) {
        DEBUGV("%2d: %-16s %-10s %-4d %-12lu\n", i, pxTaskStatusArray[i].pcTaskName, eTaskStateName[pxTaskStatusArray[i].eCurrentState], pxTaskStatusArray[i].uxCurrentPriority, pxTaskStatusArray[i].ulRunTimeCounter);
    }
    delete[] pxTaskStatusArray;
}
