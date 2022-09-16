#pragma once

// GPIO 34 - 39 can only be set as input mode and do not have software-enabled pullup or pulldown functions.
// ADC pins: 0, 2, 4, 12 - 15, 25 - 39

#define PINOUT_BOARD_LED_PIN 2

#define PINOUT_CCD_SI 14  // GPO
#define PINOUT_CCD_CLK 12 // GPO
#define PINOUT_CCD_AO 13  // ADC pin required