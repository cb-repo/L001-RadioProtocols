/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef PWM_H
#define PWM_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"
#include "Core.h"
#include "GPIO.h"
#include "TIM.h"
#include "US.h"

/*
 * PUBLIC DEFINITIONS
 */

#if 	defined(RADIO_S4_PIN)
#define PWM_NUM_CHANNELS	4
#elif	defined(RADIO_S3_PIN)
#define PWM_NUM_CHANNELS	3
#elif 	defined(RADIO_S2_PIN)
#define PWM_NUM_CHANNELS	2
#elif 	defined(RADIO_S1_PIN)
#define PWM_NUM_CHANNELS	1
#endif

#define PWM_PERIOD_MS		20
#define PWM_PERIOD_US		(PWM_PERIOD_MS * 1000)
#define PWM_MIN				1000
#define PWM_CENTER			1500
#define PWM_MAX				2000

#define PWM_CH1				0
#define PWM_CH2				1
#define PWM_CH3				2
#define PWM_CH4				3

/*
 * PUBLIC TYPES
 */

typedef struct {
	bool inputLost;
	bool inputLostCh[PWM_NUM_CHANNELS];
	uint32_t ch[PWM_NUM_CHANNELS];
} PWM_Data;

/*
 * PUBLIC FUNCTIONS
 */

bool PWM_DetInit(void);
void PWM_Init (void);
void PWM_Deinit (void);
void PWM_Update (void);
PWM_Data* PWM_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* PWM_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
