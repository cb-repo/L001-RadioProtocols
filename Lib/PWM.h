/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef PWM_H
#define PWM_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"
#include "Core.h"
#include "GPIO.h"
#include "TIM.h"

/*
 * PUBLIC DEFINITIONS
 */

#define PWM_NUM_CHANNELS	4

#define PWM_PERIOD			20
#define PWM_MIN				1000
#define PWM_CENTER			1500
#define PWM_MAX				2000

/*
 * PUBLIC TYPES
 */

typedef struct {
	GPIO_t * GPIO[PWM_NUM_CHANNELS];
	uint8_t Pin[PWM_NUM_CHANNELS];
	TIM_t * Timer;
	uint32_t Tim_Freq;
	uint32_t Tim_Reload;
} PWM_Properties;

typedef struct {
	bool failsafe;
	int16_t ch[PWM_NUM_CHANNELS];
} PWM_Data;

/*
 * PUBLIC FUNCTIONS
 */

bool PWM_Detect(PWM_Properties);
void PWM_Init (PWM_Properties);
void PWM_Deinit (void);
void PWM_Update (void);
PWM_Data* PWM_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* PWM_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
