
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef PWM_H
#define PWM_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#include "STM32X.h"

#include "Core.h"
#include "GPIO.h"
#include "TIM.h"

#include "Radio.h"


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#if defined(PWM_CH_NUM)
#if PWM_CH_NUM > 4
#error "PWM_CH_NUM cannot be greater than 4"
#endif
#elif defined(PWM_CH1_Pin) && defined(PWM_CH2_Pin) && defined(PWM_CH3_Pin) && defined(PWM_CH4_Pin)
#define PWM_CH_NUM	4
#elif defined(PWM_CH1_Pin) && defined(PWM_CH2_Pin) && defined(PWM_CH3_Pin)
#define PWM_CH_NUM	3
#elif defined(PWM_CH1_Pin) && defined(PWM_CH2_Pin)
#define PWM_CH_NUM	2
#elif defined(PWM_CH1_Pin)
#define PWM_CH_NUM	1
#else
#error "error defining PWM_CH_NUM"
#endif

#define PWM_PERIOD_MS		20
#define PWM_PERIOD_MAX_MS	25
#define PWM_PERIOD_MAX_US 	(PWM_PERIOD_MAX_MS*1000)
#define PWM_PERIOD_MIN_MS 	10
#define PWM_PERIOD_MIN_US 	(PWM_PERIOD_MIN_MS*1000)
#define PWM_PERIOD_US		(PWM_PERIOD_MS * 1000)

#define PWM_TIMEOUT_CYCLES	5
#define PWM_TIMEOUT_MS		(PWM_PERIOD_MAX_MS * PWM_TIMEOUT_CYCLES)

#define PWM_TIMEIN_CYCLES	3


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC TYPES      									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


void 		PWM_Init 		( void );
void 		PWM_Deinit 		( void );
bool 		PWM_Detect		( void );
void 		PWM_Update 		( void );


uint32_t*	PWM_getPtrData 	( void );
bool* 		PWM_getPtrFault ( void );


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EXTERN DECLARATIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* PWM_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
