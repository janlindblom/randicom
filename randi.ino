// Copyright (c) 2022 Jan Lindblom <jan@namnlos.co>

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

#define DPI 121 // The 1.5" SSD1327 displays have a DPI/PPI of 121 instead of the default 141.

#include <Adafruit_NeoPixel.h>
//#include <Adafruit_I2CDevice.h>
//#include <Adafruit_GrayOLED.h>
//#include <Adafruit_SSD1327.h>
//#include <splash.h>
//#include <Fonts/FreeMono9pt7b.h>
//#include "fonts/InputMono_Regular4pt7b.h"
//#include "fonts/FSEX302_alt4pt7b.h"
// #include <lcdgfx.h>
#include <U8g2lib.h>

#include <stdarg.h>

#define U8LOG_WIDTH 22
#define U8LOG_HEIGHT 17
uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
U8G2LOG u8g2log;

void        neopixel_setup(void* param);
void        neopixel_update(void* param);
void        wifi_setup(void* param);
void        ntp_setup(void* param);
void        display_setup(void* param);
const char* macToString(uint8_t mac[6]);
const char* encToString(uint8_t enc);
void        ps();

std::map<eTaskState, const char*> eTaskStateName{{eReady, "Ready"}, {eRunning, "Running"}, {eBlocked, "Blocked"}, {eSuspended, "Suspended"}, {eDeleted, "Deleted"}};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB);
// Adafruit_SSD1327  monitor = Adafruit_SSD1327(128, 128, &Wire, RESET_PIN, 100000u, 100000u);
// Adafruit_SSD1327  console = Adafruit_SSD1327(128, 128, &Wire1, RESET_PIN, 100000u, 100000u);

U8G2_SSD1327_WS_128X128_F_HW_I2C con(U8G2_R0, /* reset=*/U8X8_PIN_NONE); //, /* clock=*/27, /* data=*/26);

// U8G2_SSD1327_EA_W128128_2_HW_I2C     con(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // , /* clock=*/5, /* data=*/4);
// U8G2_SSD1327_EA_W128128_2_2ND_HW_I2C mon(U8G2_R0, /* reset=*/U8X8_PIN_NONE); // , /* clock=*/27, /* data=*/26);

//U8G2_SSD1327_WS_128X128_F_2ND_HW_I2C mon(U8G2_R0, /* reset=*/U8X8_PIN_NONE); //, /* clock=*/27, /* data=*/26);

// const GFXfont DisplayFont = InputMono_Regular4pt7b;
//  const GFXfont     InputFont   = InputMono_Regular4pt7b;
//  const GFXfont     FixedSys    = FSEX302_alt4pt7b;

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

#define boot_complete (wifi_configured && wifi_ap_configured && ntp_configured && neopixel_configured && display_configured)

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

#define monitor_scroll(str)                                                                     \
    {                                                                                           \
        int16_t  x, y;                                                                          \
        uint16_t w, h;                                                                          \
        monitor.getTextBounds(str, monitor.getCursorX(), monitor.getCursorY(), &x, &y, &w, &h); \
        if (y + h >= monitor.height()) {                                                        \
            int16_t pixels = (y + h) - monitor.height();                                        \
            monitor.setCursor(0, 6);                                                            \
            monitor.clearDisplay();                                                             \
            uint8_t* buffer = monitor.getBuffer();                                              \
            if (Serial) {                                                                       \
                Serial.printf("[M] Scroll pixels: %d\n", pixels);                               \
                Serial.printf("[M] Buffer: %d\n", sizeof(buffer));                              \
            }                                                                                   \
        } else {                                                                                \
            if (Serial) {                                                                       \
                Serial.println("[M] No scrolling required.");                                   \
            }                                                                                   \
        }                                                                                       \
    };

#define console_scroll(str)                                                                     \
    {                                                                                           \
        int16_t  x, y;                                                                          \
        uint16_t w, h;                                                                          \
        console.getTextBounds(str, console.getCursorX(), console.getCursorY(), &x, &y, &w, &h); \
        if (y >= console.height()) {                                                            \
            int16_t pixels = (y + h) - console.height();                                        \
            console.setCursor(0, 6);                                                            \
            console.clearDisplay();                                                             \
            if (Serial) {                                                                       \
                Serial.printf("[C] Scroll pixels: %d\n", pixels);                               \
            }                                                                                   \
        } else {                                                                                \
            if (Serial) {                                                                       \
                Serial.println("[C] No scrolling required.");                                   \
            }                                                                                   \
        }                                                                                       \
    };

#define console_println(s)                                                                                              \
    {                                                                                                                   \
        if (display_configured) {                                                                                       \
            console.fillRect(console.getCursorX(), console.getCursorY(), 128 - console.getCursorX(), 8, SSD1327_BLACK); \
            console_scroll(s);                                                                                          \
            console.println(s);                                                                                         \
            console.display();                                                                                          \
        }                                                                                                               \
    };

#define console_printf(s)                                                                                               \
    {                                                                                                                   \
        if (display_configured) {                                                                                       \
            console.fillRect(console.getCursorX(), console.getCursorY(), 128 - console.getCursorX(), 8, SSD1327_BLACK); \
            console_scroll(s);                                                                                          \
            console.printf(s);                                                                                          \
            console.display();                                                                                          \
        }                                                                                                               \
    };

#define console_printf2(s, p)                                                                                           \
    {                                                                                                                   \
        if (display_configured) {                                                                                       \
            console.fillRect(console.getCursorX(), console.getCursorY(), 128 - console.getCursorX(), 8, SSD1327_BLACK); \
            console_scroll(s);                                                                                          \
            console.printf(s, p);                                                                                       \
            console.display();                                                                                          \
        }                                                                                                               \
    };

#define monitor_printf2(s, p)                                                                                           \
    {                                                                                                                   \
        if (display_configured) {                                                                                       \
            monitor.fillRect(monitor.getCursorX(), monitor.getCursorY(), 128 - monitor.getCursorX(), 8, SSD1327_BLACK); \
            monitor_scroll(s);                                                                                          \
            monitor.printf(s, p);                                                                                       \
            monitor.display();                                                                                          \
        }                                                                                                               \
    };

#define console_print2(s, p)                                                                                            \
    {                                                                                                                   \
        if (display_configured) {                                                                                       \
            console.fillRect(console.getCursorX(), console.getCursorY(), 128 - console.getCursorX(), 8, SSD1327_BLACK); \
            console_scroll(s);                                                                                          \
            console.print(s, p);                                                                                        \
            console.display();                                                                                          \
        }                                                                                                               \
    };

void display_setup(void* param) {
    // monitor.begin(OLED_ADDR);
    // console.begin(OLED_ADDR);
    // monitor.setTextSize(1);
    // console.setTextSize(1);
    // monitor.setFont(&DisplayFont);
    // console.setFont(&DisplayFont);
    // monitor.setTextColor(SSD1327_WHITE, SSD1327_BLACK);
    // console.setTextColor(SSD1327_WHITE, SSD1327_BLACK);

    // monitor.clearDisplay();
    // console.clearDisplay();

    // monitor.drawBitmap(monitor.width() / 2 - splash2_width / 2, monitor.height() / 2 - splash2_height / 2, splash2_data, splash2_width, splash2_height, SSD1327_WHITE);
    // console.drawBitmap(console.width() / 2 - splash2_width / 2, console.height() / 2 - splash2_height / 2, splash2_data, splash2_width, splash2_height, SSD1327_WHITE);
    // monitor.display();
    // console.display();

    // delay(150);
    // console.clearDisplay();
    //  monitor.clearDisplay();
    //  monitor.display();
    // console.display();

    // monitor.setCursor(0, 6);
    // console.setCursor(0, 6);

    // con.setBusClock(400000);
    
    con.begin();
    con.setContrast(160);
    // mon.begin();
    con.setFont(u8g2_font_5x7_tr); // set the font for the terminal window
    u8g2log.begin(U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
    u8g2log.setLineHeightOffset(0); // set extra space between lines in pixel, this can be negative
    u8g2log.setRedrawMode(0);       // 0: Update screen with newline, 1: Update screen for every char

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
        Serial.begin(9600);
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
    } else {
        neopixel_setup(NULL);
    }
    /*
        if (t < millis()) {
            t = millis() + 15000; // every 15 seconds
            u8g2log.print("\f");  // \f = form feed: clear the screen
        }
        u8g2log.print("millis=");
        u8g2log.print(millis());
        u8g2log.print("\n");
    */
    // print the log window
    con.firstPage();
    do {
        con.drawLog(0, 10, u8g2log); // draw the log content on the display
    } while (con.nextPage());
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
    } else {
        wifi_setup(NULL);
    }

    if (first_boot && boot_complete) {
        u8g2log.println("System Initialised.");
        first_boot = false;
    }

    ps();
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
        if (sine[i] != wifi_led && sine[i] != ntp_led && sine[i] != display_led) {
            uint_fast32_t color = strip.ColorHSV(hue, 255, 255);
            colors[sine[i]]     = color;
        }
        strip.setPixelColor(sine[i], colors[sine[i]]);
        hue += 32;
        strip.show();
    }
}

void wifi_check_status(void* param) {
    (void)param;
    colors[wifi_led] = col_working;

    if (WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED) {
        u8g2log.println("[N] WiFi connection lost...");
        wifi_connected   = false;
        colors[wifi_led] = col_waiting;
    }

    if (multi.run() != WL_CONNECTED) {
        u8g2log.println("[N] Unable to connect to network.");
        colors[wifi_led] = col_bad;
        wifi_connected   = false;
        wifi_setup(NULL);
    } else {
        int ping = WiFi.ping(WiFi.gatewayIP());
        if (ping > 0) {
            colors[wifi_led] = col_good;
        } else {
            u8g2log.println("[N] Unable to reach network gateway...");
            colors[wifi_led] = col_bad;
            wifi_connected   = false;
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

    u8g2log.println("[N] Configuring WiFi...");
    colors[wifi_led] = col_waiting;
    if (WiFi.status() == WL_NO_SHIELD) {
        u8g2log.println("[N] WiFi shield not present.");
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    }
    u8g2log.println("[N] Scanning for networks...");
    auto cnt = WiFi.scanNetworks();
    if (!cnt) {
        u8g2log.println("[N] No networks found.");
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    } else {
        colors[wifi_led] = col_working;
        u8g2log.printf("[N] Found %d networks\n", cnt);
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
        u8g2log.printf("[N] Adding network: %s\n", ssid);
        if (multi.addAP(ssid, password)) {
#ifdef SECSSID
            u8g2log.printf("[N] Adding network: %s\n", ssid2);
            if (multi.addAP(ssid2, password2)) {
#endif
                wifi_ap_configured = true;
#ifdef SECSSID
            }
#endif
        }
    }

    if (multi.run() != WL_CONNECTED) {
        u8g2log.println("[N] Unable to connect to network.");
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    }
    wifi_connected   = true;
    colors[wifi_led] = col_good;
    u8g2log.println("[N] WiFi connected.");
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
        u8g2log.println("[T] Configuring NTP...");
        NTP.begin(NTP_SERVER1, NTP_SERVER2, NTP_TIMEOUT);
        if (NTP.running()) {
            colors[ntp_led] = col_good;
        } else {
            colors[ntp_led] = col_bad;
        }
        ntp_configured = true;
        u8g2log.println("[T] NTP comfigured.");
    }
    ntp_setup_running = false;
}

void ps() {
    int           tasks             = uxTaskGetNumberOfTasks();
    TaskStatus_t* pxTaskStatusArray = new TaskStatus_t[tasks];
    unsigned long runtime;
    tasks = uxTaskGetSystemState(pxTaskStatusArray, tasks, &runtime);
    u8g2log.printf("[P] Tasks: %d\n", tasks);
    u8g2log.printf("%-10s %10s\n", "NAME", "STATE");
    // console.display();
    for (int i = 0; i < tasks; i++) {
        u8g2log.printf("%-10s %10s\n", pxTaskStatusArray[i].pcTaskName, eTaskStateName[pxTaskStatusArray[i].eCurrentState]);
        // console.display();
    }
    delete[] pxTaskStatusArray;
}