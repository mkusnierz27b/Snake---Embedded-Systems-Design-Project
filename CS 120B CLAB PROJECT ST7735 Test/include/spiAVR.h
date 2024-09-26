/*         Name & E-mail: Matthew Kusnierz mkusn002@ucr.edu


 *         Discussion Section: 23


 *         Assignment: Custom Lab Project Final Submission


 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.


 *


 *         Demo Link: https://youtu.be/YVADGBSmJvo


 */

#ifndef SPIAVR_H
#define SPIAVR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "helper.h"
#include "serialATmega.h"


//B5 should always be SCK(spi clock) and B3 should always be MOSI. If you are using an
//SPI peripheral that sends data back to the arduino, you will need to use B4 as the MISO pin.
//The SS pin can be any digital pin on the arduino. Right before sending an 8 bit value with
//the SPI_SEND() funtion, you will need to set your SS pin to low. If you have multiple SPI
//devices, they will share the SCK, MOSI and MISO pins but should have different SS pins.
//To send a value to a specific device, set it's SS pin to low and all other SS pins to high.

// Outputs, pin definitions
#define PIN_SCK                   PORTB5//SHOULD ALWAYS BE B5 ON THE ARDUINO
#define PIN_MOSI                  PORTB3//SHOULD ALWAYS BE B3 ON THE ARDUINO
#define PIN_SS                    PORTD6
#define PIN_DC                    PORTD5
#define PIN_RESET                 PORTB0
#define SWRESET                   0x01 //software reset
#define SLPOUT                    0x11 //sleep out & booster on
#define COLMOD                    0x3A //interface pixel format
#define DISPON                    0x29 //display on
#define CASET                     0x2A //column address set
#define RASET                     0x2B //row address set
#define RAMWR                     0x2C //memory write
#define MADCTL                    0x36 //memory data access control


//If SS is on a different port, make sure to change the init to take that into account.


void SPI_HARDWARE_RESET() {
    PORTB = SetBit(PORTB,PIN_RESET,0);
    //serial_char(GetBit(PORTB,PIN_RESET)+0x30);
    _delay_ms(200);
    PORTB = SetBit(PORTB,PIN_RESET,1);
    //serial_char(GetBit(PORTB,PIN_RESET)+0x30);
    _delay_ms(200);
}

void SPI_SEND(char data)
{ 
    PORTD = SetBit(PORTD,PIN_SS,0);
    //serial_char(GetBit(PORTB,PIN_SS)+0x30);
    //_delay_ms(200);
    SPDR = data;//set data that you want to transmit
    while (!(SPSR & (1 << SPIF)));// wait until done transmitting
    PORTD = SetBit(PORTD,PIN_SS,1);
    //serial_char(GetBit(PORTB,PIN_SS)+0x30);
    //_delay_ms(200);
}

void SPI_SEND_COMMAND(char data) {
    PORTD = SetBit(PORTD,PIN_DC,0);
    //serial_char(GetBit(PORTB,PIN_DC)+0x30);
    //_delay_ms(200);
    SPI_SEND(data);
}

void SPI_SEND_DATA(char data) {
    PORTD = SetBit(PORTD,PIN_DC,1);
    //serial_char(GetBit(PORTB,PIN_DC)+0x30);
    //_delay_ms(200);
    SPI_SEND(data);
}




void SPI_INIT() {
    // Enable SPI, set as master, and set clock rate fck/16
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);
    SPI_HARDWARE_RESET();
    SPI_SEND_COMMAND(SWRESET);
    _delay_ms(150);
    SPI_SEND_COMMAND(SLPOUT);
    _delay_ms(200);
    SPI_SEND_COMMAND(COLMOD);
    SPI_SEND_DATA(0x05); //16-bit color mode
    SPI_SEND_COMMAND(DISPON);
    _delay_ms(200);
}

#endif /* SPIAVR_H */