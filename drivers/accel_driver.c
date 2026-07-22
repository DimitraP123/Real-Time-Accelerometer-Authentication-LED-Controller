#include "accel_driver.h"
#include "spi0.h"

#include <stdbool.h>
#include <stdint.h>
#include <rp2350/dma.h>
#include <rp2350/resets.h>
#include <rp2350/spi.h>

#include <rp2350/resets.h>
#include <rp2350/sio.h>
#include <rp2350/io_bank0.h>
#include <rp2350/pads_bank0.h>
#include <rp2350/clocks.h>

#define CONCAT2(X,Y) X ## Y
#define CONCAT3(X,Y,Z) X ## Y ## Z
#define IO_BANK0_GPIO_CTRL(X) CONCAT3(IO_BANK0_GPIO,X,_CTRL)
#define PADS_BANK0_GPIO(X) CONCAT2(PADS_BANK0_GPIO,X)

#define DATA_BITS 8 
#define MOTOROLA_CPHA 1
#define MOTOROLA_CPOL 1
#define SPI0_SCLK_LOC 6
#define SPI0_TXD_LOC 7
#define SPI0_CSn_LOC 5
#define SPI0_RXD_LOC 4

#define SPI0_RESETS ( RESETS_RESET_SPI0_MASK \ | RESETS_RESET_IO_BANK0_MASK \ | RESETS_RESET_PADS_BANK0_MASK )

#define READ_CMD       (1 << 7)
#define WRITE_CMD      0x00
#define RW_MULTIPLE    (1 << 6)
#define OUT_X_L_REG    0x28

#define ACCEL_DMA_CH   1

#define FIFO_SIZE 64

static volatile uint8_t accel_raw[6];

static volatile bool dma_in_progress = false;

static accel_sample_t fifo[FIFO_SIZE];
static volatile uint32_t fifo_head = 0;
static volatile uint32_t fifo_tail = 0;

static bool fifo_push(accel_sample_t s)
{
    uint32_t next = (fifo_head + 1) % FIFO_SIZE;

    if (next == fifo_tail)
        return false;  

    fifo[fifo_head] = s;
    fifo_head = next;
    return true;
}

static bool fifo_pop(accel_sample_t *s)
{
    if (fifo_head == fifo_tail)
        return false; 

    *s = fifo[fifo_tail];
    fifo_tail = (fifo_tail + 1) % FIFO_SIZE;
    return true;
}

static void clear_fifo(void)
{
    uint8_t temp;
    while (spi0_read(&temp));
}

void configure_accelerometer(void)
{
    uint8_t temp;

    RESETS_RESET_CLR = RESETS_RESET_DMA_MASK;
    while (!(RESETS_RESET_DONE & RESETS_RESET_DMA_MASK));

    clear_fifo();

    while (!spi0_write(WRITE_CMD | 0x20));
    while (!spi0_write(0x77));
    while (!spi0_read(&temp));

    SPI0_SSPDMACR |= SPI0_SSPDMACR_RXDMAE(1);
}

void accel_data_request(void)
{
    uint8_t temp;

    if (dma_in_progress)
        return;

    dma_in_progress = true;

    clear_fifo();

    while (!spi0_write(READ_CMD | RW_MULTIPLE | OUT_X_L_REG));

    while (!spi0_write(0x00));
    while (!spi0_read(&temp));

    DMA_CH1_READ_ADDR = (uint32_t)&SPI0_SSPDR;
    DMA_CH1_WRITE_ADDR = (uint32_t)accel_raw;

    DMA_CH1_TRANS_COUNT =
        DMA_CH1_TRANS_COUNT_MODE(0) |
        DMA_CH1_TRANS_COUNT_COUNT(6);

    DMA_CH1_CTRL_TRIG =
        DMA_CH1_CTRL_TRIG_READ_ERROR(1) |
        DMA_CH1_CTRL_TRIG_WRITE_ERROR(1) |
        DMA_CH1_CTRL_TRIG_TREQ_SEL(25) |   
        DMA_CH1_CTRL_TRIG_INCR_WRITE(1) |
        DMA_CH1_CTRL_TRIG_INCR_READ(0) |
        DMA_CH1_CTRL_TRIG_DATA_SIZE(0) |
        DMA_CH1_CTRL_TRIG_EN(1);

    for (int i = 0; i < 6; i++)
    {
        while (!spi0_write(0x00));
    }
}

bool accel_data_retrieve(accel_sample_t *out)
{
    if (!out)
    {
        return false;
    }
        
    if (dma_in_progress)
    {
        if (!(DMA_CH1_CTRL_TRIG & DMA_CH1_CTRL_TRIG_BUSY_MASK))
        {

            int16_t x = (int16_t)((accel_raw[1] << 8) | accel_raw[0]);
            int16_t y = (int16_t)((accel_raw[3] << 8) | accel_raw[2]);
            int16_t z = (int16_t)((accel_raw[5] << 8) | accel_raw[4]);

            x >>= 4;   
            y >>= 4;   
            z >>= 4;   

            accel_sample_t sample;
            sample.x_axis = x;
            sample.y_axis = y;
            sample.z_axis = z;

            fifo_push(sample);
            dma_in_progress = false;
        }

        else
        {
            return false;
        }
    }

    return fifo_pop(out);
}

bool accelerometer_data_available(void)
{
    return fifo_head != fifo_tail;
}