// Copyright (c) 2023 Jan Lindblom <jan@namnlos.co>

// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "network.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiNTP.h>
#include <WiFiUdp.h>

#include "arduino_secrets.h"
#include "randi.h"
#include "util.h"


const char *ssid     = STASSID;
const char *password = STAPSK;
#ifdef SECSSID
const char *ssid2     = SECSSID;
const char *password2 = SECPSK;
#endif
WiFiMulti multi;

volatile bool wifi_setup_running = false;
volatile bool wifi_configured    = false;
volatile bool wifi_connected     = false;
volatile bool wifi_ap_configured = false;
volatile bool ntp_setup_running  = false;
volatile bool ntp_configured     = false;

const uint_fast8_t wifi_led = 7;
const uint_fast8_t ntp_led  = 6;

void wifi_check_status(void *param) {
    (void)param;
    colors[wifi_led] = col_working;

    if (WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED) {
        wifi_connected   = false;
        colors[wifi_led] = col_waiting;
    }

    if (multi.run() != WL_CONNECTED) {
        colors[wifi_led] = col_bad;
        wifi_connected   = false;
        // wifi_setup(NULL);
    } else {
        wifi_connected = true;
        int ping       = WiFi.ping(WiFi.gatewayIP());
        if (ping > 0) {
            colors[wifi_led] = col_good;

        } else {
            colors[wifi_led] = col_bad;
            wifi_connected   = false;
            // wifi_setup(NULL);
        }
    }
}

void wifi_setup(void *param) {
    (void)param;
    if (wifi_setup_running) {
        return;
    }
    wifi_setup_running = true;

    colors[wifi_led] = col_waiting;
    if (WiFi.status() == WL_NO_SHIELD) {
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    }
    auto cnt = WiFi.scanNetworks();
    if (!cnt) {
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    } else {
        colors[wifi_led] = col_working;
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
        if (multi.addAP(ssid, password)) {
#    ifdef SECSSID
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
        colors[wifi_led]   = col_bad;
        wifi_connected     = false;
        wifi_setup_running = false;
        return;
    }
    wifi_connected     = true;
    colors[wifi_led]   = col_good;
    wifi_configured    = true;
    wifi_setup_running = false;
}

void ntp_setup(void *param) {
    (void)param;
    if (ntp_setup_running) {
        return;
    }
    colors[ntp_led]   = col_waiting;
    ntp_setup_running = true;
    if (wifi_configured && !ntp_configured) {
        colors[ntp_led] = col_working;
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
    }
    ntp_setup_running = false;
}
