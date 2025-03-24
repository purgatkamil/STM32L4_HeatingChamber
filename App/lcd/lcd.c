#include "lcd.h"
#include "font.h"

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#define ST7735S_SLPOUT			0x11
#define ST7735S_DISPOFF			0x28
#define ST7735S_DISPON			0x29
#define ST7735S_CASET			0x2a
#define ST7735S_RASET			0x2b
#define ST7735S_RAMWR			0x2c
#define ST7735S_MADCTL			0x36
#define ST7735S_COLMOD			0x3a
#define ST7735S_FRMCTR1			0xb1
#define ST7735S_FRMCTR2			0xb2
#define ST7735S_FRMCTR3			0xb3
#define ST7735S_INVCTR			0xb4
#define ST7735S_PWCTR1			0xc0
#define ST7735S_PWCTR2			0xc1
#define ST7735S_PWCTR3			0xc2
#define ST7735S_PWCTR4			0xc3
#define ST7735S_PWCTR5			0xc4
#define ST7735S_VMCTR1			0xc5
#define ST7735S_GAMCTRP1		0xe0
#define ST7735S_GAMCTRN1		0xe1

#define CMD(x)			((x) | 0x100)

#define FONT_WIDTH 5
#define FONT_HEIGHT 8

lcd_font_s fonts[5][3] =
{
	{{Font8_Table, 8, 5}},
	{{Font12_Table, 12, 7}},
	{{Font16_Table, 16, 11}},
	{{Font20_Table, 20, 14}},
	{{Font24_Table, 24, 17}}
};

static const uint16_t init_table[] =
{
  CMD(ST7735S_FRMCTR1), 0x01, 0x2c, 0x2d,
  CMD(ST7735S_FRMCTR2), 0x01, 0x2c, 0x2d,
  CMD(ST7735S_FRMCTR3), 0x01, 0x2c, 0x2d, 0x01, 0x2c, 0x2d,
  CMD(ST7735S_INVCTR), 0x07,
  CMD(ST7735S_PWCTR1), 0xa2, 0x02, 0x84,
  CMD(ST7735S_PWCTR2), 0xc5,
  CMD(ST7735S_PWCTR3), 0x0a, 0x00,
  CMD(ST7735S_PWCTR4), 0x8a, 0x2a,
  CMD(ST7735S_PWCTR5), 0x8a, 0xee,
  CMD(ST7735S_VMCTR1), 0x0e,
  CMD(ST7735S_GAMCTRP1), 0x0f, 0x1a, 0x0f, 0x18, 0x2f, 0x28, 0x20, 0x22,
                         0x1f, 0x1b, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,
  CMD(ST7735S_GAMCTRN1), 0x0f, 0x1b, 0x0f, 0x17, 0x33, 0x2c, 0x29, 0x2e,
                         0x30, 0x30, 0x39, 0x3f, 0x00, 0x07, 0x03, 0x10,
  CMD(0xf0), 0x01,
  CMD(0xf6), 0x00,
  CMD(ST7735S_COLMOD), 0x05,
  CMD(ST7735S_MADCTL), 0xa0,

  CMD(ST7735S_MADCTL), 0x60, //rotacja o 180 stopni  //0xa0,
};

typedef struct
{
    uint16_t frame_buffer[LCD_WIDTH * LCD_HEIGHT];
    osMutexId_t buffer_mutex;
    volatile bool dma_done;
    volatile bool dma_error;
} lcd_handler_t;

static lcd_handler_t lcd_handler;

static void handle_error(void);
static bool lcd_send_command(uint8_t cmd);
static bool lcd_send_data(uint8_t data);

static bool lcd_send_command(uint8_t cmd)
{
    bool result = false;
    taskENTER_CRITICAL();
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

    if (HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY) == HAL_OK)
    {
        result = true;
    }

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    taskEXIT_CRITICAL();

    return result;
}

static bool lcd_send_data(uint8_t data)
{
    bool result = false;
    taskENTER_CRITICAL();
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

    if (HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY) == HAL_OK)
    {
        result = true;
    }

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    taskEXIT_CRITICAL();

    return result;
}

static bool lcd_send(uint16_t value)
{
	bool result;

    for (int i = 0; i < 3; ++i)
    {
        if (value & 0x100)
        {
            result = lcd_send_command(value);
        }
        else
        {
            result = lcd_send_data(value);
        }

        if (result)
        	break;
    }

    return result;
}

static bool lcd_send_data16(uint16_t value)
{
    return lcd_send(value >> 8) && lcd_send(value);
}

static bool lcd_set_window(int x, int y, int width, int height)
{
    bool result = true;

    result &= lcd_send(CMD(ST7735S_CASET));
    result &= lcd_send_data16(LCD_OFFSET_X + x);
    result &= lcd_send_data16(LCD_OFFSET_X + x + width - 1);

    result &= lcd_send(CMD(ST7735S_RASET));
    result &= lcd_send_data16(LCD_OFFSET_Y + y);
    result &= lcd_send_data16(LCD_OFFSET_Y + y + height - 1);

    return result;
}

bool lcd_init(void)
{
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    osDelay(100);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    osDelay(100);

    for (int i = 0; i < sizeof(init_table) / sizeof(init_table[0]); i++)
    {
        lcd_send(init_table[i]);
    }

    osDelay(200);
    lcd_send(CMD(ST7735S_SLPOUT));
    osDelay(110);
    lcd_send(CMD(ST7735S_DISPON));

    lcd_handler.buffer_mutex = osMutexNew(NULL);

    return (lcd_handler.buffer_mutex != NULL);
}

void lcd_put_pixel(int x, int y, uint16_t color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    if (osMutexAcquire(lcd_handler.buffer_mutex, osWaitForever) == osOK)
    {
        lcd_handler.frame_buffer[x + y * LCD_WIDTH] = color;
        osMutexRelease(lcd_handler.buffer_mutex);
    }
    else
    {
        handle_error();
    }
}

void lcd_fill(uint16_t color)
{
    if (osMutexAcquire(lcd_handler.buffer_mutex, osWaitForever) == osOK)
    {
        for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++)
        {
            lcd_handler.frame_buffer[i] = color;
        }

        osMutexRelease(lcd_handler.buffer_mutex);
    }
    else
    {
        handle_error();
    }
}

bool lcd_copy(void)
{
    bool result = false;

    if (osMutexAcquire(lcd_handler.buffer_mutex, osWaitForever) == osOK)
    {
        lcd_handler.dma_done = false;
        lcd_handler.dma_error = false;

        lcd_set_window(0, 0, LCD_WIDTH, LCD_HEIGHT);
        lcd_send(CMD(ST7735S_RAMWR));

        taskENTER_CRITICAL();
        HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
        HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)lcd_handler.frame_buffer, sizeof(lcd_handler.frame_buffer));
        taskEXIT_CRITICAL();

        if (status == HAL_OK)
        {
            result = true;
        }

        osMutexRelease(lcd_handler.buffer_mutex);
    }
    else
    {
        handle_error();
    }

    return result;
}



void lcd_display_char(uint16_t x, uint16_t y, char ascii_char, uint16_t color, lcd_font_e font_type)
{
    uint8_t font_width = fonts[font_type]->width;
    uint8_t font_height = fonts[font_type]->height;

    uint32_t char_offset = (ascii_char - ' ') * font_height * (font_width / 8 + (font_width % 8 ? 1 : 0));
    unsigned char* ptr = fonts[font_type]->font_add + char_offset;

    if (osMutexAcquire(lcd_handler.buffer_mutex, osWaitForever) == osOK)
    {
        for (uint16_t page = 0; page < font_height; page++)
        {
            for (uint16_t column = 0; column < font_width; column++)
            {
                if (*ptr & (0x80 >> (column % 8)))
                {
                    lcd_handler.frame_buffer[(x + column) + (y + page) * LCD_WIDTH] = color;
                }

                if (column % 8 == 7)
                {
                    ptr++;
                }
            }

            if (font_width % 8 != 0)
            {
                ptr++;
            }
        }

        osMutexRelease(lcd_handler.buffer_mutex);
    }
    else
    {
        handle_error();
    }
}

void lcd_display_string(uint16_t x_start, uint16_t y_start, char* str, uint16_t color, lcd_font_e font_type)
{
    uint8_t font_width = fonts[font_type]->width;
    uint8_t font_height = fonts[font_type]->height;

    while (*str != '\0')
    {
        if (x_start + font_width > LCD_WIDTH)
        {
            x_start = 0;
            y_start += font_height;
        }

        if (y_start + font_height > LCD_HEIGHT)
        {
            break;
        }

        lcd_display_char(x_start, y_start, *str, color, font_type);
        str++;
        x_start += font_width;
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        lcd_handler.dma_done = true;
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        lcd_handler.dma_error = true;
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    }
}

static void handle_error(void)
{

}

