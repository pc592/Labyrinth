#include "mbed.h"
#include "ssd1289.h"
#include "MyTouch.h"

// **********************************************************************
// make bus for lcd data
//* // prepare the data bus for writing commands and pixel data
BusOut lcdDataBus( PC_0, PC_1, PC_2, PC_3, PC_4, PC_5, PC_6, PC_7, PC_8, PC_9, PC_10, PC_11, PC_12, PC_13, PC_14, PC_15 ); // 16 pins pc13-pb7

void mytouch_irq_handler(void) ;
volatile unsigned int x, y ;

// create LCD class instance
SSD1289_LCD lcd( PA_11, PA_12, PB_13, PB_14, &lcdDataBus ); //CS, RESET, RS, WR, DATA_PORT
// create MYTOUCH class instance
MYTOUCH MyTouch( PA_7, PA_6, PA_5, PB_6, PB_8, USE_12BITS, &mytouch_irq_handler); // mosi, miso, sck, cs, irq pin, mode, pointer to touch panel irq handler

void mytouch_irq_handler(void)
    {
        float xf, yf ;
        x = MyTouch.TP_GetX();
        y = MyTouch.TP_GetY();
        xf = x ;
        yf = y ;
        xf = (xf/1024)*320 ;
        yf = (yf/1024)*240 ;
        x = (int)xf ;
        y = (int)yf ;
        lcd.DrawCircle(x,y,2, COLOR_GREEN) ;
        wait(0.2);
    }
// **********************************************************************


int main() {
    char text[24] ;
    x = 0 ;
    y = 0 ;
    // initialize display - place it in standard portrait mode and set background to black and
    //                      foreground to white color.
    lcd.Initialize(LANDSCAPE, RGB16);
    lcd.ClearScreen() ;
    lcd.FillScreen(COLOR_BLACK) ;
    lcd.SetForeground(COLOR_YELLOW);
    lcd.SetBackground(COLOR_BLACK) ;
    
    // set current font to the smallest 8x12 pixels font.
    lcd.SetFont(&TerminusFont ) ;    // print something on the screen
    while(1)
        {        
        sprintf(text,"X%d Y%d    ", x, y ) ;
        lcd.Print(text, 15, 15 ) ;
        wait(0.5) ;
        }
}
