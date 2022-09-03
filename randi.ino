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
#include <WiFiUdp.h>
#include <WiFiNTP.h>

#include <time.h>
#include <sys/time.h>

#include <Adafruit_NeoPixel.h>

std::map<eTaskState, const char*> eTaskStateName{{eReady, "Ready"}, {eRunning, "Running"}, {eBlocked, "Blocked"}, {eSuspended, "Suspended"}, {eDeleted, "Deleted"}};

volatile bool first_boot          = true;
volatile bool wifi_configured     = false;
volatile bool neopixel_configured = false;
volatile bool wifi_ap_configured  = false;
volatile bool ntp_configured      = false;

// WiFi stuff
const char* ssid     = STASSID;
const char* password = STAPSK;
WiFiMulti   multi;

// NeoPixel stuff.
Adafruit_NeoPixel strip  = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB);
uint_fast16_t     hue    = 0;
uint_fast8_t      sine[] = {4, 3, 2, 1, 0};
// uint32_t          colors[] = {strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0)};
//  Standard colours
const uint_fast32_t black    = strip.Color(0, 0, 0);
const uint_fast32_t red      = strip.Color(255, 0, 0);
const uint_fast32_t green    = strip.Color(0, 255, 0);
const uint_fast32_t blue     = strip.Color(0, 0, 255);
const uint_fast32_t purple   = strip.Color(255, 0, 255);
volatile uint32_t   colors[] = {black, black, black, black, black};
const uint_fast8_t  wifi_led = sine[0];

// OLED stuff.
uint_fast8_t ucBackBuffer[1024];

void ps() {
    int           tasks             = uxTaskGetNumberOfTasks();
    TaskStatus_t* pxTaskStatusArray = new TaskStatus_t[tasks];
    unsigned long runtime;
    tasks = uxTaskGetSystemState(pxTaskStatusArray, tasks, &runtime);
    Serial.printf("# Tasks: %d\n", tasks);
    Serial.println("ID, NAME, STATE, PRIO, CYCLES");
    for (int i = 0; i < tasks; i++) {
        Serial.printf("%d: %-16s %-10s %d %lu\n", i, pxTaskStatusArray[i].pcTaskName, eTaskStateName[pxTaskStatusArray[i].eCurrentState], pxTaskStatusArray[i].uxCurrentPriority, pxTaskStatusArray[i].ulRunTimeCounter);
    }
    delete[] pxTaskStatusArray;
}

const char* macToString(uint8_t mac[6]) {
    static char s[20];
    sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return s;
}

const char* encToString(uint_least8_t enc) {
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

void wifi_setup() {
    Serial.println("Configuring WiFi...");
    colors[wifi_led] = black;
    while (!Serial) {
        delay(10);
    }
    colors[wifi_led] = purple;
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present.");
        colors[wifi_led] = red;
        return;
    }
    auto cnt = WiFi.scanNetworks();
    if (!cnt) {
        DEBUGV("No networks found.\n");
        colors[wifi_led] = red;
        return;
    } else {
        colors[wifi_led] = purple;
        DEBUGV("Found %d networks\n\n", cnt);
        DEBUGV("%32s %5s %17s %2s %4s\n", "SSID", "ENC", "BSSID        ", "CH", "RSSI");
        for (auto i = 0; i < cnt; i++) {
            uint8_t bssid[6];
            WiFi.BSSID(i, bssid);
            DEBUGV("%32s %5s %17s %2d %4d\n", WiFi.SSID(i), encToString(WiFi.encryptionType(i)), macToString(bssid), WiFi.channel(i), WiFi.RSSI(i));
        }
    }

    if (!wifi_ap_configured) {
        DEBUGV("Connecting to network: %s\n", ssid);
        if (multi.addAP(ssid, password)) {
        }
        wifi_ap_configured = true;
    }

    if (multi.run() != WL_CONNECTED) {
        Serial.println("Unable to connect to network.");
        colors[wifi_led] = red;
        return;
    }
    colors[wifi_led] = green;
    Serial.println(WiFi.localIP());
    wifi_configured = true;

    if (!ntp_configured) {
        Serial.println("Configuring NTP...");
        // struct timeval tv;
        // const char     time_server1[] = NTP_SERVER1;
        // const char     time_server2[] = NTP_SERVER2;
        // configTime(3600, 3600 * 3, time_server1, time_server2);
        NTP.begin(NTP_SERVER1, NTP_SERVER2, NTP_TIMEOUT);
        // NTP.waitSet();
        ntp_configured = true;
    }
}

void wifi_check_status() {
    colors[wifi_led] = purple;
    if (WiFi.status() != WL_CONNECTED) {
        colors[wifi_led] = red;
        wifi_configured  = false;
        wifi_setup();
    } else {
        int ping = WiFi.ping(WiFi.gatewayIP());
        if (ping > 0) {
            colors[wifi_led] = green;
        } else {
            colors[wifi_led] = red;
        }
    }
}

void neopixel_setup() {
    while (!Serial) {
        delay(10);
    }
    strip.begin();
    strip.setBrightness(30);
    strip.show();
    neopixel_configured = true;
}

void neopixel_update() {
    for (uint_fast8_t i = 0; i < NUM_LEDS; i++) {
        if (hue >= 65467) {
            hue = 0;
        }
        if (sine[i] != wifi_led) {
            uint_fast32_t color = strip.ColorHSV(hue, 255, 255);
            colors[sine[i]]     = color;
        }
        strip.setPixelColor(sine[i], colors[i]);
        hue += 64;
    }
    strip.show();
    delay(100);
}

void setup() {
    Serial.begin(9600);
    neopixel_setup();
}

void loop() {
    delay(20000);
    if (wifi_configured) {
        wifi_check_status();
        // xTaskCreate(wifi_check_status, "WIFI_STATUS_CHECK", 128, nullptr, 1, nullptr);
    } else {
        wifi_setup();
        // xTaskCreate(wifi_check_status, "WIFI_STATUS_CHECK", 128, nullptr, 1, nullptr);
    }
}

void setup1() {
    wifi_setup();
    delay(500);
    if (neopixel_configured && wifi_configured) {
        first_boot = false;
    }
}

void loop1() {
    neopixel_update();
}
