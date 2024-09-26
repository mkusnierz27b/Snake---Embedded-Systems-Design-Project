#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "spiAVR.h"
#include "periph.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define SNAKE_MAX_LENGTH 2048
#define INITIAL_SNAKE_LENGTH 2
#define SNAKE_SIZE 8

enum directions {up, right, down, left};

typedef struct {
    unsigned char x[SNAKE_MAX_LENGTH];
    unsigned char y[SNAKE_MAX_LENGTH];
    unsigned int length;
    directions direction;
} Snake;

typedef struct {
    unsigned char x;
    unsigned char y;
} Food;

volatile Snake snake;
volatile Food food;
volatile bool game_over = 0;

void ST7735_SetAddressWindow(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1) {
    SPI_SEND_COMMAND(CASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(x0);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(x1);

    SPI_SEND_COMMAND(RASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(y0);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(y1);

    SPI_SEND_COMMAND(RAMWR);
}

void ST7735_FillRect(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char color) {
    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    for (unsigned int i = 0; i < w * h; i++) {
        SPI_SEND_DATA(color >> 8);
        SPI_SEND_DATA(color & 0xFF);
    }
}

void ST7735_FillScreen(unsigned int color) {
    ST7735_FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color);
}

void init_game() {
    snake.length = INITIAL_SNAKE_LENGTH;
    for (unsigned char i = 0; i < snake.length; i++) {
        snake.x[i] = SCREEN_WIDTH / 2;
        snake.y[i] = SCREEN_HEIGHT / 2 + i * SNAKE_SIZE;
    }
    snake.direction = up;

    food.x = rand() % (SCREEN_WIDTH / SNAKE_SIZE) * SNAKE_SIZE;
    food.y = rand() % (SCREEN_HEIGHT / SNAKE_SIZE) * SNAKE_SIZE;
}

void move_snake() {
    for (unsigned char i = snake.length - 1; i > 0; i--) {
        snake.x[i] = snake.x[i - 1];
        snake.y[i] = snake.y[i - 1];
    }
    if (snake.direction == up) snake.y[0] -= SNAKE_SIZE;
    else if (snake.direction == right) snake.x[0] += SNAKE_SIZE;
    else if (snake.direction == down) snake.y[0] += SNAKE_SIZE;
    else if (snake.direction == left) snake.x[0] -= SNAKE_SIZE;
}

void check_collisions() {
    if (snake.x[0] >= SCREEN_WIDTH || snake.y[0] >= SCREEN_HEIGHT || snake.x[0] < 0 || snake.y[0] < 0) {
        game_over = 1;
    }
    for (unsigned char i = 1; i < snake.length; i++) {
        if (snake.x[0] == snake.x[i] && snake.y[0] == snake.y[i]) {
            game_over = 1;
        }
    }
    if (snake.x[0] == food.x && snake.y[0] == food.y) {
        snake.length++;
        food.x = rand() % (SCREEN_WIDTH / SNAKE_SIZE) * SNAKE_SIZE;
        food.y = rand() % (SCREEN_HEIGHT / SNAKE_SIZE) * SNAKE_SIZE;
    }
}

void update_display() {
    ST7735_FillScreen(0x0000);
    for (unsigned char i = 0; i < snake.length; i++) {
        ST7735_FillRect(snake.x[i], snake.y[i], SNAKE_SIZE, SNAKE_SIZE, 0xFFFF);
    }
    ST7735_FillRect(food.x, food.y, SNAKE_SIZE, SNAKE_SIZE, 0xF800);
}

ISR(TIMER0_COMPA_vect) {
    if (game_over) return;

    if ((ADC_read(0) > 600)) snake.direction = up;
    else if (ADC_read(0) < 400) snake.direction = down;
    else if (ADC_read(1) < 400) snake.direction = left;
    else if (ADC_read(1) > 600) snake.direction = right;

    move_snake();
    check_collisions();
    update_display();
}

void setup_timer() {
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS02) | (1 << CS00);
    OCR0A = 156;
    TIMSK0 = (1 << OCIE0A);
    sei();
}

int main(void) {
    SPI_INIT();
    init_game();
    setup_timer();

    while (!game_over) {
        // Main loop does nothing, everything is handled in the ISR
    }

    ST7735_FillScreen(0xF800); // Game over screen

    while (1);
}