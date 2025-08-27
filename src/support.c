// #include "stm32f0xx.h"
// #include <string.h> // for memmove()
// #include <stdio.h> // for memmove()

// void nano_wait(unsigned int n);

// void nano_wait(unsigned int n) {
//     asm(    "        mov r0,%0\n"
//             "repeat: sub r0,#83\n"
//             "        bgt repeat\n" : : "r"(n) : "r0", "cc");
// }


// //=============================================================================
// // Part 1: 7-segment display update with DMA
// //=============================================================================
// extern uint16_t msg[8];

// const char font[] = {
//     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
//     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
//     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
//     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
//     0x00, // 32: space
//     0x86, // 33: exclamation
//     0x22, // 34: double quote
//     0x76, // 35: octothorpe
//     0x00, // dollar
//     0x00, // percent
//     0x00, // ampersand
//     0x20, // 39: single quote
//     0x39, // 40: open paren
//     0x0f, // 41: close paren
//     0x49, // 42: asterisk
//     0x00, // plus
//     0x10, // 44: comma
//     0x40, // 45: minus
//     0x80, // 46: period
//     0x00, // slash
//     // digits
//     0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
//     // seven unknown
//     0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
//     // Uppercase
//     0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x6f, 0x76, 0x30, 0x1e, 0x00, 0x38, 0x00,
//     0x37, 0x3f, 0x73, 0x7b, 0x31, 0x6d, 0x78, 0x3e, 0x00, 0x00, 0x00, 0x6e, 0x00,
//     0x39, // 91: open square bracket
//     0x00, // backslash
//     0x0f, // 93: close square bracket
//     0x00, // circumflex
//     0x08, // 95: underscore
//     0x20, // 96: backquote
//     // Lowercase
//     0x5f, 0x7c, 0x58, 0x5e, 0x79, 0x71, 0x6f, 0x74, 0x10, 0x0e, 0x00, 0x30, 0x00,
//     0x54, 0x5c, 0x73, 0x7b, 0x50, 0x6d, 0x78, 0x1c, 0x00, 0x00, 0x00, 0x6e, 0x00
// };

// void set_digit_segments(int digit, char val) {
//     msg[digit] = (digit << 8) | val;
// }

// // void print(const char str[])
// // {
// //     const char *p = str;
// //     for(int i=0; i<8; i++) {
// //         if (*p == '\0') {
// //             msg[i] = (i<<8);
// //         } else {
// //             msg[i] = (i<<8) | font[*p & 0x7f] | (*p & 0x80);
// //             p++;
// //         }
// //     }
// // }

// void printfloat(float f)
// {
//     char buf[10];
//     snprintf(buf, 10, "%f", f);
//     for(int i=1; i<10; i++) {
//         if (buf[i] == '.') {
//             // Combine the decimal point into the previous digit.
//             buf[i-1] |= 0x80;
//             memcpy(&buf[i], &buf[i+1], 10-i-1);
//         }
//     }
//     print(buf);
// }

// void append_segments(char val) {
//     for (int i = 0; i < 7; i++) {
//         set_digit_segments(i, msg[i+1] & 0xff);
//     }
//     set_digit_segments(7, val);
// }

// void clear_display(void) {
//     for (int i = 0; i < 8; i++) {
//         msg[i] = msg[i] & 0xff00;
//     }
// }

// //=============================================================================
// // Part 2: Debounced keypad scanning.
// //=============================================================================

// // 16 history bytes.  Each byte represents the last 8 samples of a button.

// // void drive_column(int c)
// // {
// //     GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
// // }



// // void show_keys(void)
// // {
// //     char buf[] = "        ";
// //     for(;;) {
// //         char event = get_key_event();
// //         memmove(buf, &buf[1], 7);
// //         buf[7] = event;
// //         print(buf);
// //     }
// // }