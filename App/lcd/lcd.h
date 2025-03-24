#ifndef INC_LCD_H_
#define INC_LCD_H_

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

// Inicjalizacja z FreeRTOS (mutex + task)
bool lcd_init_freertos(void);


void lcd_copy(void);
// Rysowanie i bufor
void lcd_fill(uint16_t color);
void lcd_draw_pixel(int x, int y, uint16_t color);
void lcd_fill_box(int x1, int y1, int x2, int y2, uint16_t color);
void lcd_draw_horizontal_line(int y, int x_start, int x_stop, uint16_t color);
void lcd_draw_vertical_line(int x, int y_start, int y_stop, uint16_t color);
void lcd_draw_horizontal_line_dotted(int y, int x_start, int x_stop, uint16_t color);
void lcd_draw_vertical_line_dotted(int x, int y_start, int y_stop, uint16_t color);
void LCD_DrawLine(int Xstart, int Ystart, int Xend, int Yend, uint16_t color);
void LCD_DrawCircle(int X_Center, int Y_Center, int Radius, uint16_t color);

// Tekst
void LCD_DisplayChar(uint16_t Xpoint, uint16_t Ypoint, char Ascii_Char, uint16_t Color, lcd_font_e font_type);
void LCD_DisplayString(uint16_t Xstart, uint16_t Ystart, char* pString, uint16_t Color, lcd_font_e font_type);

// Stan i synchronizacja
void lcd_mark_dirty(void);
bool lcd_is_busy(void);
void lcd_transfer_done(void);

bool lcd_get_dirty(void);
void lcd_clear_dirty(void);
osMutexId_t lcd_get_mutex(void); // jeśli potrzebujesz mutex poza modułem


// Callback dla DMA zakończenia SPI
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);

#endif /* INC_LCD_H_ */
