#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

enum pcf8574_pinout {
    lcd_rs_pin = 0,
    lcd_rw_pin = 1,
    lcd_e_pin  = 2,
    lcd_bl_pin = 3,
    lcd_d4_pin = 4,
    lcd_d5_pin = 5,
    lcd_d6_pin = 6,
    lcd_d7_pin = 7
};

typedef enum lcd_rw {
    lcd_write = 0,
    lcd_read = 1
} lcd_rw_t;

typedef enum lcd_rs {
    lcd_instruction = 0,
    lcd_data = 1
} lcd_rs_t;

int i2c_send(int file, char byte);
int lcd_backlight_off(int file);
int lcd_backlight_on(int file);
int lcd_backlight_apply(int file);
int lcd_init(int file);
int lcd_home(int file);
int lcd_print(int file, char *text);
int lcd_on(int file);
int lcd_off(int file);
int lcd_clear(int file);
int lcd_modeset(int file);
int lcd_funcset(int file);
int lcd_addr(int file, char addr);
int lcd_send(int file, lcd_rs_t rs, lcd_rw_t rw, char data);
int lcd_send4_int(int file, char data);
int i2c_lcd_send(int file, char byte);

int backlight = 1 << lcd_bl_pin;

int main()
{
    int addr = 0x27;
    char *filename = "/dev/i2c-0";
    int file;
    char str[20];

    if ((file = open(filename, O_RDWR)) < 0) {
        fprintf(stderr, "Can not open file `%s'\n", filename);
        exit(1);
    }
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        fputs("Failed to acquire bus access and/or talk to slave.\n", stderr);
        exit(1);
    }
    if (! lcd_init(file)) {
        fputs("Error initializing display\n", stderr);
        exit(1);
    }
    lcd_on(file);
    lcd_backlight_on(file);
    while(1) {
	lcd_addr(file, 0x00);
    	gets(str);
	lcd_clear(file);
    	lcd_print(file, str);
    	lcd_addr(file, 0x40);
    	gets(str);
    	lcd_print(file, str);
    }
    return 0;
}

int lcd_backlight_off(int file)
{
    backlight = 0;
    return lcd_backlight_apply(file);
}

int lcd_backlight_on(int file)
{
    backlight = 1 << lcd_bl_pin;
    return lcd_backlight_apply(file);
}

int lcd_backlight_apply(int file)
{
    return i2c_send(file, backlight | 1 << lcd_e_pin);
}

int lcd_init(int file)
{
    int i;
    usleep(15000);

    i = lcd_send4_int(file, (1 << lcd_d5_pin) | (1 << lcd_d4_pin));
    if (i < 1)
        return 0;
    usleep(4100);
    i = lcd_send4_int(file, (1 << lcd_d5_pin) | (1 << lcd_d4_pin));
    if (i < 1)
        return 0;
    usleep(100);
    i = lcd_send4_int(file, (1 << lcd_d5_pin) | (1 << lcd_d4_pin));
    if (i < 1)
        return 0;
    i = lcd_send4_int(file, 1 << lcd_d5_pin);
    if (i < 1)
        return 0;
    i = lcd_funcset(file);
    if (i < 1)
        return 0;
    i = lcd_off(file);
    if (i < 1)
        return 0;
    i = lcd_clear(file);
    if (i < 1)
        return 0;
    i = lcd_modeset(file);
    if (i < 1)
        return 0;
    return 1;
}

int lcd_print(int file, char *text)
{
    char *ptr = text;
    int i;
    while (*ptr != '\0') {
        i = lcd_send(file, lcd_data, lcd_write, *ptr);
        if (i < 1) {
            break;
        } else {
            ptr++;
        }
    }
    return ptr - text;
}

int lcd_funcset(int file)
{
     int i;
     i = lcd_send(file, lcd_instruction, lcd_write, 0x28);
     if (i > 0)
         usleep(37);
     return i;
}

int lcd_on(int file)
{
    int i;
    i = lcd_send(file, lcd_instruction, lcd_write, 0x0C);
    if (i > 0)
        usleep(37);
    return i;
}

int lcd_off(int file)
{
    int i;
    i = lcd_send(file, lcd_instruction, lcd_write, 0x08);
    if (i > 0)
        usleep(37);
    return i;
}

int lcd_clear(int file)
{
    int i;
    i = lcd_send(file, lcd_instruction, lcd_write, 0x01);
    return i;
}

int lcd_modeset(int file)
{
    int i;
    i = lcd_send(file, lcd_instruction, lcd_write, 0x06);
    if (i > 0)
        usleep(37);
    return i;
}

int lcd_home(int file)
{
    int i;
    i = lcd_send(file, lcd_instruction, lcd_write, 0x02);
    if (i > 0)
        usleep(1520);
    return i;
}

int lcd_addr(int file, char addr)
{
    int i;
    i = lcd_send(file, lcd_instruction, lcd_write, 1 << 7 | addr);
    if (i > 0)
        usleep(37);
    return i;
}

int lcd_send(int file, lcd_rs_t rs, lcd_rw_t rw, char data)
{
    char byte;
    rs = rs & 1;
    rw = rw & 1;
    rs = rs << lcd_rs_pin;
    rw = rw << lcd_rw_pin;

    byte =
        (((data >> 7) & 1) << lcd_d7_pin) |
        (((data >> 6) & 1) << lcd_d6_pin) |
        (((data >> 5) & 1) << lcd_d5_pin) |
        (((data >> 4) & 1) << lcd_d4_pin) |
        rw | rs;
    i2c_lcd_send(file, byte);

    byte =
        (((data >> 3) & 1) << lcd_d7_pin) |
        (((data >> 2) & 1) << lcd_d6_pin) |
        (((data >> 1) & 1) << lcd_d5_pin) |
        (((data >> 0) & 1) << lcd_d4_pin) |
        rw | rs;
    return i2c_lcd_send(file, byte);
}

int lcd_send4_int(int file, char data)
{
    char byte = data &
        ~(1 << lcd_rs_pin) &
        ~(1 << lcd_rw_pin);
    return i2c_lcd_send(file, byte);
}

int i2c_lcd_send(int file, char byte)
{
    int i;

    byte = byte | (1 << lcd_e_pin) | backlight;
    i = i2c_send(file, byte);
    if (i < 1)
        return 0;

    byte = byte & ~(1 << lcd_e_pin);
    i = i2c_send(file, byte);
    if (i < 1)
        return 0;

    byte = byte | (1 << lcd_e_pin);
    i = i2c_send(file, byte);
    return i;
}

int i2c_send(int file, char byte)
{
    const char *buf = &byte;
    return write(file, buf, 1);
}
