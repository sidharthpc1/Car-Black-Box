/* 
 * File:   eeprom.h
 * Author: sidha
 *
 * Created on 13 October, 2026, 10:17 AM
 */

#ifndef EXTERNAL_EEPROM_H
#define EXTERNAL_EEPROM_H

#define SLAVE_WRITE  0xA0
#define SLAVE_READ   0xA1

unsigned char read_external_eeprom(unsigned char address);
void write_external_eeprom(unsigned char address, unsigned char data);

#endif
