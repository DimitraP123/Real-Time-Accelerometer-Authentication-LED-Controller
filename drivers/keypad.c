/* 
 | Programmer: Dimitra Pando
 |
 | The purpose of this file is to implement a debounced, FSM-based driver for a 4×3 
 | matrix keypad. It has functions that include the logic to configure GPIO pins, 
 | scan rows and columns to detect key presses, and store the corresponding correct
 | character for the main function to receive.
 */

#include "keypad.h"
#include <stdint.h>
#include <rp2350/resets.h>
#include <rp2350/io_bank0.h>
#include <rp2350/pads_bank0.h>
#include <rp2350/sio.h>
#include <rp2350/pio.h>

#define GPIO_RESETS (RESETS_RESET_IO_BANK0_MASK | RESETS_RESET_PADS_BANK0_MASK)

#define NOKEY 0xFF
#define ALL  15

#define PO0 2
#define PO1 3
#define PO2 4
#define PO3 5
#define PI0 6
#define PI1 7
#define PI2 8

typedef enum keypad_FSM_state_t
{
    WAIT_PRESS = 0,
    SCAN0,
    SCAN1,
    SCAN2,
    SCAN3,
    WAIT_RELEASE
} keypad_state_t;

static const char keypad_layout[4][3] = 
{
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

static keypad_state_t keypad_state = WAIT_PRESS;
static volatile char keypad_char;

static void keypad_scan_row(int8_t row)
{
    SIO_GPIO_OUT_CLR = (1<<PO0)|(1<<PO1)|(1<<PO2)|(1<<PO3);
    SIO_GPIO_OE_CLR = (1<<PO0)|(1<<PO1)|(1<<PO2)|(1<<PO3);
    switch(row)
    {
        case -1: 
            break;
        case 0: 
            SIO_GPIO_OE_SET = (1<<PO0);
            break;
        case 1: 
            SIO_GPIO_OE_SET = (1<<PO1);
            break;
        case 2: 
            SIO_GPIO_OE_SET = (1<<PO2);
            break;
        case 3: 
            SIO_GPIO_OE_SET = (1<<PO3);
            break;
        default:
            SIO_GPIO_OE_SET = (1<<PO0)|(1<<PO1)|(1<<PO2)|(1<<PO3);
            break;   
    }
}

void configure_keypad(void)
{
    resets.reset_clr = GPIO_RESETS;
    while (!(resets.reset_done & GPIO_RESETS))
    continue;

    pads_bank0.gpio2 =
    PADS_BANK0_GPIO2_OD(0) |
    PADS_BANK0_GPIO2_IE(0) |
    PADS_BANK0_GPIO2_DRIVE(0) |
    PADS_BANK0_GPIO2_PUE(0) |
    PADS_BANK0_GPIO2_PDE(0) |
    PADS_BANK0_GPIO2_SCHMITT(0) |
    PADS_BANK0_GPIO2_SLEWFAST(0);
    io_bank0.gpio2_ctrl =
    IO_BANK0_GPIO2_CTRL_IRQOVER(0) |
    IO_BANK0_GPIO2_CTRL_INOVER(0) |
    IO_BANK0_GPIO2_CTRL_OEOVER(0) |
    IO_BANK0_GPIO2_CTRL_OUTOVER(0) |
    IO_BANK0_GPIO2_CTRL_FUNCSEL(5);

    pads_bank0.gpio3 =
    PADS_BANK0_GPIO3_OD(0) |
    PADS_BANK0_GPIO3_IE(0) |
    PADS_BANK0_GPIO3_DRIVE(0) |
    PADS_BANK0_GPIO3_PUE(0) |
    PADS_BANK0_GPIO3_PDE(0) |
    PADS_BANK0_GPIO3_SCHMITT(0) |
    PADS_BANK0_GPIO3_SLEWFAST(0);
    io_bank0.gpio3_ctrl =
    IO_BANK0_GPIO3_CTRL_IRQOVER(0) |
    IO_BANK0_GPIO3_CTRL_INOVER(0) |
    IO_BANK0_GPIO3_CTRL_OEOVER(0) |
    IO_BANK0_GPIO3_CTRL_OUTOVER(0) |
    IO_BANK0_GPIO3_CTRL_FUNCSEL(5);

    pads_bank0.gpio4 =
    PADS_BANK0_GPIO4_OD(0) |
    PADS_BANK0_GPIO4_IE(0) |
    PADS_BANK0_GPIO4_DRIVE(0) |
    PADS_BANK0_GPIO4_PUE(0) |
    PADS_BANK0_GPIO4_PDE(0) |
    PADS_BANK0_GPIO4_SCHMITT(0) |
    PADS_BANK0_GPIO4_SLEWFAST(0);
    io_bank0.gpio4_ctrl =
    IO_BANK0_GPIO4_CTRL_IRQOVER(0) |
    IO_BANK0_GPIO4_CTRL_INOVER(0) |
    IO_BANK0_GPIO4_CTRL_OEOVER(0) |
    IO_BANK0_GPIO4_CTRL_OUTOVER(0) |
    IO_BANK0_GPIO4_CTRL_FUNCSEL(5);

    pads_bank0.gpio5 =
    PADS_BANK0_GPIO5_OD(0) |
    PADS_BANK0_GPIO5_IE(0) |
    PADS_BANK0_GPIO5_DRIVE(0) |
    PADS_BANK0_GPIO5_PUE(0) |
    PADS_BANK0_GPIO5_PDE(0) |
    PADS_BANK0_GPIO5_SCHMITT(0) |
    PADS_BANK0_GPIO5_SLEWFAST(0);
    io_bank0.gpio5_ctrl =
    IO_BANK0_GPIO5_CTRL_IRQOVER(0) |
    IO_BANK0_GPIO5_CTRL_INOVER(0) |
    IO_BANK0_GPIO5_CTRL_OEOVER(0) |
    IO_BANK0_GPIO5_CTRL_OUTOVER(0) |
    IO_BANK0_GPIO5_CTRL_FUNCSEL(5);

    pads_bank0.gpio6 =
    PADS_BANK0_GPIO6_OD(0) |
    PADS_BANK0_GPIO6_IE(1) |
    PADS_BANK0_GPIO6_DRIVE(0) |
    PADS_BANK0_GPIO6_PUE(1) |
    PADS_BANK0_GPIO6_PDE(0) |
    PADS_BANK0_GPIO6_SCHMITT(0) |
    PADS_BANK0_GPIO6_SLEWFAST(0);
    io_bank0.gpio6_ctrl =
    IO_BANK0_GPIO6_CTRL_IRQOVER(0) |
    IO_BANK0_GPIO6_CTRL_INOVER(0) |
    IO_BANK0_GPIO6_CTRL_OEOVER(0) |
    IO_BANK0_GPIO6_CTRL_OUTOVER(0) |
    IO_BANK0_GPIO6_CTRL_FUNCSEL(5);

    pads_bank0.gpio7 =
    PADS_BANK0_GPIO7_OD(0) |
    PADS_BANK0_GPIO7_IE(1) |
    PADS_BANK0_GPIO7_DRIVE(0) |
    PADS_BANK0_GPIO7_PUE(1) |
    PADS_BANK0_GPIO7_PDE(0) |
    PADS_BANK0_GPIO7_SCHMITT(0) |
    PADS_BANK0_GPIO7_SLEWFAST(0);
    io_bank0.gpio7_ctrl =
    IO_BANK0_GPIO7_CTRL_IRQOVER(0) |
    IO_BANK0_GPIO7_CTRL_INOVER(0) |
    IO_BANK0_GPIO7_CTRL_OEOVER(0) |
    IO_BANK0_GPIO7_CTRL_OUTOVER(0) |
    IO_BANK0_GPIO7_CTRL_FUNCSEL(5);

    pads_bank0.gpio8 =
    PADS_BANK0_GPIO8_OD(0) |
    PADS_BANK0_GPIO8_IE(1) |
    PADS_BANK0_GPIO8_DRIVE(0) |
    PADS_BANK0_GPIO8_PUE(1) |
    PADS_BANK0_GPIO8_PDE(0) |
    PADS_BANK0_GPIO8_SCHMITT(0) |
    PADS_BANK0_GPIO8_SLEWFAST(0);
    io_bank0.gpio8_ctrl =
    IO_BANK0_GPIO8_CTRL_IRQOVER(0) |
    IO_BANK0_GPIO8_CTRL_INOVER(0) |
    IO_BANK0_GPIO8_CTRL_OEOVER(0) |
    IO_BANK0_GPIO8_CTRL_OUTOVER(0) |
    IO_BANK0_GPIO8_CTRL_FUNCSEL(5);

    keypad_char = 0;
}

static uint8_t read_col()
{
    uint8_t retval = NOKEY;  
    if (0 == (SIO_GPIO_IN & (1<<PI0)))
        retval = 0;
    else if (0 == (SIO_GPIO_IN & (1<<PI1))) 
        retval = 1;
    else if (0 == (SIO_GPIO_IN & (1<<PI2)))
        retval = 2;
    return retval;
}

void task_keypad_scan(void)
{
    uint8_t col_value = NOKEY;

    switch(keypad_state)
    {
        case WAIT_PRESS:
            keypad_scan_row(ALL);
            if (read_col() != NOKEY)
            {
                keypad_state = SCAN0;
                keypad_scan_row(0);
            }
            break;

        case SCAN0:
            col_value = read_col();

            if (read_col() == NOKEY)
            {
                keypad_state = SCAN1;
                keypad_scan_row(1);
            }
            else
            {
                keypad_state = WAIT_RELEASE;
                keypad_scan_row(ALL);
                keypad_char = keypad_layout[0][col_value];
            }
            break;

        case SCAN1:
            col_value = read_col();

            if (col_value == NOKEY)
            {
                keypad_state = SCAN2;
                keypad_scan_row(2);
            }
            else
            {
                keypad_state = WAIT_RELEASE;
                keypad_scan_row(ALL);
                keypad_char = keypad_layout[1][col_value];
            }
            break;

        case SCAN2:
            col_value = read_col();

            if (col_value == NOKEY)
            {
                keypad_state = SCAN3;
                keypad_scan_row(3);
            }
            else
            {
                keypad_state = WAIT_RELEASE;
                keypad_scan_row(ALL);
                keypad_char = keypad_layout[2][col_value];
            }
            break;

        case SCAN3:
            col_value = read_col();

            if (col_value == NOKEY)
            {
                keypad_state = WAIT_RELEASE;
                keypad_scan_row(ALL);
            }
            else
            {
                keypad_state = WAIT_RELEASE;
                keypad_scan_row(ALL);
                keypad_char = keypad_layout[3][col_value];
            }
            break;

        case WAIT_RELEASE:
            if (read_col() == NOKEY)
            {
               keypad_state = WAIT_PRESS;
            }
            break;
    }
}

int keypad_getchar(char *cp)
{
    if (keypad_char == 0)
        return 0;
    *cp = keypad_char;
    keypad_char = 0;
    return 1;
}
