/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef IBUS_H
#define IBUS_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"
#include "Core.h"
#include "UART.h"
#include "GPIO.h"
#include "US.h"

/*
 * PUBLIC DEFINITIONS
 */

#define IBUS_NUM_CHANNELS	14

#define IBUS_BAUD			115200

#define IBUS_PERIOD			7
#define IBUS_MIN			0x3E8	// == 1000
#define IBUS_CENTER			0x5DC	// == 1500
#define IBUS_MAX			0x7D0	// == 2000

/*
 * PUBLIC TYPES
 */

typedef struct {
	bool inputLost;
	uint32_t ch[IBUS_NUM_CHANNELS];
} IBUS_Data;

/*
 * PUBLIC FUNCTIONS
 */

bool IBUS_DetInit(void);
void IBUS_Init (void);
void IBUS_Deinit (void);
void IBUS_Update (void);
IBUS_Data* IBUS_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* SBUS_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
