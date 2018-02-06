// MyTouch.h - Declaration class (driver for) of touchpanel based on ADS7843 or XPT2046
// version: 1.0
// author:  Mchał Pyszka
// date:    28 NOV 2014
//
#ifndef MyTouch_h
#define MyTOUCH_H

#include "mbed.h"

#define USE_12BITS  0
#define USE_8BITS   2

#define     TP_GETX_8BIT     0x98     
#define     TP_GETY_8BIT     0xD8
#define     TP_GETX_12BIT    0x90
#define     TP_GETY_12BIT    0xD0

#define TP_X 0
#define TP_Y 1

#define TP_FREQUENCY 500000

class MYTOUCH
    {
    public:
        MYTOUCH(PinName _tp_mosi, PinName _tp_miso, PinName _tp_sclk, PinName _tp_cs, PinName _tp_irq, unsigned char _resolution, void (*pointer_To_TP_IRQ_Handler)(void)) ;
        unsigned int TP_GetX(){return TP_Get(TP_X);}
        unsigned int TP_GetY(){return TP_Get(TP_Y);}
        void TP_ChangeMode(unsigned char _tp_mode){tp_mode = _tp_mode ;}
    protected:
        unsigned int TP_Get(unsigned char XY) ;
    
        unsigned char tp_mode ;
        SPI tp_spi ;
        DigitalOut tp_cs ;
        InterruptIn tp_irqhandler ;
        unsigned char argument_type[4] ;
    } ;

#endif

		