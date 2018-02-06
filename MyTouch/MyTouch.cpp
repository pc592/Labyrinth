#include "mbed.h"
#include "MyTouch.h"

MYTOUCH::MYTOUCH(PinName _tp_mosi, PinName _tp_miso, PinName _tp_sclk, PinName _tp_cs, PinName _tp_irq, unsigned char _resolution, void (*pointer_To_TP_IRQ_Handler)(void))
        :tp_spi(_tp_mosi,_tp_miso,_tp_sclk),tp_cs(_tp_cs),tp_irqhandler(_tp_irq)
        {
        tp_cs = 1 ;
        tp_spi.frequency(TP_FREQUENCY) ;
        tp_spi.format(8,0) ;
        
        argument_type[0] = TP_GETX_12BIT ;
        argument_type[1] = TP_GETY_12BIT ;
        argument_type[2] = TP_GETX_8BIT ;
        argument_type[3] = TP_GETY_8BIT ;
        
        if (pointer_To_TP_IRQ_Handler!=NULL)
        tp_irqhandler.fall(pointer_To_TP_IRQ_Handler) ;
        
        tp_mode = _resolution ;
        }
        
unsigned int MYTOUCH::TP_Get(unsigned char XY)
    {
    unsigned char hi, low;
    unsigned int tmp;
 
    tmp=0;
    tp_cs.write(0);
    wait_us(1);
    tp_spi.write(argument_type[tp_mode+XY]);
    wait_us(1);
    if (tp_mode==USE_12BITS)
        {
        hi = tp_spi.write(0x00);  // hi
        wait_us(1);
        }
    low = tp_spi.write(0x00);  // low
    tp_cs.write(1);
    tmp = ((hi << 8 ) | low);
    tmp >>= 3;
    tmp &= 0xfff;
    tmp /= 4;                  // between 0 and 1024
    return tmp;
    }
