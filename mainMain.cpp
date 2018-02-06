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

#include <stdio.h>

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
		if (paused && !printed) {
			wait(0.5);
			TFT.locate(TFT.width()/2-10, TFT.height()/2);
//			TFT.printf("||");
			printed = 1;
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

/* Labyrinth set-up */

/* All of these are initialized at the start of the program */
/* Maze dimensions */
static int MAZE_WIDTH = 0;
static int MAZE_HEIGHT = 0;
/* Start the ball in the upper left corner? */
int START_X_POS = 0;
int START_Y_POS = 0;
/*Finish point CENTER is near the center right*/
int FINISH_X_POS = 0;
int FINISH_Y_POS = 0;

static const int WALL_FATNESS = 8;

static const int BACKGROUND = Black;
static const int FOREGROUND = White;
static const int FRAME_COLOR = Maroon;
static const int WALL_COLOR = Maroon;
static const int HOLE_COLOR = DarkGrey;
static const int WIN_COLOR = Green;
static const int BALL_COLOR = White;

static int numWalls = 0;
static int numHoles = 0;

// Wall object
typedef struct wall {
	/*(x,y)= top left corner of the wall */
	int x_pos;
	int y_pos;
	
	/* Wall color */
	int color;

	/* Wall dimensions */
	int width; //horizontal span (how fat)
	int length; //vertical span (how tall)
	int ori; // orientation (0 for horizontal, 1 for vertical)
	
} Wall;

/*Basic Wall constructor*/
Wall createWall(int x, int y, int xSize, int ySize, int color){
	Wall wall;
	wall.x_pos = x;
	wall.y_pos = y;
	wall.color = color;
	wall.width = xSize;
	wall.length = ySize;
	wall.ori = (ySize > xSize);
	
	TFT.fillrect(x, y, x+xSize, y+ySize, color);

	return wall;
}

/* Hole/Win object */
typedef struct zone {
	/*(x,y) denotes center of the zone */
	int x_pos;
	int y_pos;

	/* Radius of the zone */
	int radius;
	/* Color of the zone */
	int color;
	
	int type; //0 = hole, 1 = win zone, 2 = restart/end game
	
} Zone;


/* Constructor for holes/win */
Zone createZone(int x, int y, int radius, int color, int type){
	Zone zone;

	zone.x_pos = x;
	zone.y_pos = y;

	zone.radius = radius;
	zone.color = color;
	
	zone.type = type;
	
	TFT.fillcircle(x, y, radius, color);

	return zone;
} 

/* Marble functions */

// Initializations
static const int BALL_RADIUS = 6;
static const int HOLE_RADIUS = BALL_RADIUS+4;

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

int wonGame = 0;
int lostGame = 0;

/* Win/Lose/Restart game - finish ball rolling, print ending */
ball marbleDrop(Ball ball, Zone zone) {
	for (int i=3;i>0;i--) {
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,BACKGROUND) ;
		ball.x_pos = ball.x_pos - (ball.x_pos - zone.x_pos)/i;
		ball.y_pos = ball.y_pos - (ball.y_pos - zone.y_pos)/i;
		TFT.fillcircle(zone.x_pos,zone.y_pos,zone.radius,zone.color) ;
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,ball.color) ;
		wait(0.05);
	}
	wait(0.05);
	
	TFT.fillcircle(zone.x_pos,zone.y_pos,zone.radius,zone.color) ;
	
	TFT.set_font((unsigned char*) Arial28x28);
	TFT.locate(MAZE_WIDTH/4, MAZE_HEIGHT/2-20) ;
	if (zone.type == 1) {
		wait(0.5);
		TFT.cls();
		wait(0.5);
		TFT.printf("you won! :D") ;
	} else if (zone.type == 0) {
		wait(0.5);
		TFT.cls();
		wait(0.5);
		TFT.printf("you lost :(") ;
	}
	wait(3);
	return ball;
}

// check for wall collisions and return coordinate
int checkWalls(int newX, int newY, Ball ball, Wall wall) {
		int wallMinX = bound(wall.x_pos-ball.radius,0.0,MAZE_WIDTH);
		int wallMaxX = bound(wall.x_pos+wall.width+ball.radius,0.0,MAZE_WIDTH);
		int wallMinY = bound(wall.y_pos-ball.radius,0.0,MAZE_HEIGHT);
		int wallMaxY = bound(wall.y_pos+wall.length+ball.radius,0.0,MAZE_HEIGHT);
	
    int coord = 0;
    if (newX > wallMinX-1 && newX < wallMaxX+1 &&
				newY > wallMinY-1 && newY < wallMaxY+1) {
			coord = 1;
		}
    return coord ;
}

/* Recalculation of ball's movement, and checks */
ball marbleRoll(Ball ball, Zone win, wall arrayWalls[12], zone arrayHoles[13])
{
	backlight = 1 ;
	
	// x,y accelerations are flipped due to orientation
	ball.xAcc = (20 * (acc->getAccY())) ;
	ball.yAcc = (20 * (acc->getAccX())) ;
	
	// get new positions
	int newX = bound(ball.x_pos - ball.xAcc,ball.radius,MAZE_WIDTH-ball.radius);
	int newY = bound(ball.y_pos + ball.yAcc,ball.radius,MAZE_HEIGHT-ball.radius);
	
	// wall collision check
	for (int i=0;i<numWalls;i++) {
		Wall wall = arrayWalls[i];
		
		int inWall = checkWalls(newX, newY, ball, wall);
	
		if (inWall) {
			// Sliding
			if (newX+ball.radius > wall.x_pos+wall.width || newX-ball.radius < wall.x_pos) {
				newX = ball.x_pos;
			}
			if (newY+ball.radius > wall.y_pos || newY-ball.radius < wall.y_pos+wall.length) {
				newY = ball.y_pos;
			}
			
			if (newX+ball.radius == wall.x_pos) {
				newX -= 0.1;
			} else if (newX-ball.radius == wall.x_pos+wall.width) {
				newX += 0.1;
			}
			if (newY+ball.radius == wall.y_pos) {
				newY += 0.1;
			} else if (newY-ball.radius == wall.y_pos+wall.length) {
				newY -= 0.1;
			}
		}
	}
	
	// update only if moved
	if (!(newX == ball.x_pos && newY == ball.y_pos)) {
		// erase ball
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,BACKGROUND) ;
		
		ball.x_pos = newX;
		ball.y_pos = newY;
		
		// redraw win zone
		int distFromWin = (int) sqrt(1.0*((ball.x_pos-win.x_pos)*(ball.x_pos-win.x_pos)) + 1.0*((ball.y_pos-win.y_pos)*(ball.y_pos-win.y_pos)));
		if (distFromWin < 2*win.radius) {
			TFT.fillcircle(win.x_pos,win.y_pos,win.radius,win.color) ;
		}
		// redraw holes
		for (int i=0;i<numHoles;i++) {
			Zone hole = arrayHoles[i];
			int distFromHole = (int) sqrt(1.0*((ball.x_pos-hole.x_pos)*(ball.x_pos-hole.x_pos)) + 1.0*((ball.y_pos-hole.y_pos)*(ball.y_pos-hole.y_pos)));
			if (distFromHole < 2*hole.radius) {
				TFT.fillcircle(hole.x_pos,hole.y_pos,hole.radius,hole.color) ;
			}
		}
		
		// redraw ball
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,ball.color) ;
		
		// check if won
		if (abs(ball.x_pos - win.x_pos) < win.radius && abs(ball.y_pos - win.y_pos) < win.radius) {
			wonGame = 1;
			ball = marbleDrop(ball, win);
		} else {
		// check if died
			for (int i=0;i<numHoles;i++) {
				Zone hole = arrayHoles[i];
				if (abs(ball.x_pos - hole.x_pos) <= hole.radius && abs(ball.y_pos - hole.y_pos) <= hole.radius) {
					lostGame = 1;
				}
				if (lostGame) {
					ball = marbleDrop(ball, hole);
					break;
				}
			}
		}
	}
	return ball;
}

int playAgain = 1;
void Game(void) {
	for (;;) {
		TFT.cls();
		wonGame = 0;
		lostGame = 0;
		printed = 0;
		paused = 0;
		// Initialize labyrinth / walls
		/*fillrect() -> drawrect() for an outline if you want.*/
		/* set WALL_COLOR above if you want red instead of black, etc */
		numWalls = 12;
		struct wall arrayWalls[numWalls];
		arrayWalls[0] = createWall(0, 0, MAZE_WIDTH, WALL_FATNESS, FRAME_COLOR);
		arrayWalls[1] = createWall(0, 0, WALL_FATNESS, MAZE_HEIGHT, FRAME_COLOR);
		arrayWalls[2] = createWall(0, MAZE_HEIGHT - WALL_FATNESS, MAZE_WIDTH, WALL_FATNESS, FRAME_COLOR);
		arrayWalls[3] = createWall(MAZE_WIDTH - WALL_FATNESS, 0, WALL_FATNESS, MAZE_HEIGHT, FRAME_COLOR);

		arrayWalls[4] = createWall(0, 50, 230, WALL_FATNESS, WALL_COLOR); //first horizontal wall
		arrayWalls[5] = createWall(214, 100, MAZE_WIDTH-214, WALL_FATNESS, WALL_COLOR); //second horizontal wall
		arrayWalls[6] = createWall(50, 150, 164, WALL_FATNESS, WALL_COLOR); // third horizontal wall
		arrayWalls[7] = createWall(50, 100, WALL_FATNESS, 100, WALL_COLOR); //leftmost vertical wall
		arrayWalls[8] = createWall(150, 50, WALL_FATNESS, 50+WALL_FATNESS, WALL_COLOR); //uppermiddle vertical wall
		arrayWalls[9] = createWall(106, 185, WALL_FATNESS, MAZE_HEIGHT-110, WALL_COLOR); //lowermiddle vertical wall
		arrayWalls[10] =  createWall(214, 100, WALL_FATNESS, 105, WALL_COLOR); //right most vertical wall
		arrayWalls[11] =  createWall(MAZE_WIDTH-5*HOLE_RADIUS-WALL_FATNESS, FINISH_Y_POS+2*HOLE_RADIUS-2, HOLE_RADIUS*9+WALL_FATNESS, WALL_FATNESS, WALL_COLOR); //win wall
		
		numHoles = 13;
		struct zone arrayHoles[numHoles];

		arrayHoles[0] = createZone(FINISH_X_POS, WALL_FATNESS + 10 + HOLE_RADIUS, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[1] = createZone(WALL_FATNESS + 6 + HOLE_RADIUS, 73, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[2] = createZone(243, 175, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[3] = createZone(100, 100, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[4] = createZone(295, 215, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[5] = createZone(60, 35, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[6] = createZone(125, 25, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[7] = createZone(214, 35, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[8] =  createZone(182, 130, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[9] = createZone(WALL_FATNESS + 6 + HOLE_RADIUS, MAZE_HEIGHT - WALL_FATNESS - 6 - HOLE_RADIUS, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[10] = createZone(157, 180, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[11] = createZone(MAZE_WIDTH*3/4+20, 75, HOLE_RADIUS, HOLE_COLOR, 0);
		arrayHoles[12] = createZone(WALL_FATNESS + 17 + HOLE_RADIUS, MAZE_HEIGHT/2+24, HOLE_RADIUS, HOLE_COLOR, 0);
		
		// Initialize win zone
		Zone win = createZone(FINISH_X_POS, FINISH_Y_POS, HOLE_RADIUS, WIN_COLOR, 1);
		
		// Initialize ball
		Ball ball = createBall(START_X_POS, START_Y_POS, BALL_RADIUS, BALL_COLOR);
		
		for(;;) {
			if (paused) {
				//do nothing
			} else if (wonGame || lostGame) {
				if (!printed) {
					wait(0.5);
					TFT.cls();
					wait(0.5);
					TFT.set_font((unsigned char*) Arial28x28);
					TFT.locate(MAZE_WIDTH/4+4, MAZE_HEIGHT/2-65) ;
					TFT.printf("play again?");
					TFT.set_font((unsigned char*) Arial24x23);
					TFT.locate(MAZE_WIDTH/4+15, MAZE_HEIGHT/2+30) ;
					TFT.printf("yes");
					TFT.locate(MAZE_WIDTH/2+23, MAZE_HEIGHT/2+30) ;
					TFT.printf("no");
					printed = 1;
					
					// Initialize ball
					ball = createBall(START_X_POS, START_Y_POS, BALL_RADIUS, BALL_COLOR);
				}
				
				// Initialize "yes"
				Zone yes = createZone(TFT.width()*3/8, TFT.height()/2, HOLE_RADIUS*2, Green, 2);
				
				// Initialize "no"
				Zone no = createZone(TFT.width()*5/8, TFT.height()/2, HOLE_RADIUS*2, Red, 2);
				
				if (printed) {
					
					// x,y accelerations are flipped due to orientation
					ball.xAcc = (40 * (acc->getAccY())) ;
					ball.yAcc = (40 * (acc->getAccX())) ;
					
					// get new positions
					int newX = bound(ball.x_pos - ball.xAcc,ball.radius,MAZE_WIDTH-ball.radius);
					int newY = bound(ball.y_pos + ball.yAcc,ball.radius,MAZE_HEIGHT-ball.radius);
		
					// update only if moved
					if (!(newX == ball.x_pos && newY == ball.y_pos)) {
						// erase ball
						TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,BACKGROUND) ;
						
						ball.x_pos = newX;
						ball.y_pos = newY;
						
						// redraw ball
						TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,ball.color) ;
					}
					
					// do they want to restart?
					if (abs(ball.x_pos - yes.x_pos) < yes.radius && abs(ball.y_pos - yes.y_pos) < yes.radius) {
						ball = marbleDrop(ball, yes);
						playAgain = 1;
						break;
					} else if (abs(ball.x_pos - no.x_pos) < no.radius && abs(ball.y_pos - no.y_pos) < no.radius) {
						ball = marbleDrop(ball, no);
						playAgain = 0;
						break;
					}
					wait(0.01);
				}
			} else {
				printed = 0;
				ball = marbleRoll(ball,win,arrayWalls,arrayHoles) ;
			}
			wait(0.01);

		}
		if (!playAgain) {
			TFT.cls();
			TFT.set_font((unsigned char*) Arial24x23);
			TFT.locate(TFT.width()/4+10, TFT.height()/2-40);
			TFT.printf("thank you");
			TFT.locate(TFT.width()/4-10, TFT.height()/2+10);
			TFT.printf("for playing!");
			break;
		}
//		wait(0.1);
	}
}

/* Recalculation of ball's movement, and checks */
ball marbleRoll2(Ball ball, Zone win, wall arrayWalls[2], zone arrayHoles[1])
{
	backlight = 1 ;
	
	// x,y accelerations are flipped due to orientation
	ball.xAcc = (20 * (acc->getAccY())) ;
	ball.yAcc = (20 * (acc->getAccX())) ;
	
	// get new positions
	int newX = bound(ball.x_pos - ball.xAcc,ball.radius,MAZE_WIDTH-ball.radius);
	int newY = bound(ball.y_pos + ball.yAcc,ball.radius,MAZE_HEIGHT-ball.radius);
	
	// wall collision check
	for (int i=0;i<numWalls;i++) {
		Wall wall = arrayWalls[i];
		
		int inWall = checkWalls(newX, newY, ball, wall);
	
		if (inWall) {
			// Sliding
			if (newX+ball.radius > wall.x_pos+wall.width || newX-ball.radius < wall.x_pos) {
				newX = ball.x_pos;
			}
			if (newY+ball.radius > wall.y_pos || newY-ball.radius < wall.y_pos+wall.length) {
				newY = ball.y_pos;
			}
			
			if (newX+ball.radius == wall.x_pos) {
				newX -= 0.1;
			} else if (newX-ball.radius == wall.x_pos+wall.width) {
				newX += 0.1;
			}
			if (newY+ball.radius == wall.y_pos) {
				newY += 0.1;
			} else if (newY-ball.radius == wall.y_pos+wall.length) {
				newY -= 0.1;
			}
		}
	}
	
	// update only if moved
	if (!(newX == ball.x_pos && newY == ball.y_pos)) {
		// erase ball
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,BACKGROUND) ;
		
		ball.x_pos = newX;
		ball.y_pos = newY;
		
		// redraw win zone
		int distFromWin = (int) sqrt(1.0*((ball.x_pos-win.x_pos)*(ball.x_pos-win.x_pos)) + 1.0*((ball.y_pos-win.y_pos)*(ball.y_pos-win.y_pos)));
		if (distFromWin < 2*win.radius) {
			TFT.fillcircle(win.x_pos,win.y_pos,win.radius,win.color) ;
		}
		// redraw holes
		for (int i=0;i<numHoles;i++) {
			Zone hole = arrayHoles[i];
			int distFromHole = (int) sqrt(1.0*((ball.x_pos-hole.x_pos)*(ball.x_pos-hole.x_pos)) + 1.0*((ball.y_pos-hole.y_pos)*(ball.y_pos-hole.y_pos)));
			if (distFromHole < 2*hole.radius) {
				TFT.fillcircle(hole.x_pos,hole.y_pos,hole.radius,hole.color) ;
			}
		}
		
		// redraw ball
		TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,ball.color) ;
		
		// check if won
		if (abs(ball.x_pos - win.x_pos) < win.radius && abs(ball.y_pos - win.y_pos) < win.radius) {
			wonGame = 1;
			ball = marbleDrop(ball, win);
		} else {
		// check if died
			for (int i=0;i<numHoles;i++) {
				Zone hole = arrayHoles[i];
				if (abs(ball.x_pos - hole.x_pos) <= hole.radius && abs(ball.y_pos - hole.y_pos) <= hole.radius) {
					lostGame = 1;
				}
				if (lostGame) {
					ball = marbleDrop(ball, hole);
					break;
				}
			}
		}
	}
	return ball;
}

void Instr(void) {
	
	TFT.cls();
	
	// Initialize ball
	Ball ball = createBall(START_X_POS*2, MAZE_HEIGHT/6, BALL_RADIUS, BALL_COLOR);
	
	// Initialize walls
	numWalls = 2;
	struct wall arrayWalls[numWalls];
	arrayWalls[0] =  createWall(0, MAZE_HEIGHT/3, MAZE_WIDTH*3/4, WALL_FATNESS, WALL_COLOR); //top wall
	arrayWalls[1] =  createWall(MAZE_WIDTH/4, MAZE_HEIGHT*2/3, MAZE_WIDTH*3/4, WALL_FATNESS, WALL_COLOR); //bottom wall
	
	// Initialize hole
	numHoles = 1;
	struct zone arrayHoles[numWalls];
	arrayHoles[0] = createZone(MAZE_WIDTH/2, MAZE_HEIGHT/2+WALL_FATNESS, HOLE_RADIUS*2, HOLE_COLOR, 2);
	
	// Initialize win zone
	Zone win = createZone(MAZE_WIDTH-START_X_POS*2, MAZE_HEIGHT*5/6+WALL_FATNESS, HOLE_RADIUS, WIN_COLOR, 2);
		
	// Print Text
	TFT.set_font((unsigned char*) Arial12x12);
	
	TFT.locate(MAZE_WIDTH/4, MAZE_HEIGHT/4-20) ;
	TFT.printf("follow the path...") ;
	for (;;) {
		ball = marbleRoll2(ball, win, arrayWalls, arrayHoles);
		if (wonGame) {
			wonGame=0;
			ball = createBall(MAZE_WIDTH*3/4, MAZE_HEIGHT/2+WALL_FATNESS, BALL_RADIUS, BALL_COLOR);
		}
		if (lostGame || (ball.x_pos < arrayHoles[0].x_pos-arrayHoles[0].radius && ball.y_pos > arrayWalls[0].y_pos)) {
			TFT.set_font((unsigned char*) Arial12x12);
			TFT.locate(MAZE_WIDTH/2+35, MAZE_HEIGHT/2) ;
			TFT.printf("avoid the holes!") ;
			break;
		}
		wait(0.01);
	}
	
	if (lostGame) {
		ball = createBall(MAZE_WIDTH/4, MAZE_HEIGHT/2+WALL_FATNESS, BALL_RADIUS, BALL_COLOR);
		lostGame=0;
	}
	
	int printed3=0;
	for (;;) {
		if (!printed3) {
			if (ball.y_pos > arrayWalls[1].y_pos+WALL_FATNESS) {
				TFT.set_font((unsigned char*) Arial12x12);
				TFT.locate(MAZE_WIDTH/4, MAZE_HEIGHT*3/4+20) ;
				TFT.printf("...get to the goal!") ;
				printed3=1;
			}
		}
		ball = marbleRoll2(ball, win, arrayWalls, arrayHoles);
		if (lostGame) {
			lostGame=0;
			ball = createBall(MAZE_WIDTH/4, MAZE_HEIGHT/2+WALL_FATNESS, BALL_RADIUS, BALL_COLOR);
		}
		if (wonGame) {
			wonGame=0;
			break;
		}
		wait(0.01);
	}

	return;
}

void Ready(void) {
	
	TFT.cls();
	
	wait(0.2);
	TFT.set_font((unsigned char*) Arial28x28);
	TFT.locate(MAZE_WIDTH/4+20, MAZE_HEIGHT/4-20) ;
	TFT.printf("READY?") ;
	wait(0.5);
	
	// Initialize ball
	Ball ball = createBall(START_X_POS*2, MAZE_HEIGHT/2, BALL_RADIUS, BALL_COLOR);
	
	// Initialize win zone
	Zone win = createZone(MAZE_WIDTH-START_X_POS*2, MAZE_HEIGHT/2, HOLE_RADIUS*2, WIN_COLOR, 2);
	for (;;) {
		
		// x,y accelerations are flipped due to orientation
		ball.xAcc = (20 * (acc->getAccY())) ;
		ball.yAcc = (20 * (acc->getAccX())) ;
		
		// get new positions
		int newX = bound(ball.x_pos - ball.xAcc,ball.radius,MAZE_WIDTH-ball.radius);
		int newY = bound(ball.y_pos + ball.yAcc,ball.radius,MAZE_HEIGHT-ball.radius);

		// update only if moved
		if (!(newX == ball.x_pos && newY == ball.y_pos)) {
			// erase ball
			TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,BACKGROUND) ;
			
			ball.x_pos = newX;
			ball.y_pos = newY;
			
			// redraw win zone
			int distFromWin = (int) sqrt(1.0*((ball.x_pos-win.x_pos)*(ball.x_pos-win.x_pos)) + 1.0*((ball.y_pos-win.y_pos)*(ball.y_pos-win.y_pos)));
			if (distFromWin < 2*win.radius) {
				TFT.fillcircle(win.x_pos,win.y_pos,win.radius,win.color) ;
			}
			// redraw ball
			TFT.fillcircle(ball.x_pos,ball.y_pos,ball.radius,ball.color) ;
			
			// check if won
			if (abs(ball.x_pos - win.x_pos) < win.radius && abs(ball.y_pos - win.y_pos) < win.radius) {
				wonGame = 1;
				ball = marbleDrop(ball, win);
			}
		}
		
		if (wonGame) {
			wonGame=0;
			break;
		}
		wait(0.01);
	}
	
	return;
	
}

void Start(void) {
	TFT.set_font((unsigned char*) Neu42x35);
	TFT.locate(MAZE_WIDTH/8-2, MAZE_HEIGHT/2-20) ;
	TFT.printf("LABYRINTH") ;
	wait(5);
	
	return;
}
    
int main()
{    
	// Initialize screen
	initTFT() ;
	TFT.set_orientation(3);
	MAZE_WIDTH = TFT.width();
	MAZE_HEIGHT = TFT.height();
	START_X_POS = WALL_FATNESS + BALL_RADIUS + 5;
	START_Y_POS = WALL_FATNESS + BALL_RADIUS + 5;
	FINISH_X_POS = MAZE_WIDTH - WALL_FATNESS - (2*BALL_RADIUS+10);
	FINISH_Y_POS = MAZE_HEIGHT/2+5;

	backlight = 0 ;
	TFT.background(BACKGROUND) ;
	TFT.foreground(FOREGROUND) ;
	
	// Initialize accelerometer inputting thing
	acc = new MMA8451Q(PTE25, PTE24, MMA8451_I2C_ADDRESS) ;
	
	// Start / intro / ready
	Start();
	Instr();
	Ready();
	wait(0.5);
	
	// Start game
	Game();
    
}

