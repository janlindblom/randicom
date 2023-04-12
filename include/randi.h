// Copyright (c) 2023 Jan Lindblom <jan@namnlos.co>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

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

#define I2C_CLK0 5
#define I2C_DATA0 4
#define I2C_CLK1 27
#define I2C_DATA1 26
#define FLIP180 0
#define INVERT 0

extern volatile uint32_t colors[];

void neopixel_setup(void *param);
void neopixel_update(void *param);
