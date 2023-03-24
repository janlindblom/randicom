// Copyright (c) 2022 Jan Lindblom <jan@namnlos.co>
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "display.h"
#include <splash.h>

volatile bool display_configured = false;

void display_setup(void *param) {
    if (Serial) {
        Serial.println("Setting up display...");
    }

    console.begin(OLED_ADDR);
    monitor.begin(OLED_ADDR);

    console.setTextSize(1);
    monitor.setTextSize(1);
    //  console.setFont(&DisplayFont);
    //  monitor.setFont(&DisplayFont);
    console.setTextColor(SSD1327_WHITE, SSD1327_BLACK);
    monitor.setTextColor(SSD1327_WHITE, SSD1327_BLACK);

    console.clearDisplay();
    monitor.clearDisplay();

    console.drawBitmap(monitor.width() / 2 - splash2_width / 2, monitor.height() / 2 - splash2_height / 2, splash2_data, splash2_width, splash2_height, SSD1327_WHITE);
    monitor.drawBitmap(monitor.width() / 2 - splash2_width / 2, monitor.height() / 2 - splash2_height / 2, splash2_data, splash2_width, splash2_height, SSD1327_WHITE);
    console.display();
    monitor.display();

    delay(150);
    console.clearDisplay();
    monitor.clearDisplay();
    console.display();
    monitor.display();

    // con.setBusClock(400000);
    /*

    mon.begin();
    mon.setContrast(160);
    */

    // console_configured = console.begin();
    //   console.clearBuffer();
    //   console.setContrast(160);
    /*
        console.setFont(u8g2_font_5x8_mf);
        // console.setFont(u8g2_font_helvR08_tf); // set the font for the terminal
       window u8g2log.begin(U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
        u8g2log.setLineHeightOffset(1); // set extra space between lines in pixel,
       this can be negative u8g2log.setRedrawMode(0);       // 0: Update screen
       with newline, 1: Update screen for every char

        console.print("Console initialised.\n");
        // monitor_configured = monitor.begin();
        // console.print("Monitor initialised.\n");
        console.print("Displays initialised.\n");
        */
    display_configured = true;
}
