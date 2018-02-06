/* mbed SPI_STMPE610.h to test adafruit 2.8" TFT LCD shield w Touchscreen
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
 /*
  * Note: Since the interrupt siganl of the shield was not connected
  * to an interrupt pin on my frdm-kl25z, I just used polling mode.
  */
  /*
   * Note: To make this work with FRDM-K64F
   * PTA0 must be disconnected from the swd clk by cutting J11.
   * But to re-active SWD you need to put jumper header to J11
   * so that it can be re-connected by a jumper.
   */
#ifndef SPI_STMPE610_H
#define SPI_STMPE610_H

#include "mbed.h"


/** SPI_STMPE610 Touch Sensor
 * Example:
 * @code
 * #include "mbed.h"
 * #include "SPI_STMPE610.h"
 *
 * #define PIN_MOSI        PTD2
 * #define PIN_MISO        PTD3 
 * #define PIN_SCLK        PTD1 
 * #define PIN_CS_TSC      PTA13
 * #define PIN_TSC_INTR    PTC9
 * 
 * SPI_STMPE610 TSC(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TSC) ;
 *
 * int main()
 * {
 *    uint16_t touched, x, y, z ;
 *    printf("Test SPI STMPE610\n\r") ;
 *    while (true) {
 *        touched = TSC.getRAWPoint(&x, &y, &z) ;
 *        if (touched) {
 *            printf("x = %d, y = %d, z = %d\n\r", x, y, z) ;
 *        }
 *        wait(0.1) ;
 *    }
 * }
 * @endcode
 */

class SPI_STMPE610 
{
public:
 /**
 * STMPE610 constructor
 *
 * @param mosi SPI_MOSI pin
 * @param miso SPI_MISO pin
 * @param sclk SPI_CLK pin
 * @param cs   SPI_CS  pin
 */
 
 SPI_STMPE610(PinName mosi, PinName miso, PinName sclk, PinName cs) ;
 
 /** 
  * Destructor 
  */
 ~SPI_STMPE610() ;

 /*
  * some member functions here (yet to be written)
  */

 SPI m_spi;
 DigitalOut m_cs ;
 int _mode ;
 
  void readRegs(int addr, uint8_t *data, int len) ;
  void writeRegs(uint8_t *data, int len) ;
  uint8_t read8(int addr) ;
  void write8(int addr, uint8_t data) ;
  uint16_t read16(int addr) ;
  void write16(int addr, uint16_t data) ;
  
  /**
   * get RAW value of x, y, z 
   * @param *x  raw value of x
   * @param *y  raw value of y
   * @param *z  raw value of z
   * @return if touched 
   * @note For my device usually the value seems to be between 300 ~ 3000 
   * @note when it fails to acquire value the value of 0 seems to be returned
   */
  int getRAWPoint(uint16_t *x, uint16_t *y, uint16_t *z) ;
  
  private:
} ;

#endif /* SPI_STMPE610_H */
