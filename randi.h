// Copyright (c) 2022 Jan Lindblom <jan@namnlos.io>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "arduino_secrets.h"

// NeoPixel stuff.
#define NEOPIXEL_PIN 2
#define NUM_LEDS 5

// OLED stuff.
#define SDA0_PIN 8
#define SCL0_PIN 9
#define RESET_PIN -1
#define OLED_ADDR 0x3c
#define FLIP180 1
#define INVERT 0
#define USE_HW_I2C 1

// WiFi stuff
#ifndef STASSID
#    define STASSID "no-network"
#    define STAPSK "no-password"
#endif
