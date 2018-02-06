/* mbed main.cpp to test adafruit 2.8" TFT LCD shiled w Touchscreen
 * Copyright (c) 2014 Motoo Tanaka @ Design Methodology Lab
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
 /* *
  * @note This program is derived from the SeeeStudioTFTv2 program.
  * @note Although both program share same ILI9341 TFT driver,
  * @note the touch sensor was not same with the Display I purchased from Akizuki.
  * @note http://akizukidenshi.com/catalog/g/gM-07747/
  * @note The touch sensor on the display is STMPE610,
  * @note so I hacked the minimum spi driver for it (polling mode only).
  */
/**
 * @note To make this work with FRDM-K64F
 * @note PTA0 must be disconnected from the swd clk by cutting J11.
 * @note But to re-active SWD you need to put jumper header to J11
 * @note so that it can be re-connected by a jumper.
 */

#include "mbed.h"
#include "MMA8451Q.h"
#include <math.h>
#include "SPI_TFT_ILI9341.h"
#include "SPI_STMPE610.h"
#include "Arial12x12.h"
#include "Arial24x23.h"
#include "Arial28x28.h"
#include "font_big.h"

#if 0 
/* 
// For FRDM-KL25Z
#define PIN_XP          PTB3
#define PIN_XM          PTB1
#define PIN_YP          PTB2
#define PIN_YM          PTB0
#define PIN_MOSI        PTD2
#define PIN_MISO        PTD3 
#define PIN_SCLK        PTD1 
#define PIN_CS_TFT      PTD0 
#define PIN_DC_TFT      PTD5 
#define PIN_BL_TFT      PTC9 
#define PIN_CS_SD       PTA4 
#define PIN_CS_TSC      PTA13
#define PIN_TSC_INTR    PTC9
#define PIN_BACKLIGHT   PTA12
*/
#endif

#if 1
// For FRDM-K64F
#define PIN_XP          PTB11
#define PIN_XM          PTB3
#define PIN_YP          PTB10
#define PIN_YM          PTB2
#define PIN_MOSI        PTD2
#define PIN_MISO        PTD3 
#define PIN_SCLK        PTD1 
#define PIN_CS_TFT      PTD0 
#define PIN_DC_TFT      PTC4 
#define PIN_BL_TFT      PTC3 
#define PIN_CS_SD       PTB23 
// for board rev E or later
//#define PIN_CS_TSC      PTC12
// for earlier boards use following line
 #define PIN_CS_TSC      PTA0
#define PIN_TSC_INTR    PTC3
#define PIN_BACKLIGHT   PTA1
#endif


#define MMA8451_I2C_ADDRESS (0x1d<<1)
MMA8451Q *acc = 0 ;

// SeeedStudioTFTv2 TFT(PIN_XP, PIN_XM, PIN_YP, PIN_YM, PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TFT, PIN_DC_TFT, PIN_BL_TFT, PIN_CS_SD);
//  SPI_TFT_ILI9341(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset, PinName dc, const char* name ="TFT");
SPI_TFT_ILI9341 TFT(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TFT, PIN_BL_TFT, PIN_DC_TFT) ;
SPI_STMPE610 TSC(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TSC) ;

DigitalOut backlight(PIN_BACKLIGHT) ;
DigitalIn pinD7(PIN_TSC_INTR) ;

int page = 0 ;
int numPage = 3 ;

void initTFT(void)
{
    //Configure the display driver
    TFT.background(Black);
    TFT.foreground(White);
    wait(0.01) ;
    TFT.cls();
}

void screen1(void) // Welcome Screen
{
    backlight = 0 ;
    TFT.background(White) ;
    TFT.cls() ;

    wait(0.1) ;
    TFT.set_font((unsigned char*) Arial24x23);
    TFT.foreground(Red) ;
    TFT.locate(80, 40) ;
    TFT.printf("mbed") ;
    TFT.foreground(Blue);
    TFT.locate(60, 80) ;
    TFT.printf("TFT 2.8\"") ; 
    TFT.locate(40, 120) ;
    TFT.printf("with touch") ;
    TFT.foreground(Black);
    TFT.set_font((unsigned char*) Arial12x12);
    TFT.foreground(Blue) ;
    TFT.locate(10, 180) ;
    TFT.printf("This program is running on") ;
    TFT.locate(10, 200) ;
    TFT.printf("freescale FRDM-K64F with") ;
    TFT.locate(10, 220) ;
    TFT.printf("a program developed on mbed") ;
    TFT.foreground(Green) ;
    TFT.locate(10, 260) ;
    TFT.printf("To advance demo page, touch") ;
    TFT.locate(10, 280) ;
    TFT.printf("and hold right side of screen") ;
    TFT.locate(10, 300) ;
    TFT.printf("until the next screen starts") ;
    backlight = 1 ;
}

void screen2(void) // Graphics
{
    //Draw some graphics
    int i, x[2], y[2] ;
    backlight = 0 ;

    TFT.background(Black);
    TFT.foreground(White);
    TFT.cls() ;
    TFT.set_font((unsigned char*) Arial12x12);
    TFT.locate(90,0);
    TFT.printf("Graphics");
    
    x[0] = 25 ; x[1] = 224 ;
    y[0] = 20 ; y[1] = 219 ;
    for (i = 20 ; i < 220 ; i += 10) {
        TFT.line(i+5, y[0], i+5, y[1], Blue) ;
        TFT.line(x[0], i, x[1], i, Blue) ;
    }
    TFT.line(125, y[0], 125, y[1], Green) ;
    TFT.line(x[0], 120, x[1], 120, Green) ;
    TFT.rect(x[0],y[0], x[1], y[1], Green) ;
    TFT.locate(10, 20) ;
    TFT.printf("V") ;
    TFT.locate(0, 115) ;
    TFT.printf("0.0") ;
    TFT.locate(115, 225) ;
    TFT.printf("0.0") ;
    TFT.locate(215, 225) ;
    TFT.printf("T") ;

    double s;
    for (int i = x[0]; i < 225; i++) {
        s = 40 * sin((long double)i / 20);
        TFT.pixel(i, 120 + (int)s, White);
    }
    
    TFT.fillrect(10, 240, 229, 309, White) ;
    TFT.rect(10, 240, 229, 309, Red) ;
    TFT.rect(11, 241, 228, 308, Red) ;
    
    TFT.background(White) ;
    TFT.foreground(Black) ;
    TFT.locate(20, 250) ;
    TFT.printf("With QVGA resolution") ;
    TFT.locate(20, 270) ;
    TFT.printf("simple graphics drawing") ;
    TFT.locate(20, 290) ;
    TFT.printf("capability is provided") ;
    backlight = 1 ;
}    

double clip(double src)
{
    double value ;
    value = src ;
    if (value < 0.0) {
        value = 0.0 ;
    } else if (value > 2.0) {
        value = 2.0 ;
    }
    return( value ) ;
}

void screen3(void)
{
    int t = 0 ;
    int pt = 0 ; // previous t 
    int i, x, y ;
    unsigned int data[3] ; // for x, y, z 
    unsigned int prev[3] ;
    unsigned short signalHeight = 39 ;
    unsigned short xoffset = 30 ;
    unsigned short yoffset = 120 ;
    unsigned short zoffset = 210 ;
    unsigned short paneX[2] = {20, 235} ;
    unsigned short paneH = 81 ;
 
    backlight = 0 ;
    TFT.background(Black) ;
    TFT.foreground(White) ;
//    TFT.cls() ;

 
    TFT.fillrect(paneX[0], xoffset, paneX[1], xoffset+paneH, Black) ;
    TFT.fillrect(paneX[0], yoffset, paneX[1], yoffset+paneH, Black) ;
    TFT.fillrect(paneX[0], zoffset, paneX[1], zoffset+paneH, Black) ;
    TFT.fillrect(paneX[0], xoffset, paneX[1], xoffset+paneH, Black) ;
    for (i = 0 ; i < 10 ; i++ ) {
        y = i * 8 ;
        TFT.line(paneX[0], xoffset + y, paneX[1], xoffset + y, Blue) ;
        TFT.line(paneX[0], yoffset + y, paneX[1], yoffset + y, Blue) ;
        TFT.line(paneX[0], zoffset + y, paneX[1], zoffset + y, Blue) ;
    }
    for (x = 30 ; x < paneX[1] ; x += 10 ) {
        TFT.line(x, xoffset, x, xoffset+paneH, Blue) ;
        TFT.line(x, yoffset, x, yoffset+paneH, Blue) ;
        TFT.line(x, zoffset, x, zoffset+paneH, Blue) ;
    } 
    TFT.rect(paneX[0], xoffset, paneX[1], xoffset+paneH, White) ;
    TFT.rect(paneX[0], yoffset, paneX[1], yoffset+paneH, White) ;
    TFT.rect(paneX[0], zoffset, paneX[1], zoffset+paneH, White) ;
    TFT.set_font((unsigned char*) Arial12x12);
    TFT.locate(5, xoffset+30) ;
    TFT.printf("X") ;
    TFT.locate(5, yoffset+30) ;
    TFT.printf("Y") ;
    TFT.locate(5, zoffset+30) ;
    TFT.printf("Z") ;
    TFT.locate(50, 10) ;
    TFT.printf("Xtrinsic Accelerometer") ;
    TFT.locate(90, 300) ;
    TFT.printf("MMA8451Q") ;
    
    prev[0] = xoffset + (signalHeight * clip((1.0 + acc->getAccX()))) ;
    prev[1] = yoffset + (signalHeight * clip((1.0 + acc->getAccY()))) ;
    prev[2] = zoffset + (signalHeight * clip((1.0 + acc->getAccZ()))) ;
    pt = paneX[0] ;
    backlight = 1 ;
    for(t = 21 ; t < paneX[1] ; t++) {
        data[0] = xoffset + (signalHeight * clip((1.0 + acc->getAccX()))) ;
        data[1] = yoffset + (signalHeight * clip((1.0 + acc->getAccY()))) ;
        data[2] = zoffset + (signalHeight * clip((1.0 + acc->getAccZ()))) ;
        TFT.line(pt, prev[0], t, data[0], Red) ;
        TFT.line(pt, prev[1], t, data[1], Green) ;
        TFT.line(pt, prev[2], t, data[2], Yellow) ;
        prev[0] = data[0] ;
        prev[1] = data[1] ;
        prev[2] = data[2] ;
        pt = t ;
        wait(0.01) ;
    }
}

void incPage(void)
{
    page++ ;
    if (page >= numPage) {
        page = 0 ;
    }
}

void decPage(void) 
{
    page-- ;
    if (page < 0) {
        page = numPage - 1 ;
    }
}

void screen4(void)
{
    int t = 0 ;
    int pt = 0 ; // previous t 
    int i, x, y ;
    unsigned int data[3] ; // for x, y, z 
    unsigned int prev[3] ;
    unsigned short signalHeight = 39 ;
    unsigned short xoffset = 30 ;
    unsigned short yoffset = 120 ;
    unsigned short zoffset = 210 ;
    unsigned short paneX[2] = {20, 235} ;
    unsigned short paneH = 81 ;
 
    backlight = 0 ;
    TFT.background(Black) ;
    TFT.foreground(White) ;
//    TFT.cls() ;

 
    TFT.fillrect(paneX[0], xoffset, paneX[1], xoffset+paneH, Black) ;
    TFT.fillrect(paneX[0], yoffset, paneX[1], yoffset+paneH, Black) ;
    TFT.fillrect(paneX[0], zoffset, paneX[1], zoffset+paneH, Black) ;
    TFT.fillrect(paneX[0], xoffset, paneX[1], xoffset+paneH, Black) ;
    for (i = 0 ; i < 10 ; i++ ) {
        y = i * 8 ;
        TFT.line(paneX[0], xoffset + y, paneX[1], xoffset + y, Blue) ;
        TFT.line(paneX[0], yoffset + y, paneX[1], yoffset + y, Blue) ;
        TFT.line(paneX[0], zoffset + y, paneX[1], zoffset + y, Blue) ;
    }
    for (x = 30 ; x < paneX[1] ; x += 10 ) {
        TFT.line(x, xoffset, x, xoffset+paneH, Blue) ;
        TFT.line(x, yoffset, x, yoffset+paneH, Blue) ;
        TFT.line(x, zoffset, x, zoffset+paneH, Blue) ;
    } 
    TFT.rect(paneX[0], xoffset, paneX[1], xoffset+paneH, White) ;
    TFT.rect(paneX[0], yoffset, paneX[1], yoffset+paneH, White) ;
    TFT.rect(paneX[0], zoffset, paneX[1], zoffset+paneH, White) ;
    TFT.set_font((unsigned char*) Arial12x12);
    TFT.locate(5, xoffset+30) ;
    TFT.printf("X") ;
    TFT.locate(5, yoffset+30) ;
    TFT.printf("Y") ;
    TFT.locate(5, zoffset+30) ;
    TFT.printf("Z") ;
    TFT.locate(50, 10) ;
    TFT.printf("Xtrinsic Accelerometer") ;
    TFT.locate(90, 300) ;
    TFT.printf("MMA8451Q") ;
    
    prev[0] = xoffset + (signalHeight * clip((1.0 + acc->getAccX()))) ;
    prev[1] = yoffset + (signalHeight * clip((1.0 + acc->getAccY()))) ;
    prev[2] = zoffset + (signalHeight * clip((1.0 + acc->getAccZ()))) ;
    pt = paneX[0] ;
    backlight = 1 ;
    for(t = 21 ; t < paneX[1] ; t++) {
        data[0] = xoffset + (signalHeight * clip((1.0 + acc->getAccX()))) ;
        data[1] = yoffset + (signalHeight * clip((1.0 + acc->getAccY()))) ;
        data[2] = zoffset + (signalHeight * clip((1.0 + acc->getAccZ()))) ;
        TFT.line(pt, prev[0], t, data[0], Red) ;
        TFT.line(pt, prev[1], t, data[1], Green) ;
        TFT.line(pt, prev[2], t, data[2], Yellow) ;
        prev[0] = data[0] ;
        prev[1] = data[1] ;
        prev[2] = data[2] ;
        pt = t ;
        wait(0.01) ;
    }
}
    
int main()
{
    uint16_t x, y, z ;
    int prevPage = 0 ;
    int touched = 0 ;
    
 
    
    initTFT() ;
    
    screen1() ;
        
    printf("Program Started!\n\r") ;
    
    acc = new MMA8451Q(PTE25, PTE24, MMA8451_I2C_ADDRESS) ;
    
		
		TFT.background(Black) ;
		TFT.foreground(White) ;
		TFT.cls() ;
	
    for(;;) {
//        printf("TFT width = %d, height = %d\n\r", TFT.width(), TFT.height()) ;
        switch(page) {
        case 0:
            if (prevPage != page) {
                screen1() ;
            }
            wait(1) ;
            break ;
        case 1:
            if (prevPage != page) {
                screen2() ; 
            }
            wait(1) ;
            break ;
        case 2:
            if (prevPage != page) {
                TFT.background(Black) ;
                TFT.foreground(White) ;
                TFT.cls() ;
            }
            screen3() ; 
            wait(2) ;
            break ;
        default:
            page = 0 ; 
            break ;
        }
        prevPage = page ;
        printf("Screen Printed\n\r") ;

        x = 0 ; y = 0 ; z = 0 ;
        touched = TSC.getRAWPoint(&x, &y, &z) ;
        printf("touched = %d x = %d, y = %d, z = %d\n\r",touched, x,y,z) ;
        if ((x != 0)||(y != 0) || (z != 0)) {
            if (x < 1000) { // left
                decPage() ;
            } else if (x > 3000) { // right
                incPage() ;
            }
        }
//        wait(1) ;
    }
}

