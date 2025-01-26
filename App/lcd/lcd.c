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

static uint16_t frame_buffer[LCD_WIDTH * LCD_HEIGHT];

static void lcd_send_command(uint8_t cmd)
{
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

static void lcd_send_data(uint8_t data)
{
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

#define CMD(x)			((x) | 0x100)

static void lcd_send(uint16_t value)
{
	if (value & 0x100)
	{
		lcd_send_command(value);
	}
	else
	{
		lcd_send_data(value);
	}
}

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

void lcd_init(void)
{
  int i;

  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  osDelay(100);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  osDelay(100);

  for (i = 0; i < sizeof(init_table) / sizeof(init_table[0]); i++)
  {
    lcd_send(init_table[i]);
  }

  osDelay(200);

  lcd_send_command(ST7735S_SLPOUT);
  osDelay(110);

  lcd_send_command(ST7735S_DISPON);
}

static void lcd_send_data16(uint16_t value)
{
	lcd_send_data(value >> 8);
	lcd_send_data(value);
}

static void lcd_set_window(int x, int y, int width, int height)
{
	lcd_send_command(ST7735S_CASET);
	lcd_send_data16(LCD_OFFSET_X + x);
	lcd_send_data16(LCD_OFFSET_X + x + width - 1);

	lcd_send_command(ST7735S_RASET);
	lcd_send_data16(LCD_OFFSET_Y + y);
	lcd_send_data16(LCD_OFFSET_Y + y + height- 1);
}

void lcd_put_pixel(int x, int y, uint16_t color)
{
	frame_buffer[x + y * LCD_WIDTH] = color;
}

void fill_with(uint16_t color)
{

	 int i = LCD_WIDTH * LCD_HEIGHT - 1;
	 while(i >= 0)
	 {
		frame_buffer[i] = color;
		--i;
	 }
}

void lcd_copy(void)
{
	lcd_set_window(0, 0, LCD_WIDTH, LCD_HEIGHT);
	lcd_send_command(ST7735S_RAMWR);
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)frame_buffer, sizeof(frame_buffer));
}

void lcd_transfer_done(void)
{
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

bool lcd_is_busy(void)
{
	if (HAL_SPI_GetState(&hspi1) == HAL_SPI_STATE_BUSY)
		return true;
	else
		return false;
}

void LCD_DisplayChar(uint16_t Xpoint, uint16_t Ypoint, char Acsii_Char, uint16_t Color, lcd_font_e font_type)
{
    uint8_t Font_Width = fonts[font_type]->width; // Szerokość czcionki
    uint8_t Font_Height = fonts[font_type]->height; // Wysokość czcionki

    uint32_t Char_Offset = (Acsii_Char - ' ') * Font_Height * (Font_Width / 8 + (Font_Width % 8 ? 1 : 0));
    unsigned char* ptr = (fonts[font_type]->font_add) + Char_Offset;//&Font8_Table[Char_Offset];

    for (uint16_t Page = 0; Page < Font_Height; Page++)
    {
        for (uint16_t Column = 0; Column < Font_Width; Column++)
        {
            if (*ptr & (0x80 >> (Column % 8)))
            {
                lcd_put_pixel(Xpoint + Column, Ypoint + Page, Color);
            }

            if (Column % 8 == 7)
            {
                ptr++;
            }
        }

        if (Font_Width % 8 != 0)
        {
            ptr++;
        }
    }
}

void LCD_DisplayString(uint16_t Xstart, uint16_t Ystart, char* pString, uint16_t Color, lcd_font_e font_type)
{
    uint8_t Font_Width = fonts[font_type]->width; // Szerokość czcionki
    uint8_t Font_Height = fonts[font_type]->height; // Wysokość czcionki

    while (*pString != '\0')
    {
        if (Xstart + Font_Width > LCD_WIDTH)
        {
            Xstart = 0;
            Ystart += Font_Height;
        }

        if (Ystart + Font_Height > LCD_HEIGHT)
        {
            break; // Wyjście z pętli, jeśli przekroczy wysokość ekranu
        }

        LCD_DisplayChar(Xstart, Ystart, *pString, Color, font_type);
        pString++;
        Xstart += Font_Width;
    }
}

void LCD_DrawLine ( int Xstart, int Ystart,
					int Xend, int Yend,
					uint16_t color)
{


	int Xpoint = Xstart;
	int Ypoint = Ystart;
	int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
	int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

	// Increment direction, 1 is positive, -1 is counter;
	int XAddway = Xstart < Xend ? 1 : -1;
	int YAddway = Ystart < Yend ? 1 : -1;

	//Cumulative error
	int Esp = dx + dy;

	for (;;)
	{
		//Painted dotted line, 2 point is really virtual

		lcd_put_pixel(Xpoint, Ypoint, color);

        if (2 * Esp >= dy)
        {
			if (Xpoint == Xend) break;
            Esp += dy;
			Xpoint += XAddway;
        }
        if (2 * Esp <= dx)
        {
			if (Ypoint == Yend) break;
            Esp += dx;
			Ypoint += YAddway;
        }
	}
}

void lcd_fill_box(int x1, int y1, int x2, int y2, uint16_t color)
{
    // Ensure coordinates are within the screen bounds
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= LCD_WIDTH) x2 = LCD_WIDTH - 1;
    if (y2 >= LCD_HEIGHT) y2 = LCD_HEIGHT - 1;

    // Iterate over each pixel in the specified rectangle
    for (int x = x1; x <= x2; x++)
    {
        for (int y = y1; y <= y2; y++)
        {
            lcd_put_pixel(x, y, color);
        }
    }
}

void lcd_draw_horizontal_line(int y, int x_start, int x_stop, uint16_t color)
{
	for(int i = x_start; i <= x_stop; i++)
	{
		lcd_put_pixel(i, y, color);
	}
}

void lcd_draw_vertical_line(int x, int y_start, int y_stop, uint16_t color)
{
	for(int i = y_start; i < y_stop; i++){
		lcd_put_pixel(x, i, color);
	}
}

void lcd_draw_horizontal_line_dotted(int y, int x_start, int x_stop, uint16_t color)
{
	for(int i = x_start; i <= x_stop; i++)
	{
		if((i % 2) == 0)
		{
			lcd_put_pixel(i, y, color);
		}
	}
}

void lcd_draw_vertical_line_dotted(int x, int y_start, int y_stop, uint16_t color)
{
	for(int i = y_start; i < y_stop; i++)
	{
		if((i % 2) == 0)
		{
			lcd_put_pixel(x, i, color);
		}
	}
}

void LCD_DrawCircle ( 	int X_Center, int Y_Center, int Radius, uint16_t color)
{
	//Draw a circle from (0, R) as a starting point
	int16_t XCurrent, YCurrent;
	XCurrent = 0;
	YCurrent = Radius;

	//Cumulative error,judge the next point of the logo
	int16_t Esp = 3 - ( Radius << 1 );

		while ( XCurrent <= YCurrent )
		{
			lcd_put_pixel ( X_Center + XCurrent, Y_Center + YCurrent, color);             //1
			lcd_put_pixel ( X_Center - XCurrent, Y_Center + YCurrent, color);             //2
			lcd_put_pixel ( X_Center - YCurrent, Y_Center + XCurrent, color);             //3
			lcd_put_pixel ( X_Center - YCurrent, Y_Center - XCurrent, color);             //4
			lcd_put_pixel ( X_Center - XCurrent, Y_Center - YCurrent, color);             //5
			lcd_put_pixel ( X_Center + XCurrent, Y_Center - YCurrent, color);             //6
			lcd_put_pixel ( X_Center + YCurrent, Y_Center - XCurrent, color);             //7
			lcd_put_pixel ( X_Center + YCurrent, Y_Center + XCurrent, color);             //0

			if ( Esp < 0 )
			{
				Esp += 4 * XCurrent + 6;
			}
			else
			{
				Esp += 10 + 4 * ( XCurrent - YCurrent );
				YCurrent --;
			}

			XCurrent ++;
		}
	}
