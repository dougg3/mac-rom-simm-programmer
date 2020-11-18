/*
 * board.h
 *
 *  Created on: Nov 14, 2020
 *      Author: Doug
 */

#ifndef HAL_BOARD_H_
#define HAL_BOARD_H_

#include "gpio.h"
#include "spi.h"

// Commented-out functions should be static inline in each board-specific header file.
//GPIOPin Board_LEDPin(void);
#include "board_hw.h"

void Board_Init(void);
bool Board_BrownoutDetected(void);

#endif /* HAL_BOARD_H_ */
