/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef PPM_H
#define PPM_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"
#include "Core.h"
#include "GPIO.h"
#include "TIM.h"

/*
 * PUBLIC DEFINITIONS
 */

#define PPM_NUM_CHANNELS	8

#define PPM_PERIOD			20
#define PPM_MIN				1000
#define PPM_CENTER			1500
#define PPM_MAX				2000

/*
 * PUBLIC TYPES
 */

typedef struct {
	bool inputLost;
	int16_t ch[PPM_NUM_CHANNELS];
} PPM_Data;

/*
 * PUBLIC FUNCTIONS
 */

bool PPM_Detect(void);
void PPM_Init (void);
void PPM_Deinit (void);
void PPM_Update (void);
PPM_Data* PPM_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* PPM_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
