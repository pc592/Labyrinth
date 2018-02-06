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
#include "hx8340bs.h"
#include "helpers.h"

HX8340S_LCD::HX8340S_LCD( PinName CS, PinName RESET, PinName SCL, PinName SDI, PinName BL, backlight_t blType, float defaultBackLightLevel )
    : LCD( 176, 220, CS, NC, RESET, BL, blType, defaultBackLightLevel ), _lcd_pin_scl( SCL ), _lcd_pin_sdi( SDI )
{
}

void HX8340S_LCD::Initialize( orientation_t orientation, colordepth_t colors )
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
    _lcd_pin_scl = HIGH;
    _lcd_pin_sdi = HIGH;
    if ( _lcd_pin_bl != 0 )
        *_lcd_pin_bl = HIGH;
    else if ( _bl_pwm != 0 )
        *_bl_pwm = _bl_pwm_default;
    wait_ms( 55 );
    
    Activate();
    WriteCmd( 0xC1 ); // SETEXTCMD
    WriteByteData( 0xFF );
    WriteByteData( 0x83 );
    WriteByteData( 0x40 );
    
    WriteCmd( 0x11 ); // SLPOUT
    wait_ms( 160 );
    
    WriteCmd( 0xCA );
    WriteByteData( 0x70 );
    WriteByteData( 0x00 );
    WriteByteData( 0xD9 );
    
    WriteCmd( 0xB0 ); // SETOSC
    WriteByteData( 0x01 );
    WriteByteData( 0x11 );

    WriteCmd( 0xC9 );
    WriteByteData( 0x90 );
    WriteByteData( 0x49 );
    WriteByteData( 0x10 );
    WriteByteData( 0x28 );
    WriteByteData( 0x28 );
    WriteByteData( 0x10 );
    WriteByteData( 0x00 );
    WriteByteData( 0x06 );
    wait_ms( 20 );
    
    WriteCmd( 0xC2 ); // SETGAMMAP
    WriteByteData( 0x60 );
    WriteByteData( 0x71 );
    WriteByteData( 0x01 );
    WriteByteData( 0x0E );
    WriteByteData( 0x05 );
    WriteByteData( 0x02 );
    WriteByteData( 0x09 );
    WriteByteData( 0x31 );
    WriteByteData( 0x0A );

    WriteCmd( 0xc3 ); // SETGAMMAN
    WriteByteData( 0x67 );
    WriteByteData( 0x30 );
    WriteByteData( 0x61 );
    WriteByteData( 0x17 );
    WriteByteData( 0x48 );
    WriteByteData( 0x07 );
    WriteByteData( 0x05 );
    WriteByteData( 0x33 );
    wait_ms( 10 );
    
    WriteCmd( 0xB5 ); // SETPWCTR5
    WriteByteData( 0x35 );
    WriteByteData( 0x20 );
    WriteByteData( 0x45 );

    WriteCmd( 0xB4 ); // SETPWCTR4
    WriteByteData( 0x33 );
    WriteByteData( 0x25 );
    WriteByteData( 0x4c );
    wait_ms( 10 );
    
    WriteCmd( 0x3A ); // COLMOD == color depth: 0x05 => 16bit, 0x06 => 18bit
    WriteByteData( _colorDepth == RGB16 ? 0x05 : 0x06 );
    
    WriteCmd( 0x36 ); // MADCTL
    switch ( _orientation )
    {
        case LANDSCAPE: WriteByteData( 0xB8 ); break;
        case PORTRAIT_REV: WriteByteData( 0xDC ); break;
        case LANDSCAPE_REV: WriteByteData( 0x6C ); break;
        case PORTRAIT:
        default: WriteByteData( 0x08 ); break;
    }
    
    WriteCmd( 0x29 ); // DISPON
    wait_ms( 10 );
    
    ClearXY();
    Deactivate();
}

void HX8340S_LCD::Sleep( void )
{
    Activate();
    WriteCmd( 0x28 );
    wait_ms( 10 );
    WriteCmd( 0x10 );
    wait_ms( 125 );
    LCD::Sleep();
    Deactivate();
}

void HX8340S_LCD::WakeUp( void )
{
    Activate();
    WriteCmd( 0x29 );
    wait_ms( 10 );
    WriteCmd( 0x11 );
    wait_ms( 125 );
    LCD::WakeUp();
    Deactivate();
}

void HX8340S_LCD::WriteCmd( unsigned short cmd )
{
    _lcd_pin_sdi = LOW;
    pulseLow( _lcd_pin_scl );
    serializeByte( cmd & 0xFF );
}

void HX8340S_LCD::WriteData( unsigned short data )
{
    _lcd_pin_sdi = HIGH;
    pulseLow( _lcd_pin_scl );
    serializeByte( ( data >> 8 ) & 0xFF );
    _lcd_pin_sdi = HIGH;
    pulseLow( _lcd_pin_scl );
    serializeByte( data & 0xFF );
}

void HX8340S_LCD::WriteByteData( unsigned char data )
{
    _lcd_pin_sdi = HIGH;
    pulseLow( _lcd_pin_scl );
    serializeByte( data );
}

void HX8340S_LCD::SetXY( unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2 )
{
    WriteCmdData( 0x2A, x1 );  // CASET
    WriteData( x2 );
    WriteCmdData( 0x2B, y1 );  // PASET
    WriteData( y2 );
    WriteCmd( 0x2C ); // RAMWR
}

void HX8340S_LCD::SetPixelColor( unsigned int color, colordepth_t mode )
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

void HX8340S_LCD::serializeByte( unsigned char data )
{
    for ( int i = 0; i < 8; i++ )
    {
        if ( data & 0x80 ) _lcd_pin_sdi = HIGH;
        else _lcd_pin_sdi = LOW;
        pulseLow( _lcd_pin_scl );
        data = data << 1;
    }
}
