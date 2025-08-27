//pins used - PC0-11, PB0,1,4, PA4
//for keypad im thinking - PB 2,3,5,6,7 PC13,14,15

#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fifo.h"
#include "tty.h"

#define NUMROWS 32
#define NUMCOLS 32
#define RATE 8000
#define N_GAME_WIN 15952
#define N_GAME_OVER 10658
#define N_WHACK 8263
#define N_RICKROLL 19966

//FUNCTION DECLARATIONS (and descriptions)

//sets up GPIOC_MODER configuration to general output for PC0-12 connected to Adafruit LED Matrix
void matrixGPIO();

//enables and disables latch to tell the matrix that the input for a given row is complete
void latch_matrix();

// enables output to be displayed on LED matrix
void enable_output(); 

// disables output and turns off LED matrix
void disable_output(); 

// sends a single clock pulse by setting clock to high and low (used to shift color bits into columns in a row)
void matrix_clock(); 

// sets the color of the LED (selects whether to use upper or lower half of LED matrix based on row number)
void setColor(int curr_row, int r, int g, int b); 

//drives row output to high (so we can start shifting bits into the column)
void driveRow(int curr_row); 

//clears the entire LED Matrix
void clearMatrix(); 

// Green lights successively in every row
void winDisplay(); 

// displays mole at inputted coordinates on the LED matrix
void drawMole(int curr_row, int curr_col, int r, int g, int b); 

// show score on 7 segment display
void set_col(int col);

void init_keypad();

void init_systick();

void setupMoleTimer();

void endGameTimer();

void internal_clock();


// GLOBAL VARIABLES
volatile int row = 0; //current row of mole
volatile int col = 0; // current column of mole
volatile int level = -1;
volatile int score = 0; //player score
volatile int start = 0;
// volatile int end = 0;
volatile int current_key_col = 0;
volatile int current_key_row = -1;
char USERNAME[8];
int N;
extern unsigned char game_over_raw[];
extern unsigned char rickroll_effect_raw[];
extern unsigned char whack_raw[];
extern unsigned char game_win_raw[];
unsigned char* wavetable;
int step0 = 1;
int offset0 = 0;
int whack; //indicate if you want a whack sound
int game; //indicate if you want a game over/game win sound
char LEVEL[2]; 
int highscores[3];

// MATRIX SETUP 
void matrixGPIO(){
    // LED matrix pin configuration
    // PC0 - R1 - general output
    // PC1 - B1 - general output
    // PC2 - R2 - general output
    // PC3 - B2 - general output
    // PC4 - A - general output
    // PC5 - C - general output
    // PC6 - Clk - general output
    // PC7 - OE - general output
    // PC8 - G1 - general output
    // PC9 - G2 - general output
    // PC10 - B - general output
    // PC11- D - general output
    // PB0 - Lat - general output

    RCC->AHBENR |= RCC_AHBENR_GPIOCEN; //enable clock for GPIOC
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; //enable clock for GPIOB

    GPIOC->MODER &= ~0x3ffffff; //clear required bits
    GPIOC->MODER |= 0x555555; //set to output

    GPIOB->MODER &= ~0x1;
    GPIOB->MODER |= 0x1;
}

void setupMoleTimer(){
    RCC -> APB1ENR |= RCC_APB1ENR_TIM3EN; // turning on clock TIM3
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN; // turning on clock for port B

    GPIOB -> MODER |= 0x200; // setting PB4 to alternate function mode
    
    GPIOB -> AFR[0] |= 0x10000; //set to AF1

    TIM3->PSC = 48000 - 1;
    TIM3->ARR = 1000 - 1;

    // TIM3->DIER |= TIM_DIER_TIE;
    // TIM3->DIER |= TIM_DIER_CC1IE;
    TIM3->DIER |= TIM_DIER_UIE;
    
    TIM3->CCR1 = 900;

    NVIC->ISER[0] |= (1 << TIM3_IRQn);

    TIM3->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1);  // PWM mode 1 for channel 1
    TIM3->CCER |= TIM_CCER_CC1E; // Enable output on TIM3_CH1

    // TIM3 -> CR1 |= TIM_CR1_CEN; //enable timer 3
}

void endGameTimer(void){
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN; // turning on clock for port B
    GPIOB->MODER |= 0x8; ///PB1
    GPIOB -> AFR[0] &= ~0xf0; //set to AF0

    RCC -> APB1ENR |= RCC_APB1ENR_TIM14EN; // turning on clock TIM14
    
    TIM14->PSC = 48000 - 1;
    TIM14->ARR = 60000 - 1; // 60 second timer

    NVIC->ISER[0] |= (1 << TIM14_IRQn);

    TIM14->DIER |= TIM_DIER_UIE;

    // TIM14->CR1 |= TIM_CR1_CEN;
}

void TIM14_IRQHandler(void){
    TIM14->SR &= ~TIM_SR_UIF;
    // RCC -> APB1ENR &= ~RCC_APB1ENR_TIM14EN; // turning off clock TIM14
    // RCC -> APB1ENR &= ~RCC_APB1ENR_TIM3EN; // turning off clock TIM3
    if (start == 2){
        TIM3->CR1 &= ~TIM_CR1_CEN;
        TIM14->CR1 &= ~TIM_CR1_CEN;
        TIM7->CR1 &= ~TIM_CR1_CEN;
        spi1_display1("            ");
        display_highscore(score);
        please(intToString(highscores[0]), 0);
        please(intToString(highscores[1]), 20);
        please(intToString(highscores[2]), 40);
        lcdhighscore();
        game_over();

        int count = 0;
        while(count < 1000){
            winDisplay();
            count++;
        }
        clearMatrix();
        // end = 1;
        // NVIC_DisableIRQ(SysTick_IRQn);
        // NVIC->ISER[0] &= ~(1 << TIM3_IRQn);
        // TIM3->DIER &= ~TIM_DIER_CC1IE;
        // TIM14->DIER &= ~TIM_DIER_UIE;
    }
    // start = 1;
    start++;
}

void TIM3_IRQHandler(void) {
    //acknowledge interrupt
    // TIM3->SR &= ~TIM_SR_CC1IF;
    TIM3->SR &= ~TIM_SR_UIF;

    int count = 0;
    int r = rand()%2;
    int g = rand()%2;
    int b = rand()%2;
    while(count <= 400*(4-level)){
        drawMole(row, col, r, g, b);
        count++;
    }
    // nano_wait(1000000000);
    row = rand()%4;
    col = rand()%4;
    // row = 0;
    // col = 0;
    clearMatrix();
}

void adjust_priorities() {
//   NVIC_SetPriority(TIM6_DAC_IRQn, 4); // sound
  NVIC_SetPriority(TIM14_IRQn, 1); // game timer (60 seconds)
  NVIC_SetPriority(TIM7_IRQn, 2); // whacking 
  NVIC_SetPriority(TIM3_IRQn, 3); // mole
}

// BITBANGING LOGIC
void latch_matrix(){
    GPIOB->BSRR |= 0x1; // latch to 1
    GPIOB->BRR |= 0x1; // latch to 0
}

void enable_output(){
    GPIOC->BSRR |= 0x80; //enables led matrix output
}

void disable_output(){
    GPIOC->BRR |= 0x80; //disables led matrix output
}

void matrix_clock(){
    //simulates one pulse of clock to shift data into row
    GPIOC->BSRR |= 0x40; // clk to 1
    GPIOC->BRR |= 0x40; // clk to 0
}

void setColor(int curr_row, int r, int g, int b){
    if (curr_row <= 15){ //set values for R1, G1, B1
        if (r){
            GPIOC->BSRR |= 0x1;
        }
        else{
            GPIOC->BRR |= 0x1;
        }

        if (g){
            GPIOC->BSRR |= 0x100;
        }
        else{
            GPIOC->BRR |= 0x100;
        }

        if (b){
            GPIOC->BSRR |= 0x2;
        }
        else{
            GPIOC->BRR |= 0x2;
        }
    }
    else{ //set values for R2, G2, B2
        if (r){
            GPIOC->BSRR |= 0x4;
        }
        else{
            GPIOC->BRR |= 0x4;
        }

        if (g){
            GPIOC->BSRR |= 0x200;
        }
        else{
            GPIOC->BRR |= 0x200;
        }

        if (b){
            GPIOC->BSRR |= 0x8;
        }
        else{
            GPIOC->BRR |= 0x8;
        }
    }
}

void driveRow(int curr_row){
    int A = (curr_row & 0x1); //PC4
    int C = (curr_row & 0x4) >> 2; //PC5
    int B = (curr_row & 0x2) >> 1; //PC10
    int D = (curr_row & 0x8) >> 3; //PC11

    if (A){
        GPIOC->BSRR |= (1 << 4);
    }
    else{
        GPIOC->BRR |= (1 << 4);
    }
    if (C){
        GPIOC->BSRR |= (1 << 5);
    }
    else{
        GPIOC->BRR |= (1 << 5);
    }
    if (B){
        GPIOC->BSRR |= (1 << 10);
    }
    else{
        GPIOC->BRR |= (1 << 10);
    }
    if (D){
        GPIOC->BSRR |= (1 << 11);
    }
    else{
        GPIOC->BRR |= (1 << 11);
    }

}

void clearMatrix(){
    for (int j = 0; j < NUMROWS; j++){
        disable_output();
        setColor(j, 0, 0, 0);
        driveRow(j); //enable row number
        for (int i = 0; i < NUMCOLS; i++){
            matrix_clock();
        }
        latch_matrix();
    }
    enable_output();
}

void winDisplay(){
    clearMatrix();
    for (int j = 0; j < NUMROWS; j++){
        disable_output();
        // if (j > 0){
        //     clearRow(j-1);
        // }
        // clearMatrix();
        driveRow(j); //enable row number
        setColor(j, 0, 1, 0); //set to green
        for (int i = 0; i < NUMCOLS; i++){
            matrix_clock();
        }
        latch_matrix();
        enable_output();
    }
    // nano_wait(1000000000);
}

void drawMole(int curr_row, int curr_col, int r, int g, int b){
    for (int j = 8*curr_row; j <  8*curr_row + 8; j++){
        disable_output();
        driveRow(j); //enable row number
        for (int i = 0; i < NUMCOLS; i++){
            if (i >= curr_col*8 && i<(curr_col*8+8)){
                setColor(j, r, g, b); //set to red
            }
            else{
                setColor(j, 0, 0 ,0);
            }
            matrix_clock();
        }
        latch_matrix();
        enable_output();
        // nano_wait(100);
    }
}

void init_keypad() {
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // ENABLE PORT B
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // ENABLE PORT A

  // Columns - PB7 PB6 PB5 PB3 , Rows - PB2, PA12, PA6, PA8
  GPIOB->MODER |= 0x5440; // col pins as output
  GPIOB->MODER &= ~0x30; // PB2 (row) as input
  GPIOA->MODER &= ~0x3033000; // PA12, PA6, PA8 as input
  GPIOB->OTYPER |= 0xe8; 

  GPIOB->PUPDR |= 0xa880; // pull down resistors for cols
}


uint8_t hist[16];
char queue[2];  // A two-entry queue of button press/release events.
int qin;        // Which queue entry is next for input
int qout;       // Which queue entry is next for output

const char keymap[] = "DCBA#9630852*741";

void push_queue(int n) {
    queue[qin] = n;
    qin ^= 1;
}

char pop_queue() {
    char tmp = queue[qout];
    queue[qout] = 0;
    qout ^= 1;
    return tmp;
}


void update_history(int c, int rows)
{
    for(int i = 0; i < 4; i++) {
        hist[4*c+i] = (hist[4*c+i]<<1) + ((rows>>i)&1);
        if (hist[4*c+i] == 0x01)
        {
            push_queue(0x80 | keymap[4*c+i]);
            
            if (start == 0)
            {
                // please("0", 0);
                // please("0", 20);
                // please("0", 40);
                char event = 'X';
                LEVEL[0] = event;  // Store the key press in the buffer
                LEVEL[1] = '\0';
                spi1_display1("Enter level: ");
                
                if (rows == 1)
                {
                    LEVEL[0] = 'A';
                    level = 0;
                    // start = 1;
                }
                else if (rows == 2)
                {
                    LEVEL[0] = 'B';
                    level = 1;
                    // start = 1;
                }
                else if (rows == 4)
                {
                    LEVEL[0] = 'C';
                    level = 2;
                    // start = 1;
                }
                else if (rows == 8)
                {
                    LEVEL[0] = 'D';
                    level = 3;
                    // start = 1;
                }
                else
                {
                    char error[] = "invalid";
                    // start = 0;
                }
                spi1_display2(LEVEL);

                if (LEVEL[0]!='X'){
                    start = 1;
                    nano_wait(10000000000);
                    TIM3 -> CR1 |= TIM_CR1_CEN; //enable timer 3 to have moles show up
                    TIM14->CR1 |= TIM_CR1_CEN; // start countdown
                }
                // start = 1;
            }
            else if (start && current_key_row == row && current_key_col == col)
            {
                score++;
                printf("%d, %d %d %d %d\n", score, row, col, current_key_row, current_key_col);
                spi1_display1("Your score: ");
                spi1_display2(intToString(score));
                whack_it();
            }
        }
        if (hist[4*c+i] == 0xfe){
            push_queue(keymap[4*c+i]);
        }
    }
}

void init_tim7(void) {
    RCC -> APB1ENR |= RCC_APB1ENR_TIM7EN; // turning on clock TIM2

    TIM7 -> PSC = 480 - 1;
    TIM7 -> ARR = 500 - 1;

    // TIM7 -> PSC = 4800 - 1;
    // TIM7 -> ARR = 10 - 1;

    TIM7 -> DIER |= TIM_DIER_UIE; //enable UIE flag in DIER
  
    NVIC -> ISER[0] |= (1 << TIM7_IRQn); //enable interrupt

    TIM7->CR1 |= TIM_CR1_CEN; //enable timer 7
}

void TIM7_IRQHandler() {
    TIM7 -> SR &=  ~TIM_SR_UIF; //acknowledge
    // shift by the amount of 0 to move it to lsb - so it actually outputs a 1 or 0
     //Rows - PB2, PA12, PA6, PA8

    int32_t row0 = ((GPIOB -> IDR) & 0x4) >> 2; //pb2
    int32_t row1 = ((GPIOA -> IDR) & 0x1000) >> 12; //pa12
    int32_t row2 = ((GPIOA -> IDR) & 0x40) >> 6; //pa6
    int32_t row3 = ((GPIOA -> IDR) & 0x100) >> 8; //pa8

    // int rows = read_rows();
    int rows = (row0) | (row1 << 1) | (row2 << 2) | (row3 << 3);
    
    current_key_row = row0 ? 0 : row1 ? 1 : row2 ? 2 : row3 ? 3 : -1;
    update_history(current_key_col, rows);
    
    // printf("%d, %d %d %d %d\n", score, row, col, current_key_row, current_key_col);

    current_key_col += 1;
    if (current_key_col > 3){
      current_key_col = 0;
    }
    set_col(current_key_col);
}

void set_col(int col) {
    // Columns - PB7 PB6 PB5 PB3
    GPIOB->BRR |= 0xE8;

    if (col == 0)
    {
        GPIOB->BSRR |= 0x0080; //PB7
    }

    if (col == 1)
    {
        GPIOB->BSRR |= 0x0040; //PB6
    }

    if (col == 2)
    {
        GPIOB->BSRR |= 0x0020; //PB5
    }

    if (col == 3)
    {
        GPIOB->BSRR |= 0x8; //PB3
    }
}

void init_usart5() {
    // TODO
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN; //enable clock for Port C

    GPIOC->MODER &= ~GPIO_MODER_MODER12_0; //clear relevant bits PC12
    GPIOC->MODER |= GPIO_MODER_MODER12_1; //set to alt function mode
    
    GPIOC->AFR[1] &= ~GPIO_AFRH_AFRH4; //clear AFR bits for PC12
    GPIOC->AFR[1] |= 2 << GPIO_AFRH_AFRH4_Pos;//set PC12 to AF2
  
    RCC->AHBENR |= RCC_AHBENR_GPIODEN; //enable clock for Port D

    GPIOD->MODER &= ~GPIO_MODER_MODER2_0; //clear relevant bits PD2
    GPIOD->MODER |= GPIO_MODER_MODER2_1; //set to alt function mode

    GPIOD->AFR[0] &= ~GPIO_AFRL_AFRL2; //clear AFR bits for PD2
    GPIOD->AFR[0] |= 2 << 8;//set PD2 to AF2

    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;

    USART5->CR1 &= ~USART_CR1_UE; //turn off
    USART5->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0); //set word size to 8 bits;
    USART5->CR2 &= ~(USART_CR2_STOP); // set to 1 stop bit
    USART5->CR1 &= ~USART_CR1_PCE; // no parity control
    USART5->CR1 &= ~USART_CR1_OVER8; //set to oversampling by 16
    // USART5->BRR = SystemCoreClock/115200; //set baud rate
    USART5 -> BRR = 0x1A1;
    USART5->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    USART5->CR1 |= USART_CR1_UE; //turn on

    while(!(USART5->ISR & (USART_ISR_TEACK | USART_ISR_REACK)));//wait for TE and RE to be acknowledged 
}

#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;

void enable_tty_interrupt(void) {
    // TODO
    // NVIC->ISER |= NVIC_EnableIRQ;
    // NVIC->ISER |= USART5;
    USART5->CR1 |= USART_CR1_RXNEIE;
    NVIC -> ISER[0] |= (1 << USART3_8_IRQn); //enable interrupt
    USART5->CR3 |= USART_CR3_DMAR;

    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->CSELR |= DMA2_CSELR_CH2_USART5_RX;
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;  // First make sure DMA is turned off
    DMA2_Channel2->CMAR = &serfifo;
    DMA2_Channel2->CPAR = &(USART5->RDR);
    DMA2_Channel2->CNDTR = FIFOSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR;
    DMA2_Channel2->CCR &= ~(DMA_CCR_HTIE | DMA_CCR_TCIE);
    DMA2_Channel2->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE);
    DMA2_Channel2->CCR |= (DMA_CCR_MINC | DMA_CCR_CIRC);
    DMA2_Channel2->CCR &= ~(DMA_CCR_PINC | DMA_CCR_MEM2MEM);
    DMA2_Channel2->CCR |= DMA_CCR_EN;
}

// Works like line_buffer_getchar(), but does not check or clear ORE nor wait on new characters in USART
char interrupt_getchar() {
    // TODO
    USART_TypeDef *u = USART5;
    
    // Wait for a newline to complete the buffer.
    while(fifo_newline(&input_fifo) == 0) {
        asm volatile ("wfi"); // wait for an interrupt
    }
    // Return a character from the line buffer.
    char ch = fifo_remove(&input_fifo);
    return ch;
}

int __io_putchar(int c) {
    // TODO copy from STEP2
    if (c == '\n'){
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = '\r';
    }
    while(!(USART5->ISR & USART_ISR_TXE));
    USART5->TDR = c;
    
    return c;
}

int __io_getchar(void) {
    // TODO Use interrupt_getchar() instead of line_buffer_getchar()
    return interrupt_getchar();
}

// TODO Copy the content for the USART5 ISR here
// TODO Remember to look up for the proper name of the ISR function
void USART3_8_IRQHandler(void) {
    while(DMA2_Channel2->CNDTR != sizeof serfifo - seroffset) {
        if (!fifo_full(&input_fifo))
            insert_echo_char(serfifo[seroffset]);
        seroffset = (seroffset + 1) % sizeof serfifo;
    }
}

void setup_dac(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // ENABLE CLOCK TO PORT A
    GPIOA->MODER |= 3 << (2*4); // SET PA4 TO ANALOG
    RCC->APB1ENR |= RCC_APB1ENR_DACEN; // ENABLE CLOCK TO ADC
    DAC->CR &= ~DAC_CR_TSEL1; // TIM6 AS TRIGGER 000
    DAC->CR |= DAC_CR_TEN1; //ENABLE TRIGGER
    DAC->CR |= DAC_CR_EN1; // ENABLE DAC CHANNEL 1
}

void TIM6_DAC_IRQHandler() {
    TIM6->SR &= ~TIM_SR_UIF; // ACK INTERRUPT
    offset0 += step0;

     if (whack)
    {
        if (offset0 <= N)
        {
            DAC->DHR8R1 = (wavetable[offset0]) ^ 128;
            // whack = 0;
            whack++;
        }
    }
    // else if (end){
    //     game_over(); // play game over sound effect
    // }
    else if (game)
    {
        if (offset0 >= N) 
        {
            offset0 = 0;
        }
        DAC->DHR8R1 = (wavetable[offset0]) ^ 128;
    }
}

void init_tim6(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 480 - 1;
    TIM6->ARR = (100000 / RATE) - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR2 |= 0X0020; // MMS 010 UPDATE
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

void whack_it(){
    whack = 1;
    game = 0;
    N = N_WHACK;
    wavetable = whack_raw;
    offset0 = 0;
}

void game_over(){
    whack = 0;
    game = 1;
    N = N_GAME_OVER;
    wavetable = game_over_raw;
}


// char get_key_event(void) {
//     for(;;) {
//         asm volatile ("wfi");   // wait for an interrupt
//        if (queue[qout] != 0)
//             break;
//     }
//     return pop_queue();
// }

// char get_keypress() {
//     char event;
//     for(;;) {
//         // Wait for every button event...
//         event = get_key_event();
//         // ...but ignore if it's a release.
//         if (event & 0x80)
//             break;
//     }
//     return event & 0x7f;
// }

void show_keys(void)
{
    int index = 0;  // This keeps track of how many characters we have stored

    while (index < 1) {  // Loop will exit when 8 characters are stored
        char event = get_keypress();  // Get the key press event (ignores key releases)
        USERNAME[index] = event;  // Store the key press in the buffer
        index++; 
 //       USERNAME[index] = '\0';
        // print(USERNAME);  // Output the current buffer (string of key presses)
    }
    USERNAME[index] = '\0';
    spi1_display1(USERNAME);

}

// char get_keypress(){
//     int32_t row0 = (GPIOB -> IDR) & 0x4; //pb2
//     int32_t row1 = (GPIOA -> IDR) & 0x1000; //pa12
//     int32_t row2 = (GPIOA -> IDR) & 0x40; // pa6
//     int32_t row3 = (GPIOA -> IDR) & 0x100; //pa8
//     printf("%d, %d, %d, %d\n", row0, row1, row2, row3);

//     if(row0){
//         return 'A';
//     }    
//     else if(row1){
//         return 'B';
//     }
//     else if(row2){
//         return 'C';
//     }
//     else if(row3){
//         return 'D';
//     }
//     else{
//         return 'Z';
//     }
// }

// void showlevel(void)
// {
//     char event = 'Z';
//     set_col(3);
//     while(event == 'Z'){
//         event = get_keypress();  // Get the key press event (ignores key releases)
//         printf("hello\n");
//     }

//     LEVEL[0] = event;  // Store the key press in the buffer
//     LEVEL[1] = '\0';
//     printf("%c", event);

//     spi1_display1("enter level: ");
//     spi1_display2(LEVEL);

//     if(LEVEL[0] == 'A'){
//         level = 0;
//     }
//     else if(LEVEL[0] == 'B'){
//         level = 1;
//     }
//     else if(LEVEL[0] == 'C'){
//         level = 2;
//     }
//     else if(LEVEL[0] == 'D'){
//         level = 3;
//         printf("yes");
//     }
//     else{
//         char error[] = "invalid";
//     }
// }

void startup(){
    while(!start){
        spi1_display1("Username: ");

        int count = 10000;
        int i = 0;
        while(i < count){
            spi1_display2(USERNAME);
            i++;
            nano_wait(1000000);
        }

        nano_wait(1000000);

        count = 100;
        i = 0;
        while(i < count){
            spi1_display1("              ");
            spi1_display2("              ");
            i++;
            nano_wait(1000000);
        }

        i = 0;
        count = 10;
        while(i < count){
            char* str = intToString(i);
            print(str);
            free(str);
            nano_wait(10000000000);
            i++;
        }
    }
}

void display_highscore(int score){
    // [4, 3, 6] 5
    int index = -1;
    int val = 0;
    for(int i = 0; i < 3; i++){
        if (score > highscores[i]){
            if (i==0){
                index = i;
                val = highscores[i];    
            } 
            else if (val > highscores[i]){
                index = i;
                val = highscores[i];
            } 
        }
    }
    // printf("Index is: %d\n", index);
    
    // store_highscore(score, 20 * index);
    please(intToString(score), 20 * index);
}


void please(const char* data, int location){
    uint8_t len = strlen(data);
    eeprom_write(location, data, len);

    nano_wait(10000000);

    char read[len];
    eeprom_read(location ,read, len);
    nano_wait(10000000);

    read[len + 1] = '\0';
    // printf("%s", read);
    i2c_stop();
    // printf("%s\n", read);
    highscores[location/20] = atoi(read); // string into int 
    // printf("value: %d", highscores[location/20]);
}

void read_highscore(int index){
    int len = 2;
    char read_data[len];
    eeprom_read(20*index, read_data, len);
    nano_wait(10000000);
    read_data[len + 1] = '\0';
    printf("read data is: %s\n", read_data);
    // i2c_stop();

    // char read_data1[len];
    // eeprom_read(20, read_data1, len);
    // nano_wait(10000000);
    // read_data1[len + 1] = '\0';
    // printf("%s\n", read_data1);
    // i2c_stop();

    // char read_data2[len];
    // eeprom_read(40, read_data2, len);
    // nano_wait(10000000);
    // read_data2[len] = '\0';
    // printf("%s\n", read_data2);
    // i2c_stop();

    // printf("%s\n", read_data);
    spi1_display1("hello");

    highscores[0] = atoi(read_data); // string into int 
    // highscores[1] = atoi(read_data1);
    // highscores[2] = atoi(read_data2);
}

void store_highscore(int score, uint16_t position){
    if (position < 0){
        return;
    }

    char* write_data = intToString(score);
    uint8_t len = strlen(score);
    uint16_t write_location = position;

    eeprom_write(write_location, write_data, len);
    nano_wait(1000000000);

    // char read_data[len];
    // eeprom_read(write_location, read_data, len);
    // nano_wait(10000000);
    // read_data[len] = '\0';

    // printf("%s", read_data);
    // spi1_display1("high score is: ");
    // spi1_display2(read_data);
}

void lcdhighscore(){
    char* score1 = intToString(highscores[0]);
    char* score2 = intToString(highscores[1]);
    char* score3 = intToString(highscores[2]);
    char* comma = ", ";

    char* one = strcat(score1, comma);
    one = strcat(one, score2);
    one = strcat(one, comma);
    one = strcat(one, score3);
    // printf("%s\n", one);
    spi1_display1("highscore: ");
    spi1_display2(one);
}

int main(void){
    internal_clock(); //set up internal clock (as usual in labs)
    srand(12345);

    // START
    init_keypad(); // Set up new keypad logic

    init_usart5();
    enable_tty_interrupt();
    setbuf(stdout,0);
    setbuf(stderr,0);

    init_spi1();
    spi1_init_oled();

    // // printf("hell");
    init_i2c(); 
    enable_ports_EEPROM();
    setup_dma();
    enable_dma();
    init_tim15(); // dma
    init_tim7(); // scoring and keypad scanning

    // please(intToString(2), 0);
    // please(intToString(3), 20);
    // please(intToString(1), 40);

    // // store_highscore(4, 0);
    // // read_highscore(0);

    // // nano_wait(1000000000);
    // // store_highscore(1, 20);
    // // read_highscore(1);
    // // store_highscore(0, 40); 
    // // read_highscore(0);
    // // nano_wait(10000000);
    // // read_highscore(1);
    // // printf("%d\n", highscores[0]);
    // spi1_display1("            ");
    // display_highscore(score);
    // lcdhighscore();

    // printf("score 1: %d\n score 2: %d\n score 3: %d\n", highscores[0], highscores[1], highscores[2]);

    // // Sound logic
    setup_dac(); // sound stuff
    init_tim6(); // sound timer (starts sound timer)

    // whack = 1;
    // game_over();

    // // GAME STARTS
    matrixGPIO(); // Set up LED Matrix pins
    clearMatrix(); // Clear LED Matrix to start with empty board
    adjust_priorities(); //adjust interrupt prioritues
    setupMoleTimer(); // mole movement based on difficulty
    endGameTimer(); // 60 second timer for the whole game

    // //currently testing the score using this:
    // init_usart5();
    // enable_tty_interrupt();
    // setbuf(stdout,0);
    // setbuf(stderr,0);

    // // GAME OVER


    return 0;
}