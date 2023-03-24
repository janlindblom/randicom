// Copyright (c) 2022 Jan Lindblom <jan@namnlos.co>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Adafruit_SSD1327.h>

#define OLED_ADDR 0x3c

#define DPI 121 // The 1.5" SSD1327 displays have a DPI/PPI of 121 instead of the default 141.

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

extern Adafruit_SSD1327 monitor;
extern Adafruit_SSD1327 console;

void display_setup(void *param);
