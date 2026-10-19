/*
 * File:   main.c
 * Author: sidha
 *
 * Created on 13 October, 2026, 10:14 AM
 */


#include <xc.h>
#include "clcd.h"
#include "adc.h"
#include "mkp.h"
#include "i2c.h"
#include "eeprom.h"
#include "uart.h"

#define _XTAL_FREQ 20000000

/* RTC register addresses */
#define SEC_ADDR       0x00
#define MIN_ADDR       0x01
#define HOUR_ADDR      0x02

/* EEPROM log configuration */
#define RECORD_SIZE    12
#define MAX_EVENTS     10
#define LOG_WIDTH      16
#define MENU_ITEMS     4


void set_time(void);

/* Gear list */
const unsigned char gear[8][3] =
{
    "ON",
    "GN",
    "G1",
    "G2",
    "G3",
    "G4",
    "G5",
    "GR"
};

/* Part 1 variables */
unsigned int event_count = 0;
unsigned char count = 0;
unsigned char collision = 0;

unsigned char event_total = 0;
unsigned char next_slot = 0;

unsigned char time_str[9];
unsigned char event_str[3];
unsigned char speed_str[3];

unsigned short adc_value;
unsigned char speed;

/* Part 2 log buffer */
unsigned char event[10][16];

/* Main menu */
const unsigned char menu_items[4][13] =
{
    "View log",
    "Download log",
    "Clear log",
    "Set time"
};

void init_config(void)
{
    init_clcd();
    init_matrix_keypad();
    init_adc();
    init_i2c();
    init_uart();

    /* Configure AN4 for speed input */
    ADCON1 = 0x07;
    TRISA5 = 1;
}

unsigned char read_rtc(unsigned char address)
{
    unsigned char data;

    i2c_start();
    i2c_write(0xD0);
    i2c_write(address);

    i2c_rep_start();
    i2c_write(0xD1);

    data = i2c_read();

    i2c_stop();

    return data;
}

/* Convert a BCD value into two ASCII digits */
void bcd_to_ascii(unsigned char value,
                  unsigned char *tens,
                  unsigned char *units)
{
    *tens  = ((value >> 4) & 0x0F) + '0';
    *units = (value & 0x0F) + '0';
}

void get_time(void)
{
    unsigned char hour;
    unsigned char minute;
    unsigned char second;

    hour   = read_rtc(HOUR_ADDR);
    minute = read_rtc(MIN_ADDR);
    second = read_rtc(SEC_ADDR);

    /* Mask RTC control bits */
    hour &= 0x3F;
    minute &= 0x7F;
    second &= 0x7F;

    bcd_to_ascii(hour, &time_str[0], &time_str[1]);
    time_str[2] = ':';

    bcd_to_ascii(minute, &time_str[3], &time_str[4]);
    time_str[5] = ':';

    bcd_to_ascii(second, &time_str[6], &time_str[7]);
    time_str[8] = '\0';
}

void get_speed(void)
{
    adc_value = read_adc(CHANNEL4);

    /* Mentor's calculation */
    speed = (unsigned char)(adc_value / 10.33);

    if (speed > 99)
    {
        speed = 99;
    }

    speed_str[0] = (speed / 10) + '0';
    speed_str[1] = (speed % 10) + '0';
    speed_str[2] = '\0';
}

void update_event_display(void)
{
    if (collision)
    {
        event_str[0] = 'C';
        event_str[1] = '_';
    }
    else
    {
        event_str[0] = gear[count][0];
        event_str[1] = gear[count][1];
    }

    event_str[2] = '\0';
}

void store_event(void)
{
    unsigned char address;
    unsigned char i;

    /* Select next EEPROM slot */
    address = next_slot * RECORD_SIZE;

    /* Store time */
    for (i = 0; i < 8; i++)
    {
        write_external_eeprom(address++, time_str[i]);
    }

    /* Store gear or collision event */
    for (i = 0; i < 2; i++)
    {
        write_external_eeprom(address++, event_str[i]);
    }

    /* Store speed */
    for (i = 0; i < 2; i++)
    {
        write_external_eeprom(address++, speed_str[i]);
    }

    /* Count complete records */
    if (event_total < MAX_EVENTS)
    {
        event_total++;
    }

    /* Advance to next slot */
    next_slot++;

    /* Overwrite from slot 0 after slot 9 */
    if (next_slot >= MAX_EVENTS)
    {
        next_slot = 0;
    }
}

void read_event(void)
{
    unsigned char i;
    unsigned char j;
    unsigned char slot;
    unsigned char address;

    for (i = 0; i < event_total; i++)
    {
        
        if (event_total < MAX_EVENTS)
        {
            slot = i;
        }
        else
        {
            slot = (next_slot + i) % MAX_EVENTS;
        }

        address = slot * RECORD_SIZE;

        /* Record number */
        event[i][0] = (i + 1) % 10 + '0';
        event[i][1] = ' ';

        /* Time: 8 characters */
        for (j = 0; j < 8; j++)
        {
            event[i][2 + j] =
                read_external_eeprom(address++);
        }

        event[i][10] = ' ';

        /* Event: 2 characters */
        event[i][11] =
            read_external_eeprom(address++);

        event[i][12] =
            read_external_eeprom(address++);

        event[i][13] = ' ';

        /* Speed: 2 characters */
        event[i][14] =
            read_external_eeprom(address++);

        event[i][15] =
            read_external_eeprom(address++);
    }
}

void print_log_row(unsigned char row, unsigned char line)
{
    unsigned char col;
    unsigned char address;

    address = (line == 0) ? LINE1(0) : LINE2(0);

    for (col = 0; col < LOG_WIDTH; col++)
    {
        clcd_putch(event[row][col], address + col);
    }
}

void view_log(void)
{
    unsigned char position = 0;
    unsigned char key;

    if (event_total == 0)
    {
        CLEAR_DISP_SCREEN;
        clcd_print("NO LOGS FOUND", LINE1(0));

        __delay_ms(1500);
        return;
    }

    read_event();

    while (1)
    {
        
        CLEAR_DISP_SCREEN;

        print_log_row(position, 0);

        if (position + 1 < event_total)
        {
            print_log_row(position + 1, 1);
        }
        else
        {
            clcd_print("                ", LINE2(0));
        }


        key = read_switches(STATE_CHANGE);

        if (key == MK_SW1)
        {
            if (position + 2 < event_total)
            {
                position += 2;
            }
            else
            {
                position = 0;
            }
        }
        else if (key == MK_SW2)
        {
            if (position >= 2)
            {
                position -= 2;
            }
            else if (event_total > 1)
            {
                position = ((event_total - 1) / 2) * 2;
            }
        }
        else if (key == MK_SW12)
        {
            return;
        }

        __delay_ms(150);
    }
}

void download_log(void)
{
    unsigned char i;
    unsigned char j;

    CLEAR_DISP_SCREEN;
    clcd_print("Downloading...", LINE1(0));

    if (event_total == 0)
    {
        clcd_print("No logs found", LINE2(0));
        __delay_ms(1500);
        return;
    }

    read_event();

    /* Print records in chronological order */
    for (i = 0; i < event_total; i++)
    {
        putch(event[i][0]);
        putch(' ');

        for (j = 2; j < 10; j++)
        {
            putch(event[i][j]);
        }

        putch(' ');

        putch(event[i][11]);
        putch(event[i][12]);

        putch(' ');

        putch(event[i][14]);
        putch(event[i][15]);

        putch('\r');
        putch('\n');
    }

    __delay_ms(1500);
}

void clear_log(void)
{
    event_total = 0;
    next_slot = 0;
    event_count = 0;

    CLEAR_DISP_SCREEN;
    clcd_print("Logs Cleared", LINE1(0));

    __delay_ms(1500);
}

void main_menu(void)
{
    unsigned char menu = 0;
    unsigned char key;

    while (1)
    {
        CLEAR_DISP_SCREEN;

        /* Selected option on first line */
        clcd_putch('>', LINE1(0));
        clcd_print(menu_items[menu], LINE1(1));

        /* Next option on second line */
        clcd_print(menu_items[(menu + 1) % MENU_ITEMS],
                   LINE2(1));

        key = read_switches(STATE_CHANGE);

        if (key == MK_SW1)
        {
            menu = (menu + 1) % MENU_ITEMS;
        }
        else if (key == MK_SW2)
        {
            if (menu == 0)
            {
                menu = MENU_ITEMS - 1;
            }
            else
            {
                menu--;
            }
        }
        else if (key == MK_SW11)
        {
            switch (menu)
            {
                case 0:
                    view_log();
                    break;

                case 1:
                    download_log();
                    break;

                case 2:
                    clear_log();
                    break;

                case 3:
                    set_time();
                    break;

            }
        }
        else if (key == MK_SW12)
        {
            return;
        }

        __delay_ms(150);
    }
}


/* Write one register of DS1307 */
void write_ds1307(unsigned char address, unsigned char data)
{
    i2c_start();
    i2c_write(0xD0);
    i2c_write(address);
    i2c_write(data);
    i2c_stop();
}

/* Convert BCD to decimal */
unsigned char bcd_to_decimal(unsigned char value)
{
    return ((value >> 4) * 10) + (value & 0x0F);
}

/* Convert decimal to BCD */
unsigned char decimal_to_bcd(unsigned char value)
{
    return ((value / 10) << 4) | (value % 10);
}

/* Set Time operation */
void set_time(void)
{
    unsigned char hr, min, sec;
    unsigned char key;
    unsigned char selected = 0;
    unsigned char blink = 0;

    /* Read existing RTC values */
    hr  = bcd_to_decimal(read_rtc(HOUR_ADDR) & 0x3F);
    min = bcd_to_decimal(read_rtc(MIN_ADDR) & 0x7F);
    sec = bcd_to_decimal(read_rtc(SEC_ADDR) & 0x7F);

    /* Protect against invalid RTC values */
    if (hr > 23)
        hr = 0;

    if (min > 59)
        min = 0;

    if (sec > 59)
        sec = 0;

    while (1)
    {
        /* Display HH:MM:SS */
        time_str[0] = (hr / 10) + '0';
        time_str[1] = (hr % 10) + '0';
        time_str[2] = ':';

        time_str[3] = (min / 10) + '0';
        time_str[4] = (min % 10) + '0';
        time_str[5] = ':';

        time_str[6] = (sec / 10) + '0';
        time_str[7] = (sec % 10) + '0';
        time_str[8] = '\0';

        clcd_print("HH:MM:SS", LINE1(0));
        clcd_print(time_str, LINE2(0));

        if (selected == 0)
            clcd_write(LINE2(0), INSTRUCTION_COMMAND);
        else
            clcd_write(LINE2(3), INSTRUCTION_COMMAND);

        if (blink == 0)
            clcd_write(0x0F, INSTRUCTION_COMMAND);
        else
            clcd_write(0x0C, INSTRUCTION_COMMAND);

        blink = !blink;
        __delay_ms(250);

        key = read_switches(STATE_CHANGE);

        /* SW1: increment selected field */
        if (key == MK_SW1)
        {
            if (selected == 0)
            {
                hr++;

                if (hr > 23)
                    hr = 0;
            }
            else
            {
                min++;

                if (min > 59)
                    min = 0;
            }
        }

        /* SW2: select minutes */
        else if (key == MK_SW2)
        {
            selected = 1;
        }

        /* SW11: save time to RTC and exit */
        else if (key == MK_SW11)
        {
            /* Write seconds first, then minutes and hours */
            write_ds1307(SEC_ADDR, decimal_to_bcd(sec));
            write_ds1307(MIN_ADDR, decimal_to_bcd(min));
            write_ds1307(HOUR_ADDR, decimal_to_bcd(hr));

            clcd_write(0x0C, INSTRUCTION_COMMAND);
            CLEAR_DISP_SCREEN;
            return;
        }

        /* SW12: exit without saving */
        else if (key == MK_SW12)
        {
            clcd_write(0x0C, INSTRUCTION_COMMAND);
            CLEAR_DISP_SCREEN;
            return;
        }
    }
}

void main(void)
{
    unsigned char key;

    init_config();

    while (1)
    {
        get_time();
        get_speed();
        update_event_display();

        /* Dashboard labels */
        clcd_print("TIME", LINE1(0));
        clcd_print("SP", LINE1(9));
        clcd_print("EVN", LINE1(12));

        /* Dashboard values */
        clcd_print(time_str, LINE2(0));
        clcd_print(speed_str, LINE2(9));
        clcd_print(event_str, LINE2(12));

        key = read_switches(STATE_CHANGE);

        /* SW1: increase gear */
        if (key == MK_SW1)
        {
            if (collision)
            {
                collision = 0;
                count = 1;       /* Return to GN */
            }
            else if (count < 7)
            {
                count++;
            }

            get_time();
            get_speed();
            update_event_display();
            store_event();
        }

        /* SW2: decrease gear */
        else if (key == MK_SW2)
        {
            if (collision)
            {
                collision = 0;
                count = 1;       /* Return to GN */
            }
            else if (count > 0)
            {
                count--;
            }

            get_time();
            get_speed();
            update_event_display();
            store_event();
        }

        /* SW3: collision */
        else if (key == MK_SW3)
        {
            collision = 1;

            get_time();
            get_speed();
            update_event_display();
            store_event();
        }

        /* SW11: enter main menu */
        else if (key == MK_SW11)
        {
            main_menu();
        }
    }
}