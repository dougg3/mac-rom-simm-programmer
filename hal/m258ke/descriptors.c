/****************************************************************************//**
 * @file     descriptors.c
 * @version  V0.10
 * @brief    USBD descriptors
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include "usbcdc_hw.h"

/// VID and PID of our device
#define USB_VID        0x16D0
#define USB_PID        0x06AA

/// Device descriptor
const uint8_t deviceDescriptor[] = {
    LEN_DEVICE,       // bLength
    DESC_DEVICE,      // bDescriptorType
    0x10, 0x01,       // bcdUSB
    0x02,             // bDeviceClass
    0x00,             // bDeviceSubClass
    0x00,             // bDeviceProtocol
    EP0_MAX_PKT_SIZE, // bMaxPacketSize0
    // idVendor
    USB_VID & 0x00FF, (USB_VID & 0xFF00) >> 8,
    // idProduct
    USB_PID & 0x00FF, (USB_PID & 0xFF00) >> 8,
    0x02, 0x00, // bcdDevice
    0x01,       // iManufacturer
    0x02,       // iProduct
    0x03,       // iSerialNumber
    0x01        // bNumConfigurations
};

/// Config descriptor (includes interface, endpoint descriptors)
const uint8_t configDescriptor[] =
{
    LEN_CONFIG,     // bLength
    DESC_CONFIG,    // bDescriptorType
    0x3E, 0x00,     // wTotalLength
    0x02,           // bNumInterfaces
    0x01,           // bConfigurationValue
    0x00,           // iConfiguration
    0xC0,           // bmAttributes
    0xFA,           // MaxPower

    // Interface descriptor: communication class interface
    LEN_INTERFACE,  // bLength
    DESC_INTERFACE, // bDescriptorType
    0x00,           // bInterfaceNumber
    0x00,           // bAlternateSetting
    0x01,           // bNumEndpoints
    0x02,           // bInterfaceClass
    0x02,           // bInterfaceSubClass
    0x01,           // bInterfaceProtocol
    0x00,           // iInterface

    // Header functional descriptor
    0x05,           // Size of the descriptor, in bytes
    0x24,           // CS_INTERFACE descriptor type
    0x00,           // Header functional descriptor subtype
    0x10, 0x01,     // Communication device compliant to the communication spec. ver. 1.10

    // Abstract control management functional descriptor
    0x04,           // Size of the descriptor, in bytes
    0x24,           // CS_INTERFACE descriptor type
    0x02,           // Abstract control management functional descriptor subtype
    0x06,           // bmCapabilities

    // Union functional descriptor
    0x05,           // bLength
    0x24,           // bDescriptorType: CS_INTERFACE descriptor type
    0x06,           // bDescriptorSubType
    0x00,           // bMasterInterface
    0x01,           // bSlaveInterface0

    // Endpoint descriptor
    LEN_ENDPOINT,                   // bLength
    DESC_ENDPOINT,                  // bDescriptorType
    (EP_INPUT | INT_IN_EP_NUM),     // bEndpointAddress
    EP_INT,                         // bmAttributes
    EP2_MAX_PKT_SIZE, 0x00,         // wMaxPacketSize
    0xFF,                           // bInterval

    // Interface descriptor: data class interface
    LEN_INTERFACE,  // bLength
    DESC_INTERFACE, // bDescriptorType
    0x01,           // bInterfaceNumber
    0x00,           // bAlternateSetting
    0x02,           // bNumEndpoints
    0x0A,           // bInterfaceClass
    0x00,           // bInterfaceSubClass
    0x00,           // bInterfaceProtocol
    0x00,           // iInterface

    // Endpoint descriptor
    LEN_ENDPOINT,                   // bLength
    DESC_ENDPOINT,                  // bDescriptorType
    (EP_OUTPUT | BULK_OUT_EP_NUM),  // bEndpointAddress
    EP_BULK,                        // bmAttributes
    EP4_MAX_PKT_SIZE, 0x00,         // wMaxPacketSize
    0x01,                           // bInterval

    // Endpoint descriptor
    LEN_ENDPOINT,                   // bLength
    DESC_ENDPOINT,                  // bDescriptorType
    (EP_INPUT | BULK_IN_EP_NUM),    // bEndpointAddress
    EP_BULK,                        // bmAttributes
    EP3_MAX_PKT_SIZE, 0x00,         // wMaxPacketSize
    0x01,                           // bInterval
};


/// Language descriptor
const uint8_t languageStringDescriptor[4] =
{
    4,              // bLength
    DESC_STRING,    // bDescriptorType
    0x09, 0x04      // English (United States)
};

/// Vendor string descriptor
const uint8_t vendorStringDescriptor[] =
{
    22,
    DESC_STRING,
    'D', 0,
    'o', 0,
    'u', 0,
    'g', 0,
    ' ', 0,
    'B', 0,
    'r', 0,
    'o', 0,
    'w', 0,
    'n', 0,
};

/// Product string descriptor
const uint8_t productStringDescriptor[] =
{
    48,             // bLength
    DESC_STRING,    // bDescriptorType
    'M', 0,
    'a', 0,
    'c', 0,
    ' ', 0,
    'R', 0,
    'O', 0,
    'M', 0,
    ' ', 0,
    'S', 0,
    'I', 0,
    'M', 0,
    'M', 0,
    ' ', 0,
    'P', 0,
    'r', 0,
    'o', 0,
    'g', 0,
    'r', 0,
    'a', 0,
    'm', 0,
    'm', 0,
    'e', 0,
    'r', 0,
};

/// Serial number string descriptor
const uint8_t serialStringDescriptor[] =
{
    4,             // bLength
    DESC_STRING,    // bDescriptorType
    '0', 0,
};

/// Array of string descriptors
const uint8_t * const stringDescriptors[4] =
{
    languageStringDescriptor,
    vendorStringDescriptor,
    productStringDescriptor,
    serialStringDescriptor
};

/// Descriptor info used by usbd.c
const S_USBD_INFO_T gsInfo =
{
    deviceDescriptor,
    configDescriptor,
    stringDescriptors,
};

