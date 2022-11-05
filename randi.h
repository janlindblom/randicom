// Copyright (c) 2022 Jan Lindblom <jan@namnlos.co>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "arduino_secrets.h"

#define NEOPIXEL_PIN (2u)
#define NUM_LEDS 8

//  NeoPixel colours
#define black 0x000000
#define red 0xff0000
#define green 0x00ff00
#define blue 0x0000ff
#define purple 0xff00ff
#define pink 0xff69b4
#define turqoise 0x69ffb4
#define nicered 0xff6969
#define col_waiting purple
#define col_working pink
#define col_bad red
#define col_good green

// OLED stuff.
#define RESET_PIN -1
#define OLED_ADDR 0x3c
#define I2C_CLK0 5
#define I2C_DATA0 4
#define I2C_CLK1 27
#define I2C_DATA1 26
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
