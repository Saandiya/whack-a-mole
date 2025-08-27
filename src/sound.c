// //SOUND EFFECTS USES PA4 AS ANALOG, TIM6 AND DAC CHANNEL 1
// #include "stm32f0xx.h"
// #include <math.h>   // for M_PI
// #include <stdint.h>
// #include <stdio.h>

// void nano_wait(int);
// void internal_clock();
// void dialer(void);

// //============================================================================
// // setup_dac()
// //============================================================================
// void setup_dac(void) {
//     RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // ENABLE CLOCK TO PORT A
//     GPIOA->MODER |= 3 << (2*4); // SET PA4 TO ANALOG
//     RCC->APB1ENR |= RCC_APB1ENR_DACEN; // ENABLE CLOCK TO ADC
//     DAC->CR &= ~DAC_CR_TSEL1; // TIM6 AS TRIGGER 000
//     DAC->CR |= DAC_CR_TEN1; //ENABLE TRIGGER
//     DAC->CR |= DAC_CR_EN1; // ENABLE DAC CHANNEL 1
// }

// //============================================================================
// // Timer 6 ISR
// //============================================================================
// // Write the Timer 6 ISR here.  Be sure to give it the right name.


// void TIM6_DAC_IRQHandler() {
//     TIM6->SR &= ~TIM_SR_UIF; // ACK INTERRUPT
//     offset0 += step0;

//      if (whack)
//     {
//         if (offset0 <= N)
//         {
//             DAC->DHR8R1 = (wavetable[offset0]) ^ 128;
//         }
//     }
//     else if (game)
//     {
//         if (offset0 >= N) 
//         {
//             offset0 = 0;
//         }
//         DAC->DHR8R1 = (wavetable[offset0]) ^ 128;
//     }
// }

// //============================================================================
// // init_tim6()
// //============================================================================
// void init_tim6(void) {
//     RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
//     TIM6->PSC = 480 - 1;
//     TIM6->ARR = (100000 / RATE) - 1;
//     TIM6->DIER |= TIM_DIER_UIE;
//     TIM6->CR2 |= 0X0020; // MMS 010 UPDATE
//     TIM6->CR1 |= TIM_CR1_CEN;
//     NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
// }

// // int main(void) {
// //     internal_clock();
// // if bla bla bla begin
// //     whack = ;
// //     game = ;
// //     N = ;
// //     wavetable = ;
// //     setup_dac();
// //     init_tim6(); end
// // }