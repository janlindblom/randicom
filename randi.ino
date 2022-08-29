// Copyright (c) 2022 Jan Lindblom <jan@namnlos.io>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include "arduino_secrets.h"
#include "randi.h"
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <Adafruit_NeoPixel.h>

// WiFi stuff
const char* ssid     = STASSID;
const char* password = STAPSK;
WiFiMulti   multi;
const char  timeServer[]    = "time.nist.gov";
const int   NTP_PACKET_SIZE = 48;
byte        packetBuffer[NTP_PACKET_SIZE];
WiFiUDP     Udp;

// NeoPixel stuff.
Adafruit_NeoPixel strip  = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB);
uint16_t          hue    = 0;
uint8_t           sine[] = {4, 3, 2, 1, 0};
// uint32_t          colors[] = {strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0), strip.ColorHSV(0, 0, 0)};
//  Standard colours
uint32_t black    = strip.Color(0, 0, 0);
uint32_t red      = strip.Color(255, 0, 0);
uint32_t green    = strip.Color(0, 255, 0);
uint32_t blue     = strip.Color(0, 0, 255);
uint32_t purple   = strip.Color(255, 0, 255);
uint32_t colors[] = {black, black, black, black, black};

// OLED stuff.
uint8_t ucBackBuffer[1024];

// send an NTP request to the time server at the given address
void sendNTPpacket(const char* address) {
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
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

void wifi_setup() {
    colors[sine[0]] = black;
    while (!Serial) {
        delay(10);
    }
    Serial.println("WiFi setup starting...");
    colors[sine[0]] = purple;
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        colors[sine[0]] = red;
        return;
    }
    Serial.printf("Beginning scan at %d\n", millis());
    auto cnt = WiFi.scanNetworks();
    if (!cnt) {
        Serial.printf("No networks found\n");
        colors[sine[0]] = red;
    } else {
        colors[sine[0]] = purple;
        Serial.printf("Found %d networks\n\n", cnt);
        Serial.printf("%32s %5s %17s %2s %4s\n", "SSID", "ENC", "BSSID        ", "CH", "RSSI");
        for (auto i = 0; i < cnt; i++) {
            uint8_t bssid[6];
            WiFi.BSSID(i, bssid);
            Serial.printf("%32s %5s %17s %2d %4d\n", WiFi.SSID(i), encToString(WiFi.encryptionType(i)), macToString(bssid), WiFi.channel(i), WiFi.RSSI(i));
        }
    }

    multi.addAP(ssid, password);
    if (multi.run() != WL_CONNECTED) {
        Serial.println("Unable to connect to network...");
        colors[sine[0]] = red;
        return;
    }
    colors[sine[0]] = green;
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("WiFi setup done.");
}

void wifi_check_status() {
    colors[sine[0]] = purple;
    if (WiFi.status() != WL_CONNECTED) {
        colors[sine[0]] = red;
    } else {
        int ping = WiFi.ping(WiFi.gatewayIP());
        if (ping > 0) {
            colors[sine[0]] = green;
        } else {
            colors[sine[0]] = red;
        }
    }
}

void neopixel_setup() {
    while (!Serial) {
        delay(10);
    }
    // Serial.println("NeoPixel setup starting...");
    //  gpio_set_function(NEOPIXEL_PIN, GPIO_FUNC_PWM);
    strip.begin();
    strip.setBrightness(30);
    strip.show();
    // Serial.println("Neopixel setup done.");
}

void neopixel_update() {
    for (uint8_t i = 1; i < NUM_LEDS; i++) {
        if (hue >= 65407) {
            hue = 0;
        }
        uint32_t color  = strip.ColorHSV(hue, 255, 255);
        colors[sine[i]] = color;
        delay(100);
        strip.setPixelColor(sine[i], colors[i]);
        hue += 128;
    }
    strip.show();
}

void setup() {
    Serial.begin(9600);
    wifi_setup();
}

void loop() {
    delay(1000);
    wifi_check_status();
}

void setup1() {
    neopixel_setup();
}

void loop1() {
    neopixel_update();
}
