// Copyright (c) 2022 Jan Lindblom <jan@namnlos.io>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "arduino_secrets.h"

#define NEOPIXEL_PIN (2u)
#define NUM_LEDS 8

// OLED stuff.
#define RESET_PIN -1
#define OLED_ADDR 0x3c
#define FLIP180 0
#define INVERT 0

#define SSD1327_SCROLLING_START 0x2F
#define SSD1327_SCROLLING_STOP 0x2E
#define SSD1327_SCROLLING_RIGHT 0x26
#define SSD1327_SCROLLING_LEFT 0x27

// Scroll rate constants. See datasheet page 40.
#define SSD1327_SCROLL_2 0b111
#define SSD1327_SCROLL_3 0b100
#define SSD1327_SCROLL_4 0b101
#define SSD1327_SCROLL_5 0b110
#define SSD1327_SCROLL_6 0b000
#define SSD1327_SCROLL_32 0b001
#define SSD1327_SCROLL_64 0b010
#define SSD1327_SCROLL_256 0b011

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
