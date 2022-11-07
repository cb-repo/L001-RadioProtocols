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
	GPIO_t * GPIO;
	uint8_t Pin;
	TIM_t * Timer;
	uint32_t Tim_Freq;
	uint32_t Tim_Reload;
} PPM_Properties;

typedef struct {
	bool failsafe;
	int16_t ch[PPM_NUM_CHANNELS];
} PPM_Data;

/*
 * PUBLIC FUNCTIONS
 */

bool PPM_Detect(PPM_Properties);
void PPM_Init (PPM_Properties);
void PPM_Deinit (void);
void PPM_Update (void);
PPM_Data* PPM_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* PPM_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
