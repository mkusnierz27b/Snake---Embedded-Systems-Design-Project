/*         Name & E-mail: Matthew Kusnierz mkusn002@ucr.edu


 *         Discussion Section: 23


 *         Assignment: Custom Lab Project Final Submission


 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.


 *


 *         Demo Link: https://youtu.be/YVADGBSmJvo


 */

#ifndef GAME_H
#define GAME_H

#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include "spiAVR.h"
#include <stdlib.h>

enum directions {up, down, left, right};

class Game {
    private:
        struct Snake {
            unsigned int length;
            int xSeg[257]; //These arrays store the segments of the body. 256 is the max number of segments
            int ySeg[257];
            directions direction;
        } snake;

        struct Apple {
            unsigned char xPos;
            unsigned char yPos;
        } apple;

        bool dead = false;

        //font values are backwards and mirrored because LCD is upside down
        const unsigned char font[21*5] = {
            0x00, 0x00, 0x00, 0x00, 0x00, // 0x20 ' '
            0x00, 0x00, 0x7D, 0x00, 0x00, // 0x21 '!'
            0x3E, 0x51, 0x49, 0x45, 0x3E, // 0x30 '0'
            0x00, 0x01, 0x7F, 0x21, 0x00, // 0x31 '1'
            0x31, 0x49, 0x49, 0x49, 0x27, // 0x32 '2'
            0x46, 0x69, 0x51, 0x41, 0x42, // 0x33 '3'
            0x04, 0x7F, 0x24, 0x14, 0x0C, // 0x34 '4'
            0x4E, 0x51, 0x51, 0x51, 0x72, // 0x35 '5'
            0x03, 0x49, 0x49, 0x29, 0x1E, // 0x36 '6'
            0x60, 0x50, 0x48, 0x47, 0x40, // 0x37 '7'
            0x33, 0x49, 0x49, 0x49, 0x33, // 0x38 '8'
            0x3C, 0x4A, 0x49, 0x49, 0x30, // 0x39 '9'
            0x00, 0x00, 0x33, 0x33, 0x00, // 0x3A ':'
            0x22, 0x41, 0x41, 0x41, 0x3E, // 0x43 'C'
            0x41, 0x49, 0x49, 0x49, 0x7F, // 0x45 'E'
            0x01, 0x01, 0x01, 0x01, 0x7F, // 0x4C 'L'
            0x3E, 0x41, 0x41, 0x41, 0x3E, // 0x4F 'O'
            0x31, 0x4A, 0x4C, 0x48, 0x7F, // 0x52 'R'
            0x43, 0x49, 0x49, 0x49, 0x31, // 0x53 'S'
            0x7E, 0x01, 0x01, 0x01, 0x7E, // 0x55 'U'
            0x70, 0x08, 0x07, 0x08, 0x70, // 0x59 'Y'
        };



    public:
        void move();
        void eat();
        void draw();
        bool die();
        unsigned char getScore();
        void reset();
        void draw_char(unsigned char x, unsigned char y, char c);
        void draw_string(unsigned char x, unsigned char y, const char* str);
};


void Game::draw_char(unsigned char x, unsigned char y, char character) {
    const unsigned char* segment;

    switch(character) {
        case ' ':
            segment = &font[0];
            break;

        case '!':
            segment = &font[5];
            break;

        case '0':
            segment = &font[10];
            break;

        case '1':
            segment = &font[15];
            break;

        case '2':
            segment = &font[20];
            break;

        case '3':
            segment = &font[25];
            break;

        case '4':
            segment = &font[30];
            break;

        case '5':
            segment = &font[35];
            break;

        case '6':
            segment = &font[40];
            break;

        case '7':
            segment = &font[45];
            break;

        case '8':
            segment = &font[50];
            break;

        case '9':
            segment = &font[55];
            break;

        case ':':
            segment = &font[60];
            break;

        case 'C':
            segment = &font[65];
            break;

        case 'E':
            segment = &font[70];
            break;

        case 'L':
            segment = &font[75];
            break;

        case 'O':
            segment = &font[80];
            break;

        case 'R':
            segment = &font[85];
            break;

        case 'S':
            segment = &font[90];
            break;

        case 'U':
            segment = &font[95];
            break;

        case 'Y':
            segment = &font[100];
            break;

        default:
            break;

    }

    for(unsigned char i = 0; i < 5; i++) {//write each character one column at a time (5 total)
        unsigned char bit = segment[i]; 
        for(unsigned char j = 0; j < 8; j++) { //write each column one "bit" at a time. 0's are black space
            if(bit & 1) {
                SPI_SEND_COMMAND(CASET);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(x+i);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(x+i);

                SPI_SEND_COMMAND(RASET);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(y+j);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(y+j);

                SPI_SEND_COMMAND(RAMWR);
                SPI_SEND_DATA(0xFF);
                SPI_SEND_DATA(0xFF);
            }
            bit = bit >> 1; //shift to the next bit in the column
        }
    }
}

void Game::draw_string(unsigned char x, unsigned char y, const char* str) {
    while (*str) { //iterate through each character
        draw_char(x, y, *str);
        x += 6; // move to next character (5 columns)
        str++;
    }

}

//This function moves the snake
void Game::move() {
    //set direction
    directions tempDir;
    if(ADC_read(0) > 600) tempDir = up;
    else if(ADC_read(0) < 400) tempDir = down;
    else if(ADC_read(1) > 600) tempDir = right;
    else if(ADC_read(1) < 400) tempDir = left; 
    else tempDir = snake.direction;

    //move non-head segments
    for(unsigned char i = snake.length-1; i > 0; i--) {
        snake.xSeg[i] = snake.xSeg[i-1];
        snake.ySeg[i] = snake.ySeg[i-1];
    }

    //move head
    switch(tempDir) {
        case up:
            if(snake.direction != down) {
                snake.direction = up;
            }
            break;

        case down:
            if(snake.direction != up) {
                snake.direction = down;
            }
            break;

        case left:
            if(snake.direction != right) {
                snake.direction = left;
            }
            break;

        case right:
            if(snake.direction != left) {
                snake.direction = right;
            }
            break;

        default:
            break;
    }

    switch(snake.direction) {
        case up:
            snake.ySeg[0]+= 8;
            break;

        case down:
            snake.ySeg[0]-= 8;
            break;

        case left:
            snake.xSeg[0]+= 8;
            break;

        case right:
            snake.xSeg[0]-= 8;
            break;

        default:
            break;
    }
    
    //snake runs into itself and dies
    for(unsigned char i = 1; i < snake.length; i++) {
        if(snake.xSeg[0] == snake.xSeg[i] && snake.ySeg[0] == snake.ySeg[i]) dead = true;
    }


}

//If the snake head reaches an apple, it will grow another segment and spawn another apple
void Game::eat() {
    if(apple.xPos == snake.xSeg[0] && apple.yPos == snake.ySeg[0]) {
        apple.xPos = 128 * (rand() % 16)/16;
        apple.yPos = 128 * (rand() % 16)/16;

        //set new tail
        snake.xSeg[snake.length] = snake.xSeg[snake.length-1] + (snake.xSeg[snake.length-1] - snake.xSeg[snake.length-2]);
        snake.ySeg[snake.length] = snake.ySeg[snake.length-1] + (snake.ySeg[snake.length-1] - snake.ySeg[snake.length-2]);
        snake.length++;

        if(snake.length == 256) return;

        //prevents apple from spawning in an occupied space
        for(char i = 0; i < snake.length; i++) {
            if(apple.xPos == snake.xSeg[i] && apple.yPos == snake.ySeg[i]) {
                apple.xPos = 128 * (rand() % 16)/16;
                apple.yPos = 128 * (rand() % 16)/16;
                i = -1;
            }
        }

    }
}

void Game::draw() {
    //move head
    SPI_SEND_COMMAND(CASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.xSeg[0]);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.xSeg[0]+7);

    SPI_SEND_COMMAND(RASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.ySeg[0]);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.ySeg[0]+7);

    SPI_SEND_COMMAND(RAMWR);
    for(unsigned char i = 0; i < 64; i++) {
        SPI_SEND_DATA(0xF8);
        SPI_SEND_DATA(0x00);
    }

    //move tail
    SPI_SEND_COMMAND(CASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.xSeg[snake.length-1]);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.xSeg[snake.length-1]+7);

    SPI_SEND_COMMAND(RASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.ySeg[snake.length-1]);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(snake.ySeg[snake.length-1]+7);

    SPI_SEND_COMMAND(RAMWR);
    for(unsigned char i = 0; i < 64; i++) {
        SPI_SEND_DATA(0x00);
        SPI_SEND_DATA(0x00);
    }

    //draw apple
    if(snake.length != 256) {
        SPI_SEND_COMMAND(CASET);
        SPI_SEND_DATA(0x00);
        SPI_SEND_DATA(apple.xPos);
        SPI_SEND_DATA(0x00);
        SPI_SEND_DATA(apple.xPos+7);

        SPI_SEND_COMMAND(RASET);
        SPI_SEND_DATA(0x00);
        SPI_SEND_DATA(apple.yPos);
        SPI_SEND_DATA(0x00);
        SPI_SEND_DATA(apple.yPos+7);

        SPI_SEND_COMMAND(RAMWR);
        for(unsigned char i = 0; i < 64; i++) {
            SPI_SEND_DATA(0x00);
            SPI_SEND_DATA(0x1F);
        }
    }
}

//sets a flag that will end the game if the snake hits a wall or itself
bool Game::die() {
    return snake.xSeg[0] < 0 || snake.xSeg[0] > 127 || snake.ySeg[0] < 0 || snake.ySeg[0] > 127 || dead;
}

unsigned char Game::getScore() {
    return snake.length-3;
}


//Resets variables after loss
void Game::reset() {
    SPI_SEND_COMMAND(CASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(127);

    SPI_SEND_COMMAND(RASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(127);

    SPI_SEND_COMMAND(RAMWR);
    for(unsigned int i = 0; i < 128*128; i++) {
        SPI_SEND_DATA(0x00);
        SPI_SEND_DATA(0x00);
    }

    snake.length = 3; //game does not create the tail for some reason
    snake.xSeg[0] = 64;
    snake.ySeg[0] = 64;
    snake.xSeg[1] = 56;
    snake.ySeg[1] = 64;
    snake.direction = left;

    apple.xPos = 128 * (rand() % 16)/16;
    apple.yPos = 128 * (rand() % 16)/16;


    for(char i = 0; i < snake.length; i++) {
        if(apple.xPos == snake.xSeg[i] && apple.yPos == snake.ySeg[i]) {
            apple.xPos = 128 * (rand() % 16)/16;
            apple.yPos = 128 * (rand() % 16)/16;
            i = -1;
        }
    }

    draw();
}

#endif