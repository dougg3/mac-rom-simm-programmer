/*
 * usbcdc.c
 *
 *  Created on: Jun 19, 2023
 *      Author: Doug
 *
 * Copyright (C) 2011-2023 Doug Brown
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Portions of this code also came from Nuvoton's BSP, originally
 * licensed as Apache-2.0:
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 *
 */

#include "usbcdc_hw.h"
#include <stdbool.h>

// Undocumented register for HIRC trim from Nuvoton's samples
#define TRIM_INIT				(SYS_BASE + 0x118)

#define SET_LINE_CODE			0x20
#define GET_LINE_CODE			0x21
#define SET_CONTROL_LINE_STATE  0x22

/// Setup buffer uses first 8 bytes of USB SRAM
#define SETUP_BUF_BASE			0
#define SETUP_BUF_LEN			8
/// EP0 and EP1 share the same USB SRAM, 64 bytes of control
#define EP0_BUF_BASE			(SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP0_BUF_LEN				EP0_MAX_PKT_SIZE
#define EP1_BUF_BASE			(SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP1_BUF_LEN				EP1_MAX_PKT_SIZE
/// EP2 uses 8 bytes for interrupt IN (not really used though)
#define EP2_BUF_BASE			(EP1_BUF_BASE + EP1_BUF_LEN)
#define EP2_BUF_LEN				EP2_MAX_PKT_SIZE
/// EP3 uses 64 bytes for bulk IN
#define EP3_BUF_BASE			(EP2_BUF_BASE + EP2_BUF_LEN)
#define EP3_BUF_LEN				EP3_MAX_PKT_SIZE
/// EP4 uses 64 bytes for bulk OUT
#define EP4_BUF_BASE			(EP3_BUF_BASE + EP3_BUF_LEN)
#define EP4_BUF_LEN				EP4_MAX_PKT_SIZE

/// Struct to represent the current line coding
typedef struct
{
	uint32_t baudRate; // Baud rate
	uint8_t stopBits;  // stop bit
	uint8_t parity;    // parity
	uint8_t dataBits;  // data bits
} CDCLineCoding;

static void USBCDC_SendDataInBuffer(void);
static void USBCDC_InitEndpoints(void);
static void USBCDC_ClassRequest(void);

/// Default HIRC trim value in case of errors
static uint32_t trimInit;
/// Toggle flag for ensuring the received endpoint 4 packet is the expected one
static volatile uint32_t ep4OutToggle = 0;
/// Line coding for CDC device, not used by this firmware
static CDCLineCoding cdcLineCoding = {0, 0, 0, 0};
/// Control signal for CDC device, not used by this firmware
static uint16_t cdcCtrlSignal;

/// Buffer to send to the CDC serial port
static uint8_t cdcTxBuf[EP3_MAX_PKT_SIZE];
/// Current position in the TX buffer
static uint32_t cdcTxBufPos = 0;
/// Flag that is true if a TX is currently active
static volatile bool cdcTxActive = false;
/// Buffer we read into
static uint8_t cdcRxBuf[EP4_MAX_PKT_SIZE];
/// Current length of RX buffer
static uint32_t cdcRxLen = 0;
/// Current position into RX buffer
static uint32_t cdcRxPos = 0;
/// Flag that is true if new RX data is ready to read
static volatile bool cdcRxReady = false;

/** Initializes the USB CDC serial port
 *
 */
void USBCDC_Init(void)
{
	// Set up USB
	USBD_Open(&gsInfo, USBCDC_ClassRequest);
	USBCDC_InitEndpoints();
	USBD_Start();
	NVIC_EnableIRQ(USBD_IRQn);

	// Backup the default trim value, go back to it if there's an error
	trimInit = M32(TRIM_INIT);

	// Clear USB SOF flag; it will be used to detect if we have a USB
	// signal available to use for HIRC trim
	USBD_CLR_INT_FLAG(USBD_INTSTS_SOFIF_Msk);
}

/** Performs any necessary periodic tasks for the USB CDC serial port
 *
 */
void USBCDC_Check(void)
{
	// If we haven't enabled auto trim, and we have a USB signal available,
	// then do it!
	if (((SYS->HIRCTRIMCTL & SYS_HIRCTRIMCTL_FREQSEL_Msk) != 0x1) &&
		(USBD->INTSTS & USBD_INTSTS_SOFIF_Msk))
	{
		// Clear SOF flag for next time
		USBD_CLR_INT_FLAG(USBD_INTSTS_SOFIF_Msk);

		// Start USB trim:
		// - HIRC trim reference is USB clock
		// - Enable auto trim, and trim to 48 MHz
		// - Use 4 cycles for trimming
		// - Boundary enabled (value = 10)
		SYS->HIRCTRIMCTL = (0x1UL << SYS_HIRCTRIMCTL_REFCKSEL_Pos)
						   | (0x1UL << SYS_HIRCTRIMCTL_FREQSEL_Pos)
						   | (0x0UL << SYS_HIRCTRIMCTL_LOOPSEL_Pos)
						   | (0x1UL << SYS_HIRCTRIMCTL_BOUNDEN_Pos)
						   | (10UL  << SYS_HIRCTRIMCTL_BOUNDARY_Pos);
	}

	// If we detect a clock error or trim failure, disable auto trim. We can try again later.
	if (SYS->HIRCTRIMSTS & (SYS_HIRCTRIMSTS_CLKERIF_Msk | SYS_HIRCTRIMSTS_TFAILIF_Msk))
	{
		// Restore original trim value we read at startup
		M32(TRIM_INIT) = trimInit;

		// Disable USB trim
		SYS->HIRCTRIMCTL = 0;

		// Clear the error flags
		SYS->HIRCTRIMSTS = SYS_HIRCTRIMSTS_CLKERIF_Msk | SYS_HIRCTRIMSTS_TFAILIF_Msk;

		// Clear the SOF flag so the next time it's set we know we have a USB signal
		USBD_CLR_INT_FLAG(USBD_INTSTS_SOFIF_Msk);
	}

	// Flush the USB CDC port every main loop just like LUFA does
	USBCDC_Flush();
}

/** Sends a byte out the USB serial port
 *
 * @param b The byte
 */
void USBCDC_SendByte(uint8_t b)
{
	// Fill up our buffer to send out the USB serial port
	cdcTxBuf[cdcTxBufPos++] = b;
	if (cdcTxBufPos == EP3_MAX_PKT_SIZE)
	{
		// If we reached a full packet size, send the data in the buffer
		USBCDC_SendDataInBuffer();
	}
}

/** Reads a byte from the USB serial port, if available
 *
 * @return The byte, or -1 if there is nothing available
 */
int16_t USBCDC_ReadByte(void)
{
	// Assume not ready
	int16_t ret = -1;

	// If we have data ready to read out of the USB controller's endpoint buffer, grab it now
	if (cdcRxLen == 0 && cdcRxReady)
	{
		// Flag that we handled the read event
		cdcRxReady = false;

		// Reset our read pointer just in case
		cdcRxPos = 0;

		// Read all of the data out from the USB controller
		cdcRxLen = USBD_GET_PAYLOAD_LEN(EP4);
		USBD_MemCopy(cdcRxBuf, (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP4)), cdcRxLen);

		// We grabbed all of the packet data, so tell the USB controller we're done with it
		USBD_SET_PAYLOAD_LEN(EP4, EP4_MAX_PKT_SIZE);
	}

	// If we have something left in our buffer since the last read, use it
	if (cdcRxLen > 0)
	{
		ret = cdcRxBuf[cdcRxPos++];

		// If we finished reading from the buffer, mark it as finished.
		if (cdcRxPos == cdcRxLen)
		{
			cdcRxPos = 0;
			cdcRxLen = 0;
		}
	}

	return ret;
}

/** Sends out any remaining data in the TX buffer
 *
 */
void USBCDC_Flush(void)
{
	// If we have something waiting to send, send it out now
	if (cdcTxBufPos > 0)
	{
		bool needsZLP = cdcTxBufPos == EP3_MAX_PKT_SIZE;
		USBCDC_SendDataInBuffer();

		// If we are flushing after sending a full packet of data,
		// send a ZLP afterward to indicate end of transmit to host
		if (needsZLP)
		{
			USBCDC_SendDataInBuffer();
		}
	}
}

/** Sends the data in the EP3 TX buffer to the host
 *
 */
static void USBCDC_SendDataInBuffer(void)
{
	// Wait for any previous transmit to finish first
	while (cdcTxActive);

	// Send out the packet
	USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3)), cdcTxBuf, cdcTxBufPos);
	cdcTxActive = true;
	USBD_SET_PAYLOAD_LEN(EP3, cdcTxBufPos);

	// Reset our buffer position; we've given all the data to the USB controller
	cdcTxBufPos = 0;
}

/** IRQ handler called when USB endpoint 3 is ready (CDC TX data finished transferring)
 *
 */
void EP3_Handler(void)
{
	// All we have to do is flag that we no longer have an active transmission.
	// USBCDC_SendDataInBuffer() will handle the rest.
	cdcTxActive = false;
}

/** IRQ handler called when USB endpoint 4 is ready (CDC RX data ready to read)
 *
 */
void EP4_Handler(void)
{
	// Verify the toggle has changed. If it's still the same, re-request the data.
	if (ep4OutToggle == (USBD->EPSTS0 & USBD_EPSTS0_EPSTS4_Msk))
	{
		USBD_SET_PAYLOAD_LEN(EP4, EP4_MAX_PKT_SIZE);
	}
	// Toggle changed, so we're good to go. Toggle for next time and flag that we
	// have data ready to read. USBCDC_ReadByte() will handle the rest.
	else
	{
		ep4OutToggle = USBD->EPSTS0 & USBD_EPSTS0_EPSTS4_Msk;
		cdcRxReady = true;
	}
}

/** Main IRQ handler for USB device controller
 *
 */
void USBD_IRQHandler(void)
{
	uint32_t intStatus = USBD_GET_INT_FLAG();
	uint32_t busState = USBD_GET_BUS_STATE();

	// Bus interrupt
	if (intStatus & USBD_INTSTS_BUSIF_Msk)
	{
		// Clear the flag
		USBD_CLR_INT_FLAG(USBD_INTSTS_BUSIF_Msk);

		// The bus was reset - SE0 status for long enough
		if (busState & USBD_STATE_USBRST)
		{
			// Enable USB, reset everything
			USBD_ENABLE_USB();
			USBD_SwReset();
			ep4OutToggle = 0;
		}

		// Entered suspend status (bus idle)
		if (busState & USBD_STATE_SUSPEND)
		{
			// Disable PHY while we're suspended
			USBD_DISABLE_PHY();
		}

		if (busState & USBD_STATE_RESUME)
		{
			// Re-enable USB (mainly just the PHY is what we want)
			USBD_ENABLE_USB();
		}
	}

	// USB event
	if (intStatus & USBD_INTSTS_USBIF_Msk)
	{
		// Setup packet?
		if (intStatus & USBD_INTSTS_SETUP_Msk)
		{
			// Clear the flag...
			USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

			// Stop EP0 and EP1 which are the control endpoints
			USBD_STOP_TRANSACTION(EP0);
			USBD_STOP_TRANSACTION(EP1);

			// Now handle the received setup packet
			USBD_ProcessSetupPacket();
		}

		// EP0 - control in
		if (intStatus & USBD_INTSTS_EPEVT0_Msk)
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EPEVT0_Msk);
			USBD_CtrlIn();
		}

		// EP1 - control out
		if (intStatus & USBD_INTSTS_EPEVT1_Msk)
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EPEVT1_Msk);
			USBD_CtrlOut();
		}

		// Do nothing for EP2 for now

		// EP3 - bulk in for CDC data
		if (intStatus & USBD_INTSTS_EPEVT3_Msk)
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EPEVT3_Msk);
			EP3_Handler();
		}

		// EP4 - bulk out for CDC data
		if (intStatus & USBD_INTSTS_EPEVT4_Msk)
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EPEVT4_Msk);
			EP4_Handler();
		}
	}
}

/** Initializes endpoint buffers in USB SRAM
 *
 */
void USBCDC_InitEndpoints(void)
{
	// First 8 bytes are for setup packets
	USBD->STBUFSEG = SETUP_BUF_BASE;

	// EP0 = control IN, address 0, 64 bytes
	USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
	USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);

	// EP1 = control OUT, address 0, uses same 64 bytes as control IN
	USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
	USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);

	// EP2 = interrupt IN, address 2, 8 bytes
	USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | INT_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);

	// EP3 = bulk IN, address 3, 64 bytes
	USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_IN | BULK_IN_EP_NUM);
	USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);

	// EP4 = bulk OUT, address 4, 64 bytes
	USBD_CONFIG_EP(EP4, USBD_CFG_EPMODE_OUT | BULK_OUT_EP_NUM);
	USBD_SET_EP_BUF_ADDR(EP4, EP4_BUF_BASE);

	// Mark that we are able to receive on EP4
	USBD_SET_PAYLOAD_LEN(EP4, EP4_MAX_PKT_SIZE);
}

/** USB class request callback for CDC device
 *
 */
void USBCDC_ClassRequest(void)
{
	uint8_t buf[8];

	// Grab the setup packet...
	USBD_GetSetupPacket(buf);

	// Device to host?
	if (buf[0] & 0x80)
	{
		switch (buf[1])
		{
		case GET_LINE_CODE:
			if (buf[4] == 0)
			{
				USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)), (uint8_t *)&cdcLineCoding, 7);
			}

			// Data stage
			USBD_SET_DATA1(EP0);
			USBD_SET_PAYLOAD_LEN(EP0, 7);

			// Status stage
			USBD_PrepareCtrlOut(0, 0);
			break;

		default:
			// Stall
			USBD_SET_EP_STALL(EP0);
			USBD_SET_EP_STALL(EP1);
			break;
		}
	}
	// Host to device
	else
	{
		switch (buf[1])
		{
		case SET_CONTROL_LINE_STATE:
			if (buf[4] == 0)
			{
				cdcCtrlSignal = (buf[3] << 8) | buf[2];
			}

			// Status stage
			USBD_SET_DATA1(EP0);
			USBD_SET_PAYLOAD_LEN(EP0, 0);
			break;

		case SET_LINE_CODE:
			if (buf[4] == 0)
			{
				USBD_PrepareCtrlOut((uint8_t *)&cdcLineCoding, 7);
			}

			// Status stage
			USBD_SET_DATA1(EP0);
			USBD_SET_PAYLOAD_LEN(EP0, 0);
			break;

		default:
			// Stall
			USBD_SET_EP_STALL(EP0);
			USBD_SET_EP_STALL(EP1);
			break;
		}
	}
}
