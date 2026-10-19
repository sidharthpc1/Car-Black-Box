/*
 * File:   eeprom.c
 * Author: sidha
 *
 * Created on 13 October, 2026, 10:16 AM
 */


#include <xc.h>
#include "i2c.h"
#include "eeprom.h"

#define _XTAL_FREQ 20000000

extern unsigned int event_count;

void write_external_eeprom(unsigned char address,unsigned char data)
{
    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);
    i2c_write(data);
    i2c_stop();

    __delay_ms(5);

    event_count++;
}

unsigned char read_external_eeprom(unsigned char address)
{
    unsigned char data;

    i2c_start();
    i2c_write(SLAVE_WRITE);
    i2c_write(address);

    i2c_rep_start();
    i2c_write(SLAVE_READ);
    data = i2c_read();
    i2c_stop();

    return data;
}
