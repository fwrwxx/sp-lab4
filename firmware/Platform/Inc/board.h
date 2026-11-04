#ifndef BOARD_H
#define BOARD_H

/* Ініціалізація платформи: bit-bang I2C піни, CS для SPI-flash тощо.
 * Викликається з main() перед App_Init(). */
void Board_Init(void);

#endif