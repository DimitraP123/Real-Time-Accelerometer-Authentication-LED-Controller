/* 
 | Programmer: Dimitra Pando
 |
 | This program implements an asynchronous FSM that receives commands
 | from the terminal and responds to them. The commands manually configure
 | an accelerometer, read the WHOMAI register and read acceleromter values.
 | The FSM waits for the user to enter all inputs before completing the final
 | action.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "spi0.h"
#include "usbcdc.h"

#define READ_CMD     (1<<7)
#define WRITE_CMD    0
#define RW_MULTIPLE  (1<<6)
#define DUMMY        0x00

#define ACCELERATION_X_AXIS  0x28
#define ACCELERATION_Y_AXIS  0x2A
#define ACCELERATION_Z_AXIS  0x2C

static void manual_clear_fifo(void);
static uint8_t manual_read_reg(uint8_t reg);
static void manual_write_reg(uint8_t reg, uint8_t value);
static int16_t manual_read_axis(uint8_t low_reg);
static uint8_t hex2_to_1byte(char high_nibble_char, char low_nibble_char);
static void process_command(char *cmd);

typedef enum {
    IDLE = 0,
    RXX_ENTER,
    WXXYY_ENTER,
    C_ENTER,
    X_ENTER,
    Y_ENTER,
    Z_ENTER
} fsm_state_t;

static fsm_state_t state = IDLE;

static char input_buffer[16];
static uint8_t reg_addr;
static uint8_t reg_value;

int main(void)
{
    configure_spi0();
    configure_usbcdc();

    int index = 0;

    while (1)
    {
        char input_char;

        if (usbcdc_getchar(&input_char)) 
        {
            if (input_char == '\r' || input_char == '\n') 
            {
                input_buffer[index] = '\0';
                process_command(input_buffer);
                index = 0;
            } 
            
            else 
            {
                if (index < (int)sizeof(input_buffer)-1)
                    input_buffer[index++] = input_char;
            }
        }

        switch (state)
        {
            case RXX_ENTER:
            {
                uint8_t v = manual_read_reg(reg_addr);
                printf("The value of register 0x%02X is 0x%02X\r\n", reg_addr, v);
                state = IDLE;
                break;
            }

            case WXXYY_ENTER:
            {
                manual_write_reg(reg_addr, reg_value);
                printf("The value 0x%02X was written to register 0x%02X\r\n", reg_value, reg_addr);
                state = IDLE;
                break;
            }

            case C_ENTER:
            {
                manual_clear_fifo();
                printf("SPI FIFO cleared.\r\n");
                state = IDLE;
                break;
            }
                
            case X_ENTER:
            {
                printf("Acceleration on X axis is 0x%04X\r\n", (uint16_t)manual_read_axis(ACCELERATION_X_AXIS));
                state = IDLE;
                break;
            }
                
            case Y_ENTER:
            {
                printf("Acceleration on Y axis is 0x%04X\r\n", (uint16_t)manual_read_axis(ACCELERATION_Y_AXIS));
                state = IDLE;
                break;
            }
                
            case Z_ENTER:
            {
                printf("Acceleration on Z axis is 0x%04X\r\n",
                    (uint16_t)manual_read_axis(ACCELERATION_Z_AXIS));
                state = IDLE;
                break;
            }  

            default:
                break;
        }
    }
}

static void manual_clear_fifo(void)
{
    uint8_t temp;
    while (spi0_read(&temp));
}

static uint8_t manual_read_reg(uint8_t reg)
{
    uint8_t throwaway, data;

    manual_clear_fifo();

    while (!spi0_write(READ_CMD | reg));
    while (!spi0_write(DUMMY));
    while (!spi0_read(&throwaway));
    while (!spi0_read(&data));

    return data;
}

static void manual_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t dummy;

    manual_clear_fifo();

    while (!spi0_write(WRITE_CMD | reg));
    while (!spi0_write(value));
    while (spi0_read(&dummy));
}

static int16_t manual_read_axis(uint8_t low_reg)
{
    uint8_t lo = manual_read_reg(low_reg);
    uint8_t hi = manual_read_reg(low_reg + 1);
    
    return (int16_t)(((int16_t)hi << 8) | lo);
}

static uint8_t hex2_to_1byte(char high_nibble_char, char low_nibble_char)
{
    uint8_t high_nibble_value;
    uint8_t low_nibble_value;
    uint8_t combined_byte;

    if (high_nibble_char <= '9')
    {
        high_nibble_value = high_nibble_char - '0';
    }

    else
    {
        high_nibble_value = (high_nibble_char & ~0x20) - 'A' + 10;
    }

    if (low_nibble_char <= '9')
    {
        low_nibble_value = low_nibble_char - '0';
    }

    else
    {
        low_nibble_value = (low_nibble_char & ~0x20) - 'A' + 10;
    }

    combined_byte = (uint8_t)((high_nibble_value << 4) | low_nibble_value);

    return combined_byte;
}


static void process_command(char *cmd)
{
    int len = strlen(cmd);

    if (len == 0) 
    {
        return;
    }

    else if (cmd[0] == 'R' && len == 3) 
    {
        reg_addr = hex2_to_1byte(cmd[1], cmd[2]);
        state = RXX_ENTER;
        return;
    }

    else if (cmd[0] == 'W' && len == 5) 
    {
        reg_addr  = hex2_to_1byte(cmd[1], cmd[2]);
        reg_value = hex2_to_1byte(cmd[3], cmd[4]);
        state = WXXYY_ENTER;
        return;
    }

    else if (cmd[0] == 'C') 
    {
        state = C_ENTER;
        return;
    }

    else if (cmd[0] == 'x')
    {
        state = X_ENTER;
        return;
    }

    else if (cmd[0] == 'y')
    {
        state = Y_ENTER;
        return;
    }

    else if (cmd[0] == 'z')
    {
        state = Z_ENTER;
        return;
    }
}