#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "spi.h"

#define LCD_WIDTH    160
#define LCD_HEIGHT   128

#define LCD_OFFSET_X  1
#define LCD_OFFSET_Y  2

#define BLACK     0x0000
#define RED       0x00f8
#define GREEN     0xe007
#define BLUE      0x1f00
#define YELLOW    0xe0ff
#define MAGENTA   0x1ff8
#define CYAN      0xff07
#define WHITE     0xffff

typedef struct {
    unsigned char* font_add;
    int height;
    int width;
} lcd_font_s;

typedef enum {
    LCD_FONT8,
    LCD_FONT12,
    LCD_FONT16,
    LCD_FONT20,
    LCD_FONT24,
} lcd_font_e;

bool lcd_init(void);

bool lcd_copy(void);
void lcd_fill(uint16_t color);
void lcd_put_pixel(int x, int y, uint16_t color);

void lcd_display_char(uint16_t x, uint16_t y, char ascii_char, uint16_t color, lcd_font_e font_type);
void lcd_display_string(uint16_t x_start, uint16_t y_start, char* str, uint16_t color, lcd_font_e font_type);


