/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef PWM_H
#define PWM_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"
#include "UART.h"

/*
 * PUBLIC DEFINITIONS
 */

#define PWM_NUM_CHANNELS	4

#define PWM_PERIOD			20
#define PWM_MIN				1000
#define PWM_CENTER			1500
#define PWM_MAX				2000
#define PWM_FULLSCALE		(PWM_MAX-PWM_MIN)
#define PWM_HALFSCALE		(PWM_MAX-PWM_CENTER)


/*
 * PUBLIC TYPES
 */

typedef struct {
	bool lost_frame;
	bool failsafe;
	int16_t ch[PWM_NUM_CHANNELS];
} PWM_Data;

/*
 * PUBLIC FUNCTIONS
 */

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
