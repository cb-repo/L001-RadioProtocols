/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef SBUS_H
#define SBUS_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"
#include "Core.h"
#include "UART.h"
#include "GPIO.h"

/*
 * PUBLIC DEFINITIONS
 */

#define SBUS_NUM_CHANNELS		16		// x16 standard channels

#define SBUS_BAUD				100000
#define SBUS_BAUD_FAST			200000

#define SBUS_PERIOD_ANALOGUE	14
#define SBUS_PERIOD_FAST		7
#define	SBUS_PERIOD				SBUS_PERIOD_ANALOGUE

#define SBUS_MIN				172
#define SBUS_CENTER				991
#define SBUS_MAX				1811

/*
 * PUBLIC TYPES
 */

typedef struct {
	GPIO_t * GPIO;
	uint32_t Pin;
	UART_t * UART;
	uint32_t Baud;
} SBUS_Properties;

typedef struct {
	bool inputLost;
	bool frameLost;
	bool failsafe;
	bool ch17;
	bool ch18;
	int16_t ch[SBUS_NUM_CHANNELS];
} SBUS_Data;

/*
 * PUBLIC FUNCTIONS
 */

bool SBUS_Detect (SBUS_Properties);
void SBUS_Init (SBUS_Properties);
void SBUS_Deinit (void);
void SBUS_Update (void);
SBUS_Data* SBUS_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* SBUS_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
