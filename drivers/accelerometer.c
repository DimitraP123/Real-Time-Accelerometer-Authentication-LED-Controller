#include <stdint.h>
#include <stdio.h>
#include <rp2350/spi.h>
#include <rp2350/sio.h>
#include <rp2350/io_bank0.h>

#define WRITE_CMD   0x00
#define CTRL_REG1   0x20
#define CTRL_REG4   0x23
#define WHO_AM_I    0x0F

#define ACCEL_CS_PIN 5


// ---------------------------------------------------------
// CS control helpers
// ---------------------------------------------------------
static inline void accel_cs_low(void)
{
    sio.gpio_out_clr = (1u << ACCEL_CS_PIN);
}

static inline void accel_cs_high(void)
{
    sio.gpio_out_set = (1u << ACCEL_CS_PIN);
}


// ---------------------------------------------------------
// Basic SPI write (transmit only, discard response)
// ---------------------------------------------------------
static void accel_spi_write(uint8_t byte)
{
    while (!(SPI0_SSPSR & SPI0_SSPSR_TNF_MASK))
        ;
    SPI0_SSPDR = byte;

    while (!(SPI0_SSPSR & SPI0_SSPSR_RNE_MASK))
        ;
    volatile uint32_t discard = SPI0_SSPDR;
    (void)discard;
}

// ---------------------------------------------------------
// SPI read (transmit and return response)
// ---------------------------------------------------------
static uint8_t accel_spi_read(uint8_t byte)
{
    while (!(SPI0_SSPSR & SPI0_SSPSR_TNF_MASK))
        ;
    SPI0_SSPDR = byte;

    while (!(SPI0_SSPSR & SPI0_SSPSR_RNE_MASK))
        ;
    uint32_t result = SPI0_SSPDR;  // FIX: Read from SSPDR (data register), not SSPSR
    return (uint8_t)result;
}


// ---------------------------------------------------------
// Register write wrapped in CS control
// ---------------------------------------------------------
static void accel_write_reg(uint8_t reg, uint8_t value)
{
    accel_cs_low();
    accel_spi_write(WRITE_CMD | reg);
    accel_spi_write(value);
    accel_cs_high();
}

// ---------------------------------------------------------
// Register read wrapped in CS control
// ---------------------------------------------------------
static uint8_t accel_read_reg(uint8_t reg)
{
    accel_cs_low();
    accel_spi_write(0x80 | reg);  // Set read bit (0x80)
    uint8_t result = accel_spi_read(0x00);  // Send dummy byte, get response
    accel_cs_high();
    return result;
}


// ---------------------------------------------------------
// accelerometer_init()
// ---------------------------------------------------------
void accelerometer_init(void)
{
    // Configure CS pin as GPIO output
    IO_BANK0_GPIO5_CTRL = 
        IO_BANK0_GPIO0_CTRL_FUNCSEL(5);

    sio.gpio_oe_set = (1u << ACCEL_CS_PIN);
    accel_cs_high();

    // Try a simple SPI transaction to see if accelerometer responds
    printf("\nREADING_WHO_AM_I:");
    
    accel_cs_low();
    // Send read command for WHO_AM_I (0x0F)
    accel_spi_write(0x8F);  // Read (0x80) | WHO_AM_I (0x0F)
    uint8_t response = accel_spi_read(0x00);  // Get response
    accel_cs_high();
    
    printf("0x%02X\n", response);

    // Configure accelerometer control registers
    accel_write_reg(CTRL_REG1, 0x57);  // 100 Hz, XYZ enable
    accel_write_reg(CTRL_REG4, 0x08);  // High-resolution mode
}









//Accel_driver.c file

#include "accel_driver.h"
#include "sio0.h"

#include <stdbool.h>
#include <stdint.h>
#include <rp2350/dma.h>
#include <rp2350/resets.h>
#include <rp2350/spi.h>




/* ------------ Accelerometer Register Definitions ------------ */
#define READ_CMD       (1 << 7)
#define WRITE_CMD      0x00
#define RW_MULTIPLE    (1 << 6)
#define OUT_X_L_REG    0x28

/* DMA channel used for accelerometer RX */
#define ACCEL_DMA_CH   1

/* FIFO size */
#define FIFO_SIZE 64

/* Working DMA buffer */
static volatile uint8_t accel_raw[6];

/* DMA state flags */
static volatile bool dma_in_progress = false;

/* ------------ Circular FIFO Buffer ------------ */
/* UPDATED STRUCT: holds x, y, z */

static accel_sample_t fifo[FIFO_SIZE];
static volatile uint32_t fifo_head = 0;
static volatile uint32_t fifo_tail = 0;

/* Push sample into circular FIFO */
static bool fifo_push(accel_sample_t s)
{
    uint32_t next = (fifo_head + 1) % FIFO_SIZE;

    if (next == fifo_tail)
        return false;   // FIFO full

    fifo[fifo_head] = s;
    fifo_head = next;
    return true;
}

/* Pop sample from circular FIFO */
static bool fifo_pop(accel_sample_t *s)
{
    if (fifo_head == fifo_tail)
        return false;   // empty

    *s = fifo[fifo_tail];
    fifo_tail = (fifo_tail + 1) % FIFO_SIZE;
    return true;
}

/* Clear SPI0 RX FIFO */
static void clear_fifo(void)
{
    uint8_t temp;
    while (spi0_read(&temp));
}



void configure_accelerometer(void)
{
    uint8_t temp;

    /* Enable DMA peripheral */
    RESETS_RESET_CLR = RESETS_RESET_DMA_MASK;
    while (!(RESETS_RESET_DONE & RESETS_RESET_DMA_MASK));

    /* Clear any leftover SPI words */
    clear_fifo();

    /* CTRL_REG1 = 0x20 → 0x77 (400 Hz, XYZ enable) */
    while (!spi0_write(WRITE_CMD | 0x20));
    while (!spi0_write(0x77));
    while (!spi0_read(&temp));

    /* Enable SPI0 RX DMA */
    SPI0_SSPDMACR |= SPI0_SSPDMACR_RXDMAE(1);
}



void accelerometer_request_data(void)
{
    uint8_t temp;

    if (dma_in_progress)
        return;

    dma_in_progress = true;

    clear_fifo();

    /* Send READ command (multi-byte starting at OUT_X_L) */
    while (!spi0_write(READ_CMD | RW_MULTIPLE | OUT_X_L_REG));

    /* Dummy clock to start pipeline */
    while (!spi0_write(0x00));
    while (!spi0_read(&temp));

    /* DMA reads 6 bytes: X_L, X_H, Y_L, Y_H, Z_L, Z_H */
    DMA_CH1_READ_ADDR = (uint32_t)&SPI0_SSPDR;
    DMA_CH1_WRITE_ADDR = (uint32_t)accel_raw;

    DMA_CH1_TRANS_COUNT =
        DMA_CH1_TRANS_COUNT_MODE(0) |
        DMA_CH1_TRANS_COUNT_COUNT(6);

    DMA_CH1_CTRL_TRIG =
        DMA_CH1_CTRL_TRIG_READ_ERROR(1) |
        DMA_CH1_CTRL_TRIG_WRITE_ERROR(1) |
        DMA_CH1_CTRL_TRIG_TREQ_SEL(25) |   // SPI0 RX DREQ
        DMA_CH1_CTRL_TRIG_INCR_WRITE(1) |
        DMA_CH1_CTRL_TRIG_INCR_READ(0) |
        DMA_CH1_CTRL_TRIG_DATA_SIZE(0) |
        DMA_CH1_CTRL_TRIG_EN(1);

    /* Feed extra dummy bytes to keep SPI shifting */
    for (int i = 0; i < 6; i++)
    {
        while (!spi0_write(0x00));
    }
}



bool accelerometer_get_data(accel_sample_t *out)
{
    if (!out)
        return false;

    /* If DMA still running, see if it's done */
    if (dma_in_progress)
    {
        if (!(DMA_CH1_CTRL_TRIG & DMA_CH1_CTRL_TRIG_BUSY_MASK))
        {
            /* DMA finished → decode XYZ */
            int16_t x = (int16_t)((accel_raw[1] << 8) | accel_raw[0]);
            int16_t y = (int16_t)((accel_raw[3] << 8) | accel_raw[2]);
            int16_t z = (int16_t)((accel_raw[5] << 8) | accel_raw[4]);

            accel_sample_t sample;
            sample.x_axis = x;
            sample.y_axis = y;
            sample.z_axis = z;

            fifo_push(sample);
            dma_in_progress = false;
        }
        else
        {
            /* DMA still busy → no new data */
            return false;
        }
    }

    /* Pop from FIFO */
    return fifo_pop(out);
}


bool accelerometer_data_available(void)
{
    return fifo_head != fifo_tail;
}






