/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef SBUS_H
#define SBUS_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"
#include "UART.h"

/*
 * PUBLIC DEFINITIONS
 */

#define SBUS_NUM_CHANNLS	16

#define SBUS_BAUD			100000
#define SBUS_BAUD_FAST		200000

/*
 * PUBLIC TYPES
 */

typedef struct {
	bool lost_frame;
	bool failsafe;
	bool ch17;
	bool ch18;
	int16_t ch[SBUS_NUM_CHANNLS];
} SBUS_Data;

/*
 * PUBLIC FUNCTIONS
 */

void SBUS_Init (UART_t *, uint32_t);
void SBUS_Deinit (void);
void SBUS_Update (void);
SBUS_Data* SBUS_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* SBUS_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
