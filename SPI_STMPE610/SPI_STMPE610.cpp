/* mbed SPI_STMPE610.cpp to test adafruit 2.8" TFT LCD shield w Touchscreen
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
#include "SPI_STMPE610.h"

/* some definitions here */
#define REG_CHIP_ID       0x00
#define REG_CHIP_ID_MSB   0x00
#define REG_CHIP_ID_LSB   0x01

#define REG_ID_VER        0x02
#define REG_SYS_CTRL1     0x03
#define REG_SYS_CTRL1_RESET 0x02

#define REG_SYS_CTRL2     0x04
#define REG_SPI_CFG       0x08
#define REG_INT_CTRL      0x09
#define REG_INT_CTRL_POL_HIGH 0x04
#define REG_INT_CTRL_POL_LOW  0x00
#define REG_INT_CTRL_EDGE     0x02
#define REG_INT_CTRL_LEVEL    0x00
#define REG_INT_CTRL_ENABLE   0x01
#define REG_INT_CTRL_DISABLE  0x00

#define REG_INT_EN        0x0A
#define REG_INT_EN_TOUCHDET  0x01
#define REG_INT_EN_FIFOTH    0x02
#define REG_INT_EN_FIFOOF    0x04
#define REG_INT_EN_FIFOFULL  0x08
#define REG_INT_EN_FIFOEMPTY 0x10
#define REG_INT_EN_ADC       0x40

#define REG_INT_STA       0x0B
#define REG_INT_STA_TOUCHDET 0x01

#define REG_GPIO_EN       0x0C
#define REG_GPIO_INT_STA  0x0D
#define REG_ADC_INT_EN    0x0E
#define REG_ADC_INT_STA   0x0F
#define REG_GPIO_SET_PIN  0x10
#define REG_GPIO_CLR_PIN  0x11
#define REG_GPIO_MP_STA   0x12
#define REG_GPIO_DIR      0x13
#define REG_GPIO_ED       0x14
#define REG_GPIO_RE       0x15
#define REG_GPIO_FE       0x16
#define REG_GPIO_AF       0x17
#define REG_ADC_CTRL1     0x20
#define REG_ADC_CTRL1_12BIT 0x08
#define REG_ADC_CTRL1_10BIT 0x00

#define REG_ADC_CTRL2     0x21
#define REG_ADC_CTRL2_1_625MHZ 0x00
#define REG_ADC_CTRL2_3_25MHZ  0x01
#define REG_ADC_CTRL2_6_5MHZ   0x02

#define REG_ADC_CAPT      0x22
#define REG_ADC_DATA_CH0  0x30
#define REG_ADC_DATA_CH1  0x32
#define REG_ADC_DATA_CH4  0x38
#define REG_ADC_DATA_CH5  0x3A
#define REG_ADC_DATA_CH6  0x3C
#define REG_ADC_DATA_CH7  0x3E
#define REG_TSC_CTRL      0x40
#define REG_TSC_CTRL_EN     0x01
#define REG_TSC_CTRL_XYZ    0x00
#define REG_TSC_CTRL_XY     0x02

#define REG_TSC_CFG       0x41
#define REG_TSC_CFG_1SAMPLE      0x00
#define REG_TSC_CFG_2SAMPLE      0x40
#define REG_TSC_CFG_4SAMPLE      0x80
#define REG_TSC_CFG_8SAMPLE      0xC0
#define REG_TSC_CFG_DELAY_10US   0x00
#define REG_TSC_CFG_DELAY_50US   0x08
#define REG_TSC_CFG_DELAY_100US  0x10
#define REG_TSC_CFG_DELAY_500US  0x18
#define REG_TSC_CFG_DELAY_1MS    0x20
#define REG_TSC_CFG_DELAY_5MS    0x28
#define REG_TSC_CFG_DELAY_10MS   0x30
#define REG_TSC_CFG_DELAY_50MS   0x38
#define REG_TSC_CFG_SETTLE_10US  0x00
#define REG_TSC_CFG_SETTLE_100US 0x01
#define REG_TSC_CFG_SETTLE_500US 0x02
#define REG_TSC_CFG_SETTLE_1MS   0x03
#define REG_TSC_CFG_SETTLE_5MS   0x04
#define REG_TSC_CFG_SETTLE_10MS  0x05
#define REG_TSC_CFG_SETTLE_50MS  0x06
#define REG_TSC_CFG_SETTLE_100MS 0x07

#define REG_WDW_TR_X      0x42
#define REG_WDW_TR_Y      0x44
#define REG_WDW_BL_X      0x46
#define REG_WDW_BL_Y      0x48
#define REG_FIFO_TH       0x4A
#define REG_FIFO_STA      0x4B
#define REG_FIFO_SIZE     0x4C
#define REG_TSC_DATA_X    0x4D
#define REG_TSC_DATA_Y    0x4F
#define REG_TSC_DATA_Z    0x51
#define REG_TSC_DATA_XYZ  0x52
#define REG_TSC_FRACT_XYZ 0x56
#define REG_TSC_DATA      0x57
#define REG_TSC_I_DRIVE   0x58
#define REG_TSC_SHIELD    0x59

SPI_STMPE610::SPI_STMPE610(PinName mosi, PinName miso, PinName sclk, PinName cs) :
        m_spi(mosi, miso, sclk), m_cs(cs) {
    // activate the peripheral
    m_cs = 0 ;
    _mode = 0 ;
    m_spi.frequency(1000000) ;
    m_spi.format(8, 0) ;
    write8(REG_SYS_CTRL1, REG_SYS_CTRL1_RESET) ;
    wait(0.1) ;
    write8(REG_SYS_CTRL2, 0x00) ; // turn on clocks
    write8(REG_TSC_CFG,
          REG_TSC_CFG_4SAMPLE 
        | REG_TSC_CFG_DELAY_100US
        | REG_TSC_CFG_SETTLE_1MS ) ;
        
    write8(REG_TSC_CTRL, REG_TSC_CTRL_XYZ | REG_TSC_CTRL_EN) ;   
    m_cs = 1 ;
}

SPI_STMPE610::~SPI_STMPE610() { }

void SPI_STMPE610::readRegs(int addr, uint8_t * data, int len) {
    m_cs = 0 ;

    for (int i = 0 ; i < len ; i++ ) {    
       m_spi.write((addr+i)|0x80) ;  // spacify address to read
       data[i] = m_spi.write((addr+i)|0x80) ; 
    } 
    m_spi.write(0x00) ; // to terminate read mode
    m_cs = 1 ;
}

void SPI_STMPE610::writeRegs(uint8_t * data, int len) {
   m_cs = 0 ;
   for (int i = 0 ; i < len ; i++ ) {
      m_spi.write(data[i]) ;
   }
   m_cs = 1 ;
}

void SPI_STMPE610::write8(int addr, uint8_t data8)
{
    uint8_t data[2] ;
    data[0] = addr ;
    data[1] = data8 ;
    writeRegs(data, 2) ;
}

uint8_t SPI_STMPE610::read8(int addr)
{
    uint8_t data[1] ;    
    readRegs(addr, data, 1) ;
    return( data[0] ) ;
}

void SPI_STMPE610::write16(int addr, uint16_t data16)
{
    uint8_t data[3] ;
    data[0] = addr ;
    data[1] = (data16 >> 8) & 0xFF ;
    data[2] = data16 & 0xFF ;
    writeRegs(data, 3) ;
}

uint16_t SPI_STMPE610::read16(int addr)
{
    uint8_t data[2] ;
    uint16_t value = 0 ;
    readRegs(addr, data, 2) ;
    value = (data[0] << 8) | data[1] ;
    return( value ) ;
}

int SPI_STMPE610::getRAWPoint(uint16_t *x, uint16_t *y, uint16_t *z)
{
    uint8_t data[8], touched = 0 ;
    data[0] = REG_TSC_CTRL ;
    data[1] = REG_TSC_CTRL_EN  ; 
    writeRegs(data, 2) ;
    wait(0.01) ;
     
    readRegs(REG_TSC_CTRL, data, 1) ;
    touched = data[0] & 0x80 ;

    readRegs(REG_TSC_DATA_X, data,5) ;
        *x = (data[0] << 8) | data[1] ;
        *y = (data[2] << 8) | data[3] ;
        *z = data[4] ;
        
    data[0] = 0x4B ;
    data[1] = 0x01 ;
    writeRegs(data, 2)  ; // clear FIFO
 
    data[0] = REG_TSC_CTRL ;
    data[1] = 0x00 ; // disable TSC 
    writeRegs(data, 2) ;
    
    return( touched ) ;
}
