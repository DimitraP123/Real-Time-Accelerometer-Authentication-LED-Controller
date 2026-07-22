#include "spi_dma.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <rp2350/resets.h>
#include <rp2350/sio.h>
#include <rp2350/io_bank0.h>
#include <rp2350/pads_bank0.h>
#include <rp2350/spi.h>
#include <rp2350/clocks.h>
#include <rp2350/dma.h>
#include <rp2350/m33.h>

#define CONCAT2(X,Y) X ## Y
#define CONCAT3(X,Y,Z) X ## Y ## Z
#define IO_BANK0_GPIO_CTRL(X) CONCAT3(IO_BANK0_GPIO,X,_CTRL)
#define PADS_BANK0_GPIO(X)    CONCAT2(PADS_BANK0_GPIO,X)

#define DATA_BITS       8 
#define MOTOROLA_CPHA   1
#define MOTOROLA_CPOL   1
#define SPI0_SCLK_LOC   6
#define SPI0_TXD_LOC    7
#define SPI0_CSn_LOC    5
#define SPI0_RXD_LOC    4

#define ACCEL_CS_PIN 5

#define SPI0_RESETS ( RESETS_RESET_SPI0_MASK \
                    | RESETS_RESET_IO_BANK0_MASK \
                    | RESETS_RESET_PADS_BANK0_MASK )

#define SPI_DMA_CHANNEL 0
#define SPI_DMA_IRQ_MASK (1 << SPI_DMA_CHANNEL)

#define DREQ_SPI0_RX 25

#define NUM_BYTES_DMA_TRANSFERS_PER_ACC_SAMPLE 3

#define FIFO_SIZE 64

static uint8_t dma_placed_spi_data_buffer[NUM_BYTES_DMA_TRANSFERS_PER_ACC_SAMPLE];

static accel_sample_t fifo[FIFO_SIZE];
static volatile uint32_t fifo_head = 0;
static volatile uint32_t fifo_tail = 0;

static volatile uint32_t dma_irq_count = 0;


// ---------------------------------------------------------
// FIFO helpers - Ring Buffer Pattern (from lecture notes)
// ---------------------------------------------------------

static bool fifo_push(accel_sample_t one_accelerometer_sample)
{
    uint32_t next = (fifo_head + 1) % FIFO_SIZE;
    if (next == fifo_tail) return false;  // Buffer full
    fifo[fifo_head] = one_accelerometer_sample;
    fifo_head = next;
    return true;
}

static bool fifo_pop(accel_sample_t *one_accelerometer_sample)
{
    if (fifo_head == fifo_tail) return false;  // Buffer empty
    *one_accelerometer_sample = fifo[fifo_tail];
    fifo_tail = (fifo_tail + 1) % FIFO_SIZE;
    return true;
}


// ---------------------------------------------------------
// CS control helpers (public for use in IRQ)
// ---------------------------------------------------------
void accel_cs_low(void)
{
    sio.gpio_out_clr = (1u << ACCEL_CS_PIN);
}

void accel_cs_high(void)
{
    sio.gpio_out_set = (1u << ACCEL_CS_PIN);
}


// ---------------------------------------------------------
// SPI CONFIG
// ---------------------------------------------------------

void configure_spi0(void)
{
    // FIX: Enable peripheral clock for SPI (from lecture notes - required first)
    CLOCKS_CLK_PERI_CTRL =
          CLOCKS_CLK_PERI_CTRL_ENABLE(1)
        | CLOCKS_CLK_PERI_CTRL_AUXSRC(0);

    // FIX: Bring SPI0 out of reset (required before any register access)
    RESETS_RESET_CLR = SPI0_RESETS;
    while ((RESETS_RESET_DONE & SPI0_RESETS) != SPI0_RESETS)
        continue;

    // CS = GPIO, NOT SPI function (manual CS control via SIO)
    IO_BANK0_GPIO_CTRL(ACCEL_CS_PIN) =
        IO_BANK0_GPIO0_CTRL_FUNCSEL(5);  // SIO function
    sio.gpio_oe_set = (1u << ACCEL_CS_PIN);
    accel_cs_high(); // idle high

    // Setup SCLK/MOSI/MISO as SPI function (FUNCSEL = 1)
    const uint32_t io_ctrl = IO_BANK0_GPIO0_CTRL_FUNCSEL(1);

    IO_BANK0_GPIO_CTRL(SPI0_SCLK_LOC) = io_ctrl;  // GPIO 6
    IO_BANK0_GPIO_CTRL(SPI0_TXD_LOC)  = io_ctrl;  // GPIO 7 (MOSI)
    IO_BANK0_GPIO_CTRL(SPI0_RXD_LOC)  = io_ctrl;  // GPIO 4 (MISO)

    // FIX: SPI Mode 3 configuration for LIS3DH (CPOL=1, CPHA=1)
    SPI0_SSPCR0 =
          SPI0_SSPCR0_SCR(14)        // Clock divisor
        | SPI0_SSPCR0_SPH(1)         // CPHA = 1 (phase)
        | SPI0_SSPCR0_SPO(1)         // CPOL = 1 (polarity)
        | SPI0_SSPCR0_FRF(0)         // Motorola format
        | SPI0_SSPCR0_DSS(DATA_BITS - 1);  // 8-bit data

    SPI0_SSPCPSR = SPI0_SSPCPSR_CPSDVSR(2);  // Clock prescaler

    SPI0_SSPCR1_SET = SPI0_SSPCR1_SSE(1);  // Enable SPI

    // FIX: Enable RX DMA requests (critical for DMA to be paced by SPI RX FIFO)
    SPI0_SSPDMACR = SPI0_SSPDMACR_RXDMAE(1);
}


// ---------------------------------------------------------
// DMA CONFIG
// ---------------------------------------------------------

void configure_spi_dma(void)
{
    // FIX: Bring DMA out of reset
    RESETS_RESET_CLR = RESETS_RESET_DMA_MASK;
    while (!(RESETS_RESET_DONE & RESETS_RESET_DMA_MASK))
        continue;

    // FIX: Configure DMA channel 0 for SPI RX
    DMA_CH0_READ_ADDR  = (uint32_t)&SPI0_SSPDR;  // Read from SPI RX FIFO
    DMA_CH0_WRITE_ADDR = (uint32_t)dma_placed_spi_data_buffer;  // Write to buffer

    DMA_CH0_TRANS_COUNT =
         DMA_CH0_TRANS_COUNT_COUNT(NUM_BYTES_DMA_TRANSFERS_PER_ACC_SAMPLE);

    // FIX: Configure DMA control (paced by SPI RX DREQ, 8-bit transfers, auto-increment write)
    DMA_CH0_CTRL_TRIG =
          DMA_CH0_CTRL_TRIG_TREQ_SEL(DREQ_SPI0_RX)   // Pace transfers with SPI RX events
        | DMA_CH0_CTRL_TRIG_INCR_WRITE(1)            // Auto-increment write address
        | DMA_CH0_CTRL_TRIG_DATA_SIZE(0);            // 8-bit transfers

    // FIX: Enable DMA interrupt for channel 0 and NVIC IRQ 10
    DMA_INTE0 = (1 << SPI_DMA_CHANNEL);
    m33.nvic_iser0 |= (1 << 10);

    fifo_head = fifo_tail = 0;
    printf("[spi_dma] DMA configured and ready\n");
}


// ---------------------------------------------------------
// DMA IRQ HANDLER - Processes completed SPI RX transfers
// ---------------------------------------------------------

void dma_spi_irq_handler(void)
{
    // FIX: IMMEDIATE diagnostic - are we even entering the handler?
    static uint32_t irq_entry_count = 0;
    irq_entry_count++;
    
    if (irq_entry_count == 1)
    {
        printf("[CRITICAL] DMA IRQ FIRED FOR THE FIRST TIME!\n");
    }
    
    if (irq_entry_count <= 5)
    {
        printf("[DMA_IRQ #%lu] Handler entered\n", irq_entry_count);
    }
    
    DMA_INTS0 = SPI_DMA_IRQ_MASK;  // Clear the interrupt
    dma_irq_count++;

    // Extract X-axis acceleration from 3-byte SPI response
    accel_sample_t one_sample;
    one_sample.x_axis_output_sample =
        (int16_t)((dma_placed_spi_data_buffer[2] << 8) |
                   dma_placed_spi_data_buffer[1]);

    if (irq_entry_count <= 5)
    {
        printf("[DMA_IRQ #%lu] raw[0]=%02X [1]=%02X [2]=%02X -> X=%d\n",
               irq_entry_count,
               dma_placed_spi_data_buffer[0],
               dma_placed_spi_data_buffer[1],
               dma_placed_spi_data_buffer[2],
               one_sample.x_axis_output_sample);
    }

    // Push sample into ring buffer FIFO
    fifo_push(one_sample);

    // FIX: Re-arm DMA BEFORE sending next command
    DMA_CH0_WRITE_ADDR = (uint32_t)dma_placed_spi_data_buffer;
    DMA_CH0_TRANS_COUNT =
        DMA_CH0_TRANS_COUNT_COUNT(NUM_BYTES_DMA_TRANSFERS_PER_ACC_SAMPLE);
    DMA_CH0_CTRL_TRIG_SET = DMA_CH0_CTRL_TRIG_EN(1);

    // Send next read command
    accel_cs_low();
    SPI0_SSPDR = 0xE8;
    SPI0_SSPDR = 0x00;
    SPI0_SSPDR = 0x00;
    accel_cs_high();
}

void DMA_IRQ_0_Handler(void) __attribute__((alias("dma_spi_irq_handler")));


// ---------------------------------------------------------
// PUBLIC API
// ---------------------------------------------------------

void spi_dma_start(void)
{
    printf("[spi_dma] Starting DMA...\n");
    
    // FIX: ARM DMA FIRST before sending initial read command
    DMA_CH0_WRITE_ADDR = (uint32_t)dma_placed_spi_data_buffer;
    DMA_CH0_TRANS_COUNT =
        DMA_CH0_TRANS_COUNT_COUNT(NUM_BYTES_DMA_TRANSFERS_PER_ACC_SAMPLE);
    DMA_CH0_CTRL_TRIG_SET = DMA_CH0_CTRL_TRIG_EN(1);

    // FIX: NOW send initial read command (DMA is armed)
    accel_cs_low();
    SPI0_SSPDR = 0xE8;    // READ | MULTI | OUT_X_L
    SPI0_SSPDR = 0x00;
    SPI0_SSPDR = 0x00;
    accel_cs_high();
}

bool spi_dma_sample_available(void)
{
    return fifo_head != fifo_tail;  // Consumer checks if FIFO not empty
}

bool spi_dma_get_sample(accel_sample_t *sample)
{
    return fifo_pop(sample);  // Consumer side of ring buffer
}

uint32_t get_dma_irq_count(void)
{
    return dma_irq_count;
}