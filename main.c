/**
  ******************************************************************************
  * @file    main.c
  * @author  Weili An
  * @version V1.0
  * @date    Oct 24, 2022
  * @brief   ECE 362 Lab 7 template
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include <stdint.h>

// Global data structure
char* login          = "xyz"; // Replace with your login.
char disp[9]         = "Hello...";
uint8_t col          = 0;
uint8_t mode         = 'A';
uint8_t thrust       = 0;
int16_t fuel         = 800;
int16_t alt          = 4500;
int16_t velo         = 0;

// Make them visible to autotest.o
extern char* login;
// Keymap is in `font.S` to match up what autotester expected
extern char keymap;
extern char disp[9];
extern uint8_t col;
extern uint8_t mode;
extern uint8_t thrust;
extern int16_t fuel;
extern int16_t alt;
extern int16_t velo;

char* keymap_arr = &keymap;

// Font array in assembly file
// as I am too lazy to convert it into C array
extern uint8_t font[];

// The functions we should implement
void enable_ports();
void setup_tim6();
void show_char(int n, char c);
void drive_column(int c);
int read_rows();
char rows_to_key(int rows);
void handle_key(char key);
void setup_tim7();
void write_display();
void update_variables();
void setup_tim14();

// Auotest functions
extern void check_wiring();
extern void autotest();
extern void fill_alpha();

int main(void) {
    // check_wiring();
    autotest();
    // fill_alpha();
    enable_ports();
    setup_tim6();
    setup_tim7();
    setup_tim14();

    for(;;) {
        asm("wfi");
    }
}

/**
 * @brief Enable the ports and configure pins as described
 *        in lab handout
 * 
 */

void enable_ports() {
    // Enable the RCC clock to GPIOB and GPIOC
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;

    // Configure pins PB0 – PB10 to be outputs
    GPIOB->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 |
                    GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 |
                    GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 |
                    GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0;

    // Configure pins PC4 – PC8 to be outputs
    GPIOC->MODER |= GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0 |
                    GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0;

    // Configure pins PC0 – PC3 to be inputs
    GPIOC->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3);

    // Configure pins PC0 – PC3 to be internally pulled low
    GPIOC->PUPDR |= GPIO_PUPDR_PUPDR0_1 | GPIO_PUPDR_PUPDR1_1 | GPIO_PUPDR_PUPDR2_1 |
                    GPIO_PUPDR_PUPDR3_1;
}


//-------------------------------
// Timer 6 ISR goes here
//-------------------------------
// TODO

extern void TIM6_DAC_IRQHandler() {
    TIM6->SR = 0x00000000;
    uint16_t c_odr = GPIOC->ODR;
    if (c_odr & 0x0100) {
        GPIOC->BRR |= 0x0100;
    }
    else {
        GPIOC->BSRR |= 0x0100;
    }
}


/**
 * @brief Set up timer 6 as described in handout
 * 
 */
void setup_tim6() {
    RCC->APB1ENR |= 0x10;

    TIM6->PSC = 48000 - 1;
    TIM6->ARR = 500 - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 0x20000;
}

/**
 * @brief Show a character `c` on column `n`
 *        of the segment LED display
 * 
 * @param n 
 * @param c 
 */
void show_char(int n, char c) {
    if ((n < 0) || (n > 7)) {
        return;
    }
    GPIOB->ODR = font[c];
    GPIOB->ODR |= n << 8;
}

/**
 * @brief Drive the column pins of the keypad
 *        First clear the keypad column output
 *        Then drive the column represented by `c`
 * 
 * @param c 
 */
void drive_column(int c) {
    GPIOC->BSRR |= 0xF00000;
    GPIOC->BSRR |= 0x0010 << (c & 0x03);

}

/**
 * @brief Read the rows value of the keypad
 * 
 * @return int 
 */
int read_rows() {
    return (GPIOC->IDR & 0x000F);

}

/**
 * @brief Convert the pressed key to character
 *        Use the rows value and the current `col`
 *        being scanning to compute an offset into
 *        the character map array
 * 
 * @param rows 
 * @return char 
 */
char rows_to_key(int rows) {
    int idx;
    int tempcol;
    tempcol = col & 3;
    if(rows & 1)
      {
        idx = 0;
      }
    else if(rows & 2)
      {
        idx = 1;
      }
    else if(rows & 4)
      {
        idx = 2;
      }
    else if(rows & 8)
      {
        idx = 3;
      }
    int x;
    x = (4*tempcol) + idx;
   return (keymap_arr[x]);
}

/**
 * @brief Handle key pressed in the game
 * 
 * @param key 
 */
void handle_key(char key) {
    /*
     *     if key == 'A'/'B'/'D', set mode to key
     *   else if key is a digit, set thrust to the represented
     *   value of key, i.e. if key == '1', thrust = 1, not '1'
     */
    if ((key == 'A') || (key == 'B') || (key == 'D')) {
        mode = key;
    }
    else if ((key >= '0') && (key <= '9')) {
        thrust = key - '0';
    }
}

//-------------------------------
// Timer 7 ISR goes here
//-------------------------------
// TODO

extern void TIM7_IRQHandler() {
    TIM7->SR &= ~TIM_SR_UIF;
    int rows = read_rows();
    if (rows != 0) {
        char key = rows_to_key(rows);
        handle_key(key);
    }
    char display = disp[col];
    show_char(col, display);
    col++;
    if (col > 7) {
        col = 0;
    }
    drive_column(col);
}

/**
 * @brief Setup timer 7 as described in lab handout
 * 
 */
void setup_tim7() {
    RCC->APB1ENR |= 0x20;
    TIM7->PSC = 4800 - 1;
    TIM7->ARR = 10 - 1;
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 0x40000;
}

/**
 * @brief Write the display based on game's mode
 * 
 */
void write_display() {
    switch(mode) {
        case 'C':
            snprintf(disp, 9, "%s", "Crashed");
            break;
        case 'L':
            snprintf(disp, 9, "%s", "Landed ");
            break;
        case 'A':
            snprintf(disp, 9, "ALt%5d", alt);
            break;
        case 'B':
            snprintf(disp, 9, "FUEL %3d", fuel);
            break;
        case 'D':
            snprintf(disp, 9, "Spd %4d", velo);
            break;
    }
}

/**
 * @brief Game logic
 * 
 */
void update_variables() {
    fuel -= thrust;
    if (fuel <= 0) {
        thrust = 0;
        fuel = 0;
    }

    alt += velo;
    if (alt <= 0) {
        if ((0 - velo) < 10) {
            mode = 'L';
        }
        else {
            mode = 'C';
        }
        return;
    }
    velo += (thrust - 5);
}

//-------------------------------
// Timer 14 ISR goes here
//-------------------------------
// TODO
extern void TIM14_IRQHandler() {
    TIM14->SR &= ~TIM_SR_UIF;
    update_variables();
    write_display();
}

/**
 * @brief Setup timer 14 as described in lab
 *        handout
 * 
 */
void setup_tim14() {
    RCC->APB1ENR |= 0x100;
    TIM14->PSC = 24000 - 1;
    TIM14->ARR = 1000 - 1;
    TIM14->DIER |= TIM_DIER_UIE;
    TIM14->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 0x80000;
}
