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
//#include "SPI_STMPE610.h"
#include "Arial12x12.h"
#include "Arial24x23.h"
#include "Arial28x28.h"
#include "font_big.h"
#include "mbed.h"
//#include "ssd1289.h"
#include "MyTouch.h"

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
#define PIN_CS_TSC      PTA2
#define PIN_TSC_INTR    PTC2
#define PIN_BACKLIGHT   PTA1
#endif


#define MMA8451_I2C_ADDRESS (0x1d<<1)
MMA8451Q *acc = 0 ;
void mytouch_irq_handler(void) ;
volatile unsigned int xT, yT ;

// SeeedStudioTFTv2 TFT(PIN_XP, PIN_XM, PIN_YP, PIN_YM, PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TFT, PIN_DC_TFT, PIN_BL_TFT, PIN_CS_SD);
//  SPI_TFT_ILI9341(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset, PinName dc, const char* name ="TFT");
SPI_TFT_ILI9341 TFT(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TFT, PIN_BL_TFT, PIN_DC_TFT) ;
//SPI_STMPE610 TSC(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TSC) ;
// mosi, miso, sck, cs, irq pin, mode, pointer to touch panel irq handler
MYTOUCH MyTouch( PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TSC, PIN_TSC_INTR, USE_12BITS, &mytouch_irq_handler);

DigitalIn pinD7(PIN_TSC_INTR) ;
DigitalOut backlight(PIN_BACKLIGHT) ;

/* Interrupt handler for touch screen */
int paused = 0;
int printed = 0;
void mytouch_irq_handler(void)
{
//		paused = 1-paused;
		paused = 1-paused;
		if (paused) {
			TFT.fillcircle(300,60,10,Red) ;
			wait(0.5);
		} else {
			TFT.fillcircle(300,60,10,Green) ;
		}
		
		wait(0.1);
}

/* Screen initialization */
void initTFT(void)
{
    //Configure the display driver
    TFT.background(Black);
    TFT.foreground(White);
    wait(0.01) ;
    TFT.cls();
}

/* Marble functions */

// Initializations
static const int BALL_RADIUS = 6;
static const int HOLE_RADIUS = BALL_RADIUS+2;

/* Ball object */
typedef struct ball {
	/* Position variable; note (x, y) denotes center of ball*/
	int x_pos;
	int y_pos;

	/* Dimension variable; for collision check (x/yPos +/- radius = some bound) */
	int radius;
	/**color */
	int color;

	/* Accelerometer variables; number will be generated from accelerometer 
	   output and ball will move proportionally */
	int xAcc;
	int yAcc;
} Ball;

/*Default constructor creates ball at start location*/
Ball createBall(int x_start, int y_start, int radius, int color){
	Ball ball; 
	ball.x_pos = x_start;
	ball.y_pos = y_start;
	ball.radius = radius;
	ball.color = color;
	ball.xAcc = 0;
	ball.yAcc = 0;
	
	TFT.fillcircle(x_start,y_start,radius,color) ;

	return ball;
}

/* Basic bounds for screen edges */
// May be unecessary / need to be modified for walls
int bound(int src, int min, int max)
{
    int value ;
    value = src ;
    if (value < min) {
        value = min ;
    } else if (value > 1.0*max-1) {
        value = max-1 ;
    }
    return( value ) ;
}

/* Recalculation of ball's movement, and checks */
ball marbleRoll(Ball ball) {
	backlight = 1 ;
	
	// x,y accelerations are flipped due to orientation
	ball.xAcc = (25 * (acc->getAccY())) ;
	ball.yAcc = (25 * (acc->getAccX())) ;
	
	// get new positions
	int newX = bound(ball.x_pos + ball.xAcc,ball.radius,TFT.width()-ball.radius);
	int newY = bound(ball.y_pos - ball.yAcc,ball.radius,TFT.height()-ball.radius);
	
	// update only if moved
	if (!(newX == ball.x_pos && newY == ball.y_pos)) {
		// erase ball
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,Black) ;
		
		ball.x_pos = newX;
		ball.y_pos = newY;
		
		// redraw ball
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,ball.color) ;
	}
	return ball;
}

void Game(void) {
	for (;;) {
		
		// Initialize ball
		Ball ball = createBall(50, 50, 15, White);
		
		for(;;) {
			if (paused) {
				if (!printed) {
					TFT.locate(TFT.width()/2-10, TFT.height()/2);
					TFT.printf("paused");
					printed = 1;
				}
			} else {
				printed = 0;
				ball = marbleRoll(ball) ;
			}
			wait(0.02);

		}
	}
}
    
int main()
{    
	// Initialize screen
	initTFT() ;
	TFT.set_orientation(1);
	
	backlight = 0 ;
	TFT.background(Black) ;
	TFT.foreground(White) ;

	// Initialize text font
	// (if text desired; location will always have to be located though)
	TFT.set_font((unsigned char*) Arial12x12);
	
	// Initialize accelerometer inputting thing
	acc = new MMA8451Q(PTE25, PTE24, MMA8451_I2C_ADDRESS) ;
	
	// Start game
	Game();
    
}

