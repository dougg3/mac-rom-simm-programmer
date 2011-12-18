/*
 * simm_electrical_test.h
 *
 *  Created on: Nov 26, 2011
 *      Author: Doug
 */

#ifndef SIMM_ELECTRICAL_TEST_H_
#define SIMM_ELECTRICAL_TEST_H_

#include <stdint.h>

#define GROUND_FAIL_INDEX					0xFF

#define FIRST_ADDRESS_LINE_FAIL_INDEX		0
#define LAST_ADDRESS_LINE_FAIL_INDEX		(FIRST_ADDRESS_LINE_FAIL_INDEX + 20)
#define FIRST_DATA_LINE_FAIL_INDEX			(LAST_ADDRESS_LINE_FAIL_INDEX + 1)
#define LAST_DATA_LINE_FAIL_INDEX			(FIRST_DATA_LINE_FAIL_INDEX + 31)
#define CS_FAIL_INDEX						(LAST_DATA_LINE_FAIL_INDEX + 1)
#define OE_FAIL_INDEX						(CS_FAIL_INDEX + 1)
#define WE_FAIL_INDEX						(OE_FAIL_INDEX + 1)

int SIMMElectricalTest_Run(void (*errorHandler)(uint8_t, uint8_t));

#endif /* SIMM_ELECTRICAL_TEST_H_ */
