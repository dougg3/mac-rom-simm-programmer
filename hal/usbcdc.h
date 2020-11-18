/*
 * usbcdc.h
 *
 *  Created on: Nov 22, 2020
 *      Author: Doug
 */

#ifndef HAL_USBCDC_H_
#define HAL_USBCDC_H_

#include <stdint.h>
#include "usbcdc_hw.h"

// Note: Functions commented out should be implemented as static inline
// functions in the board-specific header file for efficiency.
void USBCDC_Init(void);
void USBCDC_Disable(void);
void USBCDC_Check(void);
//void USBCDC_SendByte(uint8_t byte);
//bool USBCDC_SendData(uint8_t const *data, uint16_t len);
//int16_t USBCDC_ReadByte(void);
//void USBCDC_Flush(void)

#endif /* HAL_USBCDC_H_ */
