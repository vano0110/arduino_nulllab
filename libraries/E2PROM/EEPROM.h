/*
  EEPROM.h - EEPROM library
  Copyright (c) 2006 David A. Mellis.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef EEPROM_h
#define EEPROM_h

#include <inttypes.h>

#ifndef EEROM_SIZE
#define EEROM_SIZE 1
#endif

#define	e2pReset()	do { ECCR |= 0x20; } while(0)
#define	e2pSWMON()	do { ECCR = 0x80; ECCR |= 0x10; } while(0);
#define	e2pSWMOFF()	do { ECCR = 0x80; ECCR &= 0xEF; } while(0);
uint8_t eerom_read(uint16_t address);
void eerom_write(uint16_t address, uint8_t value);

/***
    EERef class.
    
    This object references an EEPROM cell.
    Its purpose is to mimic a typical byte of RAM, however its storage is the EEPROM.
    This class has an overhead of two bytes, similar to storing a pointer to an EEPROM cell.
***/

struct EERef{

    EERef( const int index )
        : index( index )                 {}
    
    //Access/read members.
    uint8_t operator*() const            { return eerom_read( (uint8_t*) index ); }
    operator uint8_t() const             { return **this; }
    
    //Assignment/write members.
    EERef &operator=( const EERef &ref ) { return *this = *ref; }
    EERef &operator=( uint8_t in )       { return eerom_write( (uint8_t*) index, in ), *this;  }
    EERef &operator +=( uint8_t in )     { return *this = **this + in; }
    EERef &operator -=( uint8_t in )     { return *this = **this - in; }
    EERef &operator *=( uint8_t in )     { return *this = **this * in; }
    EERef &operator /=( uint8_t in )     { return *this = **this / in; }
    EERef &operator ^=( uint8_t in )     { return *this = **this ^ in; }
    EERef &operator %=( uint8_t in )     { return *this = **this % in; }
    EERef &operator &=( uint8_t in )     { return *this = **this & in; }
    EERef &operator |=( uint8_t in )     { return *this = **this | in; }
    EERef &operator <<=( uint8_t in )    { return *this = **this << in; }
    EERef &operator >>=( uint8_t in )    { return *this = **this >> in; }
    
    EERef &update( uint8_t in )          { return  in != *this ? *this = in : *this; }
    
    /** Prefix increment/decrement **/
    EERef& operator++()                  { return *this += 1; }
    EERef& operator--()                  { return *this -= 1; }
    
    /** Postfix increment/decrement **/
    uint8_t operator++ (int){ 
        uint8_t ret = **this;
        return ++(*this), ret;
    }

    uint8_t operator-- (int){ 
        uint8_t ret = **this;
        return --(*this), ret;
    }
    
    int index; //Index of current EEPROM cell.
};

/***
    EEPtr class.
    
    This object is a bidirectional pointer to EEPROM cells represented by EERef objects.
    Just like a normal pointer type, this can be dereferenced and repositioned using 
    increment/decrement operators.
***/

struct EEPtr{

    EEPtr( const int index )
        : index( index )                {}
        
    operator int() const                { return index; }
    EEPtr &operator=( int in )          { return index = in, *this; }
    
    //Iterator functionality.
    bool operator!=( const EEPtr &ptr ) { return index != ptr.index; }
    EERef operator*()                   { return index; }
    
    /** Prefix & Postfix increment/decrement **/
    EEPtr& operator++()                 { return ++index, *this; }
    EEPtr& operator--()                 { return --index, *this; }
    EEPtr operator++ (int)              { return index++; }
    EEPtr operator-- (int)              { return index--; }

    int index; //Index of current EEPROM cell.
};

/***
    EEPROMClass class.
    
    This object represents the entire EEPROM space.
    It wraps the functionality of EEPtr and EERef into a basic interface.
    This class is also 100% backwards compatible with earlier Arduino core releases.
***/

struct EEPROMClass{

    EEPROMClass() {
#if EEROM_SIZE == 0
        // disable E2PROM
        ECCR = 0x80;
        ECCR = 0x00;
#elif EEROM_SIZE == 2
        // enable 2KB E2PROM
        ECCR = 0x80;
        ECCR = 0x4C | 0x01;
#elif EEROM_SIZE == 4
        // enable 4KB E2PROM
        ECCR = 0x80;
        ECCR = 0x4C | 0x02;
#elif EEROM_SIZE == 8
        // enable 8KB E2PROM
        ECCR = 0x80;
        ECCR = 0x4C | 0x03;
#else
        ECCR = 0x80;
        ECCR = 0x4C;
#endif
    };

    //Basic user access methods.
    EERef operator[]( const int idx )    { return idx; }
    uint8_t read( int idx )              { return EERef( idx ); }
    void write( int idx, uint8_t val )   { (EERef( idx )) = val; }
    void update( int idx, uint8_t val )  { EERef( idx ).update( val ); }
    int size(  ) { return length(); }

    //STL and C++11 iteration capability.
    EEPtr begin()                        { return 0x00; }
    EEPtr end()                          { return length(); } //Standards requires this to be the item after the last valid entry. The returned pointer is invalid.
    uint16_t length()                    { return (EEROM_SIZE*1024 - 720); } // bootloard bug will fix next time
    void read_block(uint8_t *, uint16_t, uint8_t);
    void write_block(uint8_t *, uint16_t, uint8_t);
    uint32_t read32(uint16_t);
    void write32(uint16_t, uint32_t);
    void write_swm(uint16_t, uint32_t *, uint8_t);
    void read_swm(uint16_t, uint32_t *, uint8_t);

    //Functionality to 'get' and 'put' objects to and from EEPROM.
    template< typename T > T &get( int idx, T &t ){
        EEPtr e = idx;
        uint8_t *ptr = (uint8_t*) &t;
        for( int count = sizeof(T) ; count ; --count, ++e )  *ptr++ = *e;
        return t;
    }

    template< typename T > const T &put( int idx, const T &t ){
        EEPtr e = idx;
        const uint8_t *ptr = (const uint8_t*) &t;
        for( int count = sizeof(T) ; count ; --count, ++e )  (*e).update( *ptr++ );
        return t;
    }
};

static EEPROMClass EEPROM;

#endif
