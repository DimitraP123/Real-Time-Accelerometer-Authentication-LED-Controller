#ifndef SPI_DMA_H
#define SPI_DMA_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t x_axis_output_sample;
} accel_sample_t;




void configure_spi0();

void configure_spi_dma(void);

void dma_spi_irq_handler(void);

void spi_dma_start(void);

bool spi_dma_sample_available(void);

bool spi_dma_get_sample(accel_sample_t *one_accelerometer_sample);

uint32_t get_dma_irq_count(void);  // FIX: debug function

void accel_cs_low(void);  // FIX: public CS control
void accel_cs_high(void);  // FIX: public CS control

#endif