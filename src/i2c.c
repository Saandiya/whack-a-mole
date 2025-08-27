#include "stm32f0xx.h"
#include <stdint.h>
#include <string.h> // for memmove()
#include <stdlib.h> // for srandom() and random()
#include <stdio.h>
#include "commands.h"
#include "fifo.h"
#include "tty.h"

void small_delay(void) {
    nano_wait(50000);
}


uint8_t col; // the column being scanned
uint16_t msg[8] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700 };

void drive_column(int);   // energize one of the column outputs
int  read_rows();         // read the four row inputs
void update_history(int col, int rows); // record the buttons of the driven column
char get_key_event(void); // wait for a button event (press or release)
char get_keypress(void);  // wait for only a button press event.
float getfloat(void);     // read a floating-point number from keypad
void show_keys(void);     // demonstrate get_key_event()
void print(const char str[]);

// void enable_ports(void) {
//     RCC->AHBENR |= RCC_AHBENR_GPIOBEN ;
//     RCC->AHBENR |= RCC_AHBENR_GPIOCEN; // enabling RCC for GPIOB

//     GPIOB->MODER &= ~0x3fffff;
//     GPIOB->MODER |= 0x155555; // setting 0-10 to output

//     GPIOC->MODER &= ~0xff00;
//     GPIOC->MODER |= 0x5500; // setting 4-7 to output

//     GPIOC->OTYPER |= 0xf0; // sets 4-7 to be open-drain

//     GPIOC->MODER &= ~0xff; // sets 0-3 to input

//     GPIOC->MODER &= ~0xff;
//     GPIOC->PUPDR |= 0x55; // sets 0-3 to pull-up
// }

void setup_dma(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= ~0x1; // disable the channel 5 for DMA1
    DMA1_Channel5->CMAR = (uint32_t) &msg; // 32-bit address of msg
    DMA1_Channel5->CPAR = (uint32_t) &(GPIOB->ODR); // 32-bit address of GPIOB_ODR
    DMA1_Channel5->CNDTR = 8;  // 8 LEDs
    DMA1_Channel5->CCR |= DMA_CCR_DIR; // sets transfer from mem to per
    DMA1_Channel5->CCR |= DMA_CCR_MINC | DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_CIRC;  
}

void enable_dma(void) {
    DMA1_Channel5->CCR |= DMA_CCR_EN;    
}

void init_tim15(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM15->PSC = 4800 - 1;
    TIM15->ARR = 10 - 1;
    TIM15->DIER |= TIM_DIER_UDE;
    TIM15->CR1 |= TIM_CR1_CEN;
}

// void TIM7_IRQHandler(){
//     TIM7->SR &= ~TIM_SR_UIF;
//     int rows = read_rows();
//     update_history(col, rows);
//     col = (col + 1) & 3;
//     drive_column(col);
// }

// void init_tim7(void) {
//     RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
//     TIM7->PSC = 4800 - 1;
//     TIM7->ARR = 10 - 1;
//     TIM7->DIER |= TIM_DIER_UIE;
//     NVIC->ISER[0] = (1 << TIM7_IRQn);
//     TIM7->CR1 |= TIM_CR1_CEN; 
// }

void init_spi1() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    GPIOA->MODER &= ~0xc000cc00;
    GPIOA->MODER |= 0x80008800; 
    GPIOA->AFR[0] &= ~0xf0f00000; // set AF0 for PA5, 7
    GPIOA->AFR[1] &= ~0xf0000000; // set AFO for PA15

    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR2 |= SPI_CR2_DS_3 | SPI_CR2_DS_0; // set bit size to 10
    SPI1->CR2 &=  ~(SPI_CR2_DS_2 | SPI_CR2_DS_1);
    
    SPI1->CR1 |= SPI_CR1_BR; 

    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR2 |= SPI_CR2_SSOE;
    SPI1->CR2 |= SPI_CR2_NSSP;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;

    SPI1->CR1 |= SPI_CR1_SPE; // enable the SPI1
}

void spi_cmd(unsigned int data) {
    while((SPI1->SR & 0x0002) == 0){
        small_delay();
    }
    SPI1->DR = data; // copy data into a register
}

void spi_data(unsigned int data) {
    spi_cmd(data | 0x200);
}

void spi1_init_oled() {
    nano_wait(1000000);   
    spi_cmd(0x38);
    spi_cmd(0x08);
    spi_cmd(0x01);

    nano_wait(2000000);
    
    spi_cmd(0x06);
    spi_cmd(0x02);
    spi_cmd(0x0c);
}

void spi1_display1(const char *string) {
    spi_cmd(0x02);
    while(*string != '\0'){
        spi_data(*string);
        string += 1;
    }
}

void spi1_display2(const char *string) {
    spi_cmd(0xc0); 
    while(*string != '\0'){
        spi_data(*string);
        string += 1;
    } 
}




// 

void enable_ports_EEPROM(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= 0x00280000; // set PA9, PA10 to AF
    GPIOA->AFR[1] |= 0x440; // set to AFRH
}

//===========================================================================
// Configure I2C1.
//===========================================================================
void init_i2c(void) {
    RCC->APB1ENR |=  RCC_APB1ENR_I2C1EN;
    I2C1->CR1 &=  ~I2C_CR1_PE; // disable
    I2C1->CR1 |= I2C_CR1_ANFOFF; // disable analog noise
    I2C1->CR1 |= I2C_CR1_ERRIE; //error interrupt enable
    I2C1->CR1 |=  I2C_CR1_NOSTRETCH; // disable stretch

    I2C1->TIMINGR |= 0x50000000; // set PRESC to 6
    I2C1->TIMINGR |= 0x00310309; // SCLDEL to 2, SDADEL to 2 

    I2C1->CR2 &= ~I2C_CR2_ADD10; // set to 7 bit addressing
    I2C1->CR1 |= I2C_CR1_PE; // enable
}

//===========================================================================
// Send a START bit.
//===========================================================================
void i2c_start(uint32_t targadr, uint8_t size, uint8_t dir) {
  // 0. Take current contents of CR2 register. 
    uint32_t tmpreg = I2C1->CR2;

    // 1. Clear the following bits in the tmpreg: SADD, NBYTES, RD_WRN, START, STOP
    tmpreg &= ~I2C_CR2_SADD;
    tmpreg &= ~I2C_CR2_NBYTES; 
    tmpreg &= ~I2C_CR2_RD_WRN;
    tmpreg &= ~I2C_CR2_START;   
    tmpreg &= ~I2C_CR2_STOP;

    // 2. Set read/write direction in tmpreg.
    tmpreg |= dir << 10; 

    // 3. Set the target's address in SADD (shift targadr left by 1 bit) and the data size.
    tmpreg |= ((targadr<<1) & I2C_CR2_SADD) | ((size << 16) & I2C_CR2_NBYTES);

    // 4. Set the START bit.
    tmpreg |= I2C_CR2_START;

    // 5. Start the conversion by writing the modified value back to the CR2 register.
    I2C1->CR2 = tmpreg;
}

//===========================================================================
// Send a STOP bit.
//===========================================================================
void i2c_stop(void) {
    // 0. If a STOP bit has already been sent, return from the function.
// Check the I2C1 ISR register for the corresponding bit.
    if(I2C1->CR2 & I2C_CR2_STOP){
        return;
    }

// 1. Set the STOP bit in the CR2 register.
    I2C1->CR2 |=  I2C_CR2_STOP; 

// 2. Wait until STOPF flag is reset by checking the same flag in ISR.
    while(!(I2C1->ISR & I2C_ISR_STOPF));

// 3. Clear the STOPF flag by writing 1 to the corresponding bit in the ICR.
    I2C1->ICR = I2C_ICR_STOPCF; // set to 7 bit addressing
}

//===========================================================================
// Wait until the I2C bus is not busy. (One-liner!)
//===========================================================================
void i2c_waitidle(void) {
    while(I2C1->ISR & I2C_ISR_BUSY);   
}

//===========================================================================
// Send each char in data[size] to the I2C bus at targadr.
//===========================================================================
int8_t i2c_senddata(uint8_t targadr, uint8_t data[], uint8_t size) {
    i2c_waitidle();
    i2c_start(targadr, size, 0);

    for(int i = 0; i < size; i++){
        int count = 0;
        while ((I2C1->ISR & I2C_ISR_TXIS) == 0) {
            count += 1;
            if (count > 1000000) {
                printf("timeout\n");
                return -1;
            }
            if (i2c_checknack()) {
                printf("NACK\n");
                i2c_clearnack();
                i2c_stop();
                return -1;
            }
        }
        data[i] &= I2C_TXDR_TXDATA; 
        I2C1->TXDR = data[i];
        printf("sending %d (%c)\n", data[i], data[i]);
    }

    while( !((I2C1->ISR & I2C_ISR_NACKF)) && !((I2C1->ISR & I2C_ISR_TC))); // wait till both are zero

    if(I2C1->ISR & I2C_ISR_NACKF){
        return -1;
    }

    i2c_stop();
    return 0;
}

//===========================================================================
// Receive size chars from the I2C bus at targadr and store in data[size].
//===========================================================================
int i2c_recvdata(uint8_t targadr, void *data, uint8_t size) {
    char *tmpdata = (char*)data;
    i2c_waitidle();
    i2c_start(targadr, size, 1);

    for(int i = 0; i < size; i++){
        int count = 0;
        while ((I2C1->ISR & I2C_ISR_RXNE) == 0) {
            count += 1;
            if (count > 1000000) {
                printf("timeout\n");
                return -1;
            }
            if (i2c_checknack()) {
                printf("NACK\n");
                i2c_clearnack();
                i2c_stop();
                return -1;
            }
        }
        tmpdata[i] = I2C1->RXDR & I2C_RXDR_RXDATA;  // Mask the RXDR value and store it in data[i]
        printf("recieved %c \n", tmpdata[i]);
    }
    return 0;
}

//===========================================================================
// Clear the NACK bit. (One-liner!)
//===========================================================================
void i2c_clearnack(void) {
    I2C1->ICR &= ~I2C_ICR_NACKCF;
}

//===========================================================================
// Check the NACK bit. (One-liner!)
//===========================================================================
int i2c_checknack(void) {
    if(I2C1->ISR & I2C_ISR_NACKF){
        return 1;
    }
    return 0;
}

//===========================================================================
// EEPROM functions
// We'll give these so you don't have to figure out how to write to the EEPROM.
// These can differ by device.

#define EEPROM_ADDR 0x57

void eeprom_write(uint16_t loc, const char* data, uint8_t len) {
    uint8_t bytes[34];
    bytes[0] = loc>>8;
    bytes[1] = loc&0xFF;
    for(int i = 0; i<len; i++){
        bytes[i+2] = data[i];
    }
    i2c_senddata(EEPROM_ADDR, bytes, len+2);
}

void eeprom_read(uint16_t loc, char data[], uint8_t len) {
    // ... your code here
    uint8_t bytes[2];
    bytes[0] = loc>>8;
    bytes[1] = loc&0xFF;
    i2c_senddata(EEPROM_ADDR, bytes, 2);
    i2c_recvdata(EEPROM_ADDR, data, len);
}

char* intToString(int num) {
    // Allocate memory for the string
    // The space is enough to store a 32-bit integer and the null terminator
    char* str = (char*)malloc(20 * sizeof(char));

    if (str != NULL) {
        // Convert the integer to string
        snprintf(str, 20, "%d", num);
    }
    return str;  // Return the string (caller is responsible for freeing memory)
}

#include "stm32f0xx.h"
#include <string.h> // for memmove()
#include <stdio.h> // for memmove()

void nano_wait(unsigned int n);

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}


//=============================================================================
// Part 1: 7-segment display update with DMA
//=============================================================================
extern uint16_t msg[8];

const char font[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, // 32: space
    0x86, // 33: exclamation
    0x22, // 34: double quote
    0x76, // 35: octothorpe
    0x00, // dollar
    0x00, // percent
    0x00, // ampersand
    0x20, // 39: single quote
    0x39, // 40: open paren
    0x0f, // 41: close paren
    0x49, // 42: asterisk
    0x00, // plus
    0x10, // 44: comma
    0x40, // 45: minus
    0x80, // 46: period
    0x00, // slash
    // digits
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
    // seven unknown
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    // Uppercase
    0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x6f, 0x76, 0x30, 0x1e, 0x00, 0x38, 0x00,
    0x37, 0x3f, 0x73, 0x7b, 0x31, 0x6d, 0x78, 0x3e, 0x00, 0x00, 0x00, 0x6e, 0x00,
    0x39, // 91: open square bracket
    0x00, // backslash
    0x0f, // 93: close square bracket
    0x00, // circumflex
    0x08, // 95: underscore
    0x20, // 96: backquote
    // Lowercase
    0x5f, 0x7c, 0x58, 0x5e, 0x79, 0x71, 0x6f, 0x74, 0x10, 0x0e, 0x00, 0x30, 0x00,
    0x54, 0x5c, 0x73, 0x7b, 0x50, 0x6d, 0x78, 0x1c, 0x00, 0x00, 0x00, 0x6e, 0x00
};

void set_digit_segments(int digit, char val) {
    msg[digit] = (digit << 8) | val;
}

// void print(const char str[])
// {
//     const char *p = str;
//     for(int i=0; i<8; i++) {
//         if (*p == '\0') {
//             msg[i] = (i<<8);
//         } else {
//             msg[i] = (i<<8) | font[*p & 0x7f] | (*p & 0x80);
//             p++;
//         }
//     }
// }

void printfloat(float f)
{
    char buf[10];
    snprintf(buf, 10, "%f", f);
    for(int i=1; i<10; i++) {
        if (buf[i] == '.') {
            // Combine the decimal point into the previous digit.
            buf[i-1] |= 0x80;
            memcpy(&buf[i], &buf[i+1], 10-i-1);
        }
    }
    print(buf);
}

void append_segments(char val) {
    for (int i = 0; i < 7; i++) {
        set_digit_segments(i, msg[i+1] & 0xff);
    }
    set_digit_segments(7, val);
}

void clear_display(void) {
    for (int i = 0; i < 8; i++) {
        msg[i] = msg[i] & 0xff00;
    }
}

//=============================================================================
// Part 2: Debounced keypad scanning.
//=============================================================================

// 16 history bytes.  Each byte represents the last 8 samples of a button.

// void drive_column(int c)
// {
//     GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
// }



// void show_keys(void)
// {
//     char buf[] = "        ";
//     for(;;) {
//         char event = get_key_event();
//         memmove(buf, &buf[1], 7);
//         buf[7] = event;
//         print(buf);
//     }
// }

void print(const char str[])
{
    const char *p = str;
    for(int i=0; i<8; i++) {
        if (*p == '\0') {
            msg[i] = (i<<8);
        } else {
            msg[i] = (i<<8) | font[*p & 0x7f] | (*p & 0x80);
            p++;
        }
    }
}


// int main() {
//     internal_clock();
    
//     // I2C specific
//     enable_ports();
//     init_i2c();
//     enable_ports_EEPROM();
//     setup_dma();
//     enable_dma();
//     init_tim15();
//     init_tim7();
//     init_spi1();
//     spi1_init_oled();

//     // If you don't want to deal with the command shell, you can 
//     // comment out all code below and call 
//     // eeprom_read/eeprom_write directly.

//     // init_usart5();
//     // enable_tty_interrupt();
//     // setbuf(stdin,0); 
//     // setbuf(stdout,0);
//     // setbuf(stderr,0);

//     show_keys();
//     spi1_display1("Username: ");

//     int count = 10000;
//     int i = 0;
//     while(i < count){
//         spi1_display2(USERNAME);
//         i++;
//         nano_wait(1000000);
//     }

//     nano_wait(1000000);

//     count = 100;
//     i = 0;
//     while(i < count){
//         spi1_display1("              ");
//         spi1_display2("              ");
//         i++;
//         nano_wait(1000000);
//     }

//     clear_display(); // clears seven segment

//     i = 0;
//     count = 10;
//     while(i < count){
//         char* str = intToString(i);
//         print(str);
//         free(str);
//         nano_wait(10000000000);
//         i++;
//     }

//     nano_wait(1000000);
    
//     const char *write_data = "abc";
//     uint8_t len = strlen(write_data);
//     int lens = strlen(write_data);

//     uint16_t write_location = 0;

//     eeprom_write(write_location, write_data, len);
//     nano_wait(10000000);
//     char read_data[lens];
//     eeprom_read(write_location, read_data, len);
//     nano_wait(10000000);
//     read_data[lens + 1] = '\0';
//     spi1_display1("your score is: ");
//     spi1_display2(read_data);

//     // spi1_display1("Yourusernameis");

//     // for(;;) {
//     //     printf("\n> ");
//     //     char line[100];
//     //     fgets(line, 99, stdin);
//     //     line[99] = '\0';
//     //     int len = strlen(line);
//     //     if (line[len-1] == '\n')
//     //         line[len-1] = '\0';
//     //     parse_command(line);
//     // }
// }