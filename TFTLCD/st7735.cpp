/*
 * Copyright (C)2010-2012 Henning Karlsen. All right reserved.
 * Copyright (C)2012 Todor Todorov.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to:
 *
 * Free Software Foundation, Inc.
 * 51 Franklin St, 5th Floor, Boston, MA 02110-1301, USA
 *
 *********************************************************************/
#include "st7735.h"
#include "helpers.h"

ST7735_LCD::ST7735_LCD( PinName CS, PinName RESET, PinName RS, PinName SCL, PinName SDA, PinName BL, backlight_t blType, float defaultBackLightLevel )
    : LCD( 128, 160, CS, RS, RESET, BL, blType, defaultBackLightLevel ), _lcd_pin_scl( SCL ), _lcd_pin_sda( SDA )
{
}

void ST7735_LCD::Initialize( orientation_t orientation, colordepth_t colors )
{
    _orientation = orientation;
    _colorDepth = colors;
    
    wait_ms( 100 );
    _lcd_pin_reset = HIGH;
    wait_ms( 5 );
    _lcd_pin_reset = LOW;
    wait_ms( 15 );
    _lcd_pin_reset = HIGH;
    _lcd_pin_cs = HIGH;
    _lcd_pin_rs = HIGH;
    _lcd_pin_scl = HIGH;
    _lcd_pin_sda = HIGH;
    if ( _lcd_pin_bl != 0 )
        *_lcd_pin_bl = HIGH;
    else if ( _bl_pwm != 0 )
        *_bl_pwm = _bl_pwm_default;
    wait_ms( 55 );
    
    Activate();
    WriteCmd( 0x01 ); // SW reset
    wait_ms( 120 );
    
    WriteCmd( 0x11 ); // sleep out
    wait_ms( 120 );
    
    WriteCmd( 0xB1 ); // frame control 1
    WriteByteData( 0x01 );
    WriteByteData( 0x2C );
    WriteByteData( 0x2D );
    
    WriteCmd( 0xB2 ); // frame control 2
    WriteByteData( 0x01 );
    WriteByteData( 0x2C );
    WriteByteData( 0x2D );
    
    WriteCmd( 0xB3 ); // frame control 3
    WriteByteData( 0x01 );
    WriteByteData( 0x2C );
    WriteByteData( 0x2D );
    WriteByteData( 0x01 );
    WriteByteData( 0x2C );
    WriteByteData( 0x2D );
    
    WriteCmd( 0xB4 ); // column inversion
    //WriteByteData( 0x07 );
    WriteByteData( 0x00 );

    // ST7735R Power Sequence
    WriteCmd( 0xC0 ); // power control 1
    WriteByteData( 0xA2 );
    WriteByteData( 0x02 );
    WriteByteData( 0x84 );
    
    WriteCmd( 0xC1 ); // power control 2
    WriteByteData( 0xC5 );
    
    WriteCmd( 0xC2 ); // power control 3
    WriteByteData( 0x0A );
    WriteByteData( 0x00 );
    
    WriteCmd( 0xC3 ); // power control 4
    WriteByteData( 0x8A );
    WriteByteData( 0x2A );
    
    WriteCmd( 0xC4 ); // power control 5
    WriteByteData( 0x8A );
    WriteByteData( 0xEE );
    
    WriteCmd( 0xC5 ); // voltage control 1
    WriteByteData( 0x0E );

    // ST7735R Gamma Sequence
    WriteCmd( 0xE0 ); // gamma positive
    WriteByteData( 0x0F );
    WriteByteData( 0x1A );
    WriteByteData( 0x0F );
    WriteByteData( 0x18 );
    WriteByteData( 0x2F );
    WriteByteData( 0x28 );
    WriteByteData( 0x20 );
    WriteByteData( 0x22 );
    WriteByteData( 0x1F );
    WriteByteData( 0x1B );
    WriteByteData( 0x23 );
    WriteByteData( 0x37 );
    WriteByteData( 0x00 );
    WriteByteData( 0x07 );
    WriteByteData( 0x02 );
    WriteByteData( 0x10 );
    
    WriteCmd( 0xE1 ); // gamma negative
    WriteByteData( 0x0F );
    WriteByteData( 0x1B );
    WriteByteData( 0x0F );
    WriteByteData( 0x17 );
    WriteByteData( 0x33 );
    WriteByteData( 0x2C );
    WriteByteData( 0x29 );
    WriteByteData( 0x2E );
    WriteByteData( 0x30 );
    WriteByteData( 0x30 );
    WriteByteData( 0x39 );
    WriteByteData( 0x3F );
    WriteByteData( 0x00 );
    WriteByteData( 0x07 );
    WriteByteData( 0x03 );
    WriteByteData( 0x10 );

    WriteCmd( 0x2A ); // set column address
    WriteByteData( 0x00 );
    WriteByteData( 0x00 );
    WriteByteData( 0x00 );
    WriteByteData( 0x7F );
    
    WriteCmd( 0x2B ); // set row address
    WriteByteData( 0x00 );
    WriteByteData( 0x00 );
    WriteByteData( 0x00 );
    WriteByteData( 0x9F );

    WriteCmd( 0xF0 ); // enable extensions command
    WriteByteData( 0x01 );
    
    WriteCmd( 0xF6 ); // disable ram power save mode
    WriteByteData( 0x00 );

    WriteCmd( 0x3A ); // interface pixel format (color mode): 0x05 => RGB16, 0x06 => RGB18
    WriteByteData( _colorDepth == RGB16 ? 0x05 : 0x06 );

    WriteCmd( 0x36 ); //MX, MY, RGB mode
    switch ( _orientation )
    {
        case LANDSCAPE: WriteByteData( 0x6C ); break;
        case PORTRAIT_REV: WriteByteData( 0xDC ); break;
        case LANDSCAPE_REV: WriteByteData( 0xB8 ); break;
        case PORTRAIT:
        default: WriteByteData( 0x08 ); break;
    }
    
    WriteCmd( 0x29 ); // display on

    Deactivate();
}

void ST7735_LCD::Sleep( void )
{
    Activate();
    WriteCmd( 0x28 );
    wait_ms( 10 );
    WriteCmd( 0x10 );
    wait_ms( 125 );
    LCD::Sleep();
    Deactivate();
}

void ST7735_LCD::WakeUp( void )
{
    Activate();
    WriteCmd( 0x29 );
    wait_ms( 10 );
    WriteCmd( 0x11 );
    wait_ms( 125 );
    LCD::WakeUp();
    Deactivate();
}

void ST7735_LCD::WriteCmd( unsigned short cmd )
{
    _lcd_pin_rs = LOW;
    serializeByte( cmd & 0xFF );
}

void ST7735_LCD::WriteData( unsigned short data )
{
    _lcd_pin_rs = HIGH;
    serializeByte( ( data >> 8 ) & 0xFF );
    serializeByte( data & 0xFF );
}

void ST7735_LCD::WriteByteData( unsigned char data )
{
    _lcd_pin_rs = HIGH;
    serializeByte( data );
}

void ST7735_LCD::SetXY( unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2 )
{
    WriteCmdData( 0x2a, x1 );
    WriteData( x2 );
    WriteCmdData( 0x2b, y1 );
    WriteData( y2 );
    WriteCmd( 0x2c );
}

void ST7735_LCD::SetPixelColor( unsigned int color, colordepth_t mode )
{
    unsigned char r = 0, g = 0, b = 0;
    unsigned short clr;
    if ( _colorDepth == RGB16 )
    {
        switch ( mode )
        {
            case RGB16:
                WriteData( color & 0xFFFF );
                break;
            case RGB18:
                r = ( color >> 10 ) & 0xF8;
                g = ( color >> 4 ) & 0xFC;
                b = ( color >> 1 ) & 0x1F;
                clr = ( ( r | ( g >> 5 ) ) << 8 ) | ( ( g << 3 ) | b );
                WriteData( clr );
                break;
            case RGB24:
                r = ( color >> 16 ) & 0xF8;
                g = ( color >> 8 ) & 0xFC;
                b = color & 0xF8;
                clr = ( ( r | ( g >> 5 ) ) << 8 ) | ( ( g << 3 ) | ( b >> 3 ) );
                WriteData( clr );
                break;
        }
    }
    else if ( _colorDepth == RGB18 )
    {
        switch ( mode )
        {
            case RGB16:
                r = ( ( color >> 8 ) & 0xF8 ) | ( ( color & 0x8000 ) >> 13 );
                g = ( color >> 3 ) & 0xFC;
                b = ( ( color << 3 ) & 0xFC ) | ( ( color >> 3 ) & 0x01 );
                break;
            case RGB18:
                b = ( color << 2 ) & 0xFC;
                g = ( color >> 4 ) & 0xFC;
                r = ( color >> 10 ) & 0xFC;
                break;
            case RGB24:
                r = ( color >> 16 ) & 0xFC;
                g = ( color >> 8 ) & 0xFC;
                b = color & 0xFC;
                break;
        }
        WriteByteData( r );
        WriteByteData( g );
        WriteByteData( b );
    }
}

void ST7735_LCD::serializeByte( unsigned char data )
{
    for ( int i = 0; i < 8; i++ )
    {
        if ( data & 0x80 ) _lcd_pin_sda = HIGH;
        else _lcd_pin_sda = LOW;
        pulseLow( _lcd_pin_scl );
        data = data << 1;
    }
}
