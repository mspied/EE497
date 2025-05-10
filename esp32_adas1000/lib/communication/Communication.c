/***************************************************************************//**
 *   @file   Communication.c
 *   @brief  Implementation of Communication Driver.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2012(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
********************************************************************************
 *   SVN Revision: 570
*******************************************************************************/

/******************************************************************************/
/* Include Files                                                              */
/******************************************************************************/
#include "Communication.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

#define TAG "COMM"

adas_target_t current_target = ADAS_PRIMARY;
static spi_device_handle_t spi_handle;

// Helper: assert and deassert CS based on current_target
static void assert_cs()
{
    gpio_set_level((current_target == ADAS_PRIMARY) ? ADAS_PRIMARY_CS : ADAS_SECONDARY_CS, 0);
}

static void deassert_cs()
{
    gpio_set_level((current_target == ADAS_PRIMARY) ? ADAS_PRIMARY_CS : ADAS_SECONDARY_CS, 1);
}

/***************************************************************************//**
 * @brief Initializes the SPI communication peripheral.
 *
 * @param lsbFirst - Transfer format (0 or 1).
 *                   Example: 0x0 - MSB first.
 *                            0x1 - LSB first.
 * @param clockFreq - SPI clock frequency (Hz).
 *                    Example: 1000 - SPI clock frequency is 1 kHz.
 * @param clockPol - SPI clock polarity (0 or 1).
 *                   Example: 0x0 - idle state for SPI clock is low.
 *	                          0x1 - idle state for SPI clock is high.
 * @param clockPha - SPI clock phase (0 or 1).
 *                   Example: 0x0 - data is latched on the leading edge of SPI
 *                                  clock and data changes on trailing edge.
 *                            0x1 - data is latched on the trailing edge of SPI
 *                                  clock and data changes on the leading edge.
 *
 * @return 0 - Initialization failed, 1 - Initialization succeeded.
*******************************************************************************/
unsigned char SPI_Init(unsigned char lsbFirst,
                       unsigned long clockFreq,
                       unsigned char clockPol,
                       unsigned char clockPha)
{
    esp_err_t ret;

    // SPI bus configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = clockFreq,
        .mode = ((clockPol << 1) | clockPha),
        .spics_io_num = -1,
        .queue_size = 1,
        .flags = (lsbFirst ? SPI_DEVICE_BIT_LSBFIRST : 0),
    };

    ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %d", ret);
        return 0;
    }

    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %d", ret);
        return 0;
    }

    // Configure CS pins
    gpio_config_t cs_conf = {
        .pin_bit_mask = (1ULL << ADAS_PRIMARY_CS) | (1ULL << ADAS_SECONDARY_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cs_conf);
    gpio_set_level(ADAS_PRIMARY_CS, 1);
    gpio_set_level(ADAS_SECONDARY_CS, 1);

    // Configure DRDY pins as input
    gpio_config_t drdy_conf = {
        .pin_bit_mask = (1ULL << ADAS_PRIMARY_DRDY) | (1ULL << ADAS_SECONDARY_DRDY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&drdy_conf);

    return 1;
}

/***************************************************************************//**
 * @brief Writes data to SPI.
 *
 * @param data - data represents the write buffer.
 * @param bytesNumber - Number of bytes to write.
 *
 * @return Number of written bytes.
*******************************************************************************/
unsigned char SPI_Write(unsigned char* data,
                        unsigned char bytesNumber)
{
    spi_transaction_t t = {
        .length = bytesNumber * 8,
        .tx_buffer = data,
        .rx_buffer = NULL
    };

    assert_cs();
    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    deassert_cs();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI_Write failed: %d", ret);
        return 0;
    }

    return bytesNumber;
}

/***************************************************************************//**
 * @brief Reads data from SPI.
 *
 * @param data - Data represents the read buffer.
 * @param bytesNumber - Number of bytes to read.
 *
 * @return Number of read bytes.
*******************************************************************************/
unsigned char SPI_Read(unsigned char* data,
                       unsigned char bytesNumber)
{
    uint8_t dummy[bytesNumber];
    memset(dummy, 0xFF, bytesNumber);

    spi_transaction_t t = {
        .length = bytesNumber * 8,
        .tx_buffer = dummy,
        .rx_buffer = data
    };

    assert_cs();
    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    deassert_cs();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI_Read failed: %d", ret);
        return 0;
    }

    return bytesNumber;
}

/***************************************************************************//**
 * @brief Checks if the target is ready to send data or recieve commands.
 *
 * @param target - Device to check
				ADAS_PRIMARY or ADAS_SECONDARY
 *
 * @return 0 - Device is not ready, 1 - Device is ready.
*******************************************************************************/

bool is_data_ready(adas_target_t target)
{
    gpio_num_t pin = (target == ADAS_PRIMARY) ? ADAS_PRIMARY_DRDY : ADAS_SECONDARY_DRDY;
    return gpio_get_level(pin) == 0;  // DRDY is active low
}
