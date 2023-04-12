// Copyright (c) 2023 Jan Lindblom <jan@namnlos.co>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>

// Time stuff
#ifndef NTP_SERVER1
#    define NTP_SERVER1 "0.fi.pool.ntp.org"
#endif
#ifndef NTP_SERVER2
#    define NTP_SERVER2 "1.fi.pool.ntp.org"
#endif
#define NTP_TIMEOUT 3600

// WiFi stuff
#ifndef STASSID
#    define STASSID "no-network"
#    define STAPSK "no-password"
#endif

extern volatile bool wifi_setup_running;
extern volatile bool wifi_configured;
extern volatile bool wifi_connected;
extern volatile bool wifi_ap_configured;
extern volatile bool ntp_setup_running;
extern volatile bool ntp_configured;

extern const uint_fast8_t wifi_led;
extern const uint_fast8_t ntp_led;

extern volatile uint32_t colors[];

void wifi_setup(void *param);
void wifi_check_status(void *param);
void ntp_setup(void *param);
