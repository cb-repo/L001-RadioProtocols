
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PWM.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PWM_DETECT_MS	(PWM_TIMEIN_CYCLES * PWM_PERIOD_MAX_MS * 2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct {
    uint32_t	pin;
    void 		(*irqHandler)(void);
} PWM_channels_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void PWM_Process ( RADIO_chIndex_t );

static void PWM_IRQ 	( RADIO_chIndex_t );
static void PWM_CH1_IRQ ( void );
#if PWM_CH_NUM >= 2
static void PWM_CH2_IRQ ( void );
#endif
#if PWM_CH_NUM >= 3
static void PWM_CH3_IRQ ( void );
#endif
#if PWM_CH_NUM >= 4
static void PWM_CH4_IRQ ( void );
#endif
#if PWM_CH_NUM > 4
#error // Too Many Channels Defined
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static const PWM_channels_t pwmCh[PWM_CH_NUM] = {	{ PWM_CH1_Pin, PWM_CH1_IRQ },
#if PWM_CH_NUM >= 2
													{ PWM_CH2_Pin, PWM_CH2_IRQ },
#endif
#if PWM_CH_NUM >= 3
													{ PWM_CH3_Pin, PWM_CH3_IRQ },
#endif
#if PWM_CH_NUM >= 4
													{ PWM_CH4_Pin, PWM_CH4_IRQ },
#endif
};

static volatile uint32_t	rx[ PWM_CH_NUM ];
uint32_t    				ch[ PWM_CH_NUM ];
bool    					chFault[ PWM_CH_NUM ];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * PWM_Init
 *  -
 */
void PWM_Init ( void )
{
	// RESET RADIO DATA ARRAYS
	for (uint8_t c = 0; c < PWM_CH_NUM; c++) {
		rx[c] = 0;
		ch[c] = 0;
		chFault[c] = true;
	}

	// START TIMER TO MEASURE PULSE WIDTHS
	TIM_Init(  PWM_TIM, PWM_TIM_FREQ, PWM_TIM_RELOAD );
	TIM_Start( PWM_TIM );

	// CONFIGURE EACH INPUT PIN AND ASSIGN IRQ
	for ( uint8_t c = 0; c < PWM_CH_NUM; c++ ) {
		GPIO_EnableInput( pwmCh[c].pin, GPIO_Pull_Down);
		GPIO_OnChange( pwmCh[c].pin, GPIO_IT_Both, pwmCh[c].irqHandler );
	}

	// RUN A PWM DATA UPDATE BEFORE PROGRESSING
	PWM_Update();
}


/*
 * PWM_Deinit
 *  -
 */
void PWM_Deinit ( void )
{
	// STOP AND DEINITIALISE THE RADIO TIMER
	TIM_Deinit(PWM_TIM);

	// DEINITIALISE AND UNASIGN IRQ FOR EACH RADIO INPUT PIN
	for ( uint8_t c = 0; c < PWM_CH_NUM; c++ ) {
		GPIO_OnChange( pwmCh[c].pin, GPIO_IT_None, NULL );
		GPIO_Deinit( pwmCh[c].pin );
	}
}


/*
 * PWM_Detect
 *  -
 */
bool PWM_Detect ( void )
{
	// INITIALSIE FUNCTION VARIABLES
	uint32_t tick = CORE_GetTick();

	// RUN PWM PROTOCOL DETECTION
	PWM_Init();
	while ( PWM_DETECT_MS > (CORE_GetTick() - tick) )
	{
		// comment
		PWM_Update();

		//
		bool noFaults = true;
		for ( uint8_t c = 0; c < PWM_CH_NUM; c++ ) {
			if ( chFault[c] ) {
				noFaults = false;
				break;
			}
		}

		//
		if ( noFaults ) {
			return true;
		}

		CORE_Idle();
	}

	// DEINITIALISE IF NO RADIO DETECTED
	PWM_Deinit();
	return false;
}


/*
 * PWM_Update
 *  -
 */
void PWM_Update ( void )
{
	// INITIALISE FUCNTION VARIABLES
	static uint32_t tick[ PWM_CH_NUM ] 		= {0};
	static uint8_t 	validCount[ PWM_CH_NUM ]	= {0};
	uint32_t 		now 					= CORE_GetTick();

	// ITTERATE THROUGH EACH CHANNEL
	for ( uint8_t c = 0; c < PWM_CH_NUM; c++ )
	{
		// STATE - TIMEDOUT (OR STARTUP)
		if ( chFault[c] )
		{
			// CHECK FOR DATA IN THE RECIEVE BUFFER
			if ( rx[c] ) {
				// HAVE WE REACHED TIME IN CONDITION
				if ( ++validCount[c] >= PWM_TIMEIN_CYCLES ) {
					// RESET FAULT FLAG AND PROCEED TO NORMAL OPERATION - DONT RESET RX ARRAY
					chFault[c] = false;
				} else {
					// RESET DATA FOR NEXT LOOP
					rx[c] = 0;
					tick[c] = now;
				}
			}
			// CHECK FOR TIMEIN COUNT RESET
			else if ( validCount[c] && (now - tick[c] >= PWM_PERIOD_MAX_MS) ) {
				validCount[c] = 0;
				tick[c] = now;
			}
		}

		// STATE - NORMAL OPERATION
		if ( !chFault[c] )
		{
			// CHECK FOR NEW DATA
			if ( rx[c] )
			{
				// PROCESS DATA
				PWM_Process(c);
				// RESET RELEVANT FLAGS
				tick[c] = now;
			}

			// CHECK FOR TIMEOUT CONDITION
			else if ( now - tick[c] >= PWM_TIMEOUT_MS )
			{
				// SET RELEVANT FLAGS
				chFault[c] = true;
				ch[c] = 0;
				tick[c] = now;
				validCount[c] = 0;
			}
		}
	}
}


/*
 * PWM_getDataPtr
 *  -
 */
uint32_t* PWM_getDataPtr ( void )
{
	return ch;
}


/*
 * PWM_getInputLostPtr
 *  -
 */
bool* PWM_getInputLostPtr ( void )
{
	return chFault;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * PWM_Process
 *  -
 */
static void PWM_Process ( RADIO_chIndex_t c )
{
	// EXTRACT DATA AND RESET TEMP ARRAY
	uint32_t pulse = rx[c];
	rx[c] = 0;

	// TRUNCATE RADIO DATA AND MOVE TO OUTBOUND ARRAY
	// WE ALREADY KNOW DATA IS GREATER THAN RADIO_CH_ABSMIN AND SMALLER THAN RADIO_CH_ABSMAX
	if ( pulse < RADIO_CH_MIN ) {
		ch[c] = RADIO_CH_MIN;
	}
	else if ( pulse > RADIO_CH_MAX ) {
		ch[c] = RADIO_CH_MAX;
	}
	else {
		ch[c] = pulse;
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS   									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * PWM_IRQ
 *  -
 */
static void PWM_IRQ ( RADIO_chIndex_t c )
{
	// INITIALISE LOOP VARIABLES
	uint32_t 		now 					= TIM_Read(  PWM_TIM);
	bool 			pos 					= GPIO_Read( pwmCh[c].pin );
	static bool 	pos_p[PWM_CH_NUM]		= { false };
	static uint32_t	tickHigh[PWM_CH_NUM] 	= {0};
	static uint32_t	tickLow[PWM_CH_NUM] 	= {0};

	// IGNORE NOISE ON SIGNAL I/P THAT RETURNS FASTER THAN INTERRRUPT SERVICE
	if ( pos != pos_p[c] )
	{
		// RISING EDGE PULSE DETECTED
		if ( pos ) {
			// ASSIGN VARIABLES TO USE ON PULSE LOW
			tickHigh[c] = now;
		}
		// FALLING EDGE PULSE DETECTED
		else {
			// CALCULATE SIGNAL PERIOD AND PULSE WIDTH
			uint32_t period = now - tickLow[c];
			uint32_t pulse = now - tickHigh[c];
			// CHECK SIGNAL IS VALID
			if ( pulse <= RADIO_CH_ABSMAX 		&& pulse >= RADIO_CH_ABSMIN &&
				 period <= PWM_PERIOD_MAX_US	&& period >= PWM_PERIOD_MIN_US )
			{
				// ASSIGN PULSE TO TEMP DATA ARRAY
				rx[c] = pulse;
			}
			// UPDATE VARIABLES FOR NEXT LOOP
			tickLow[c] = now;
		}

		//
		pos_p[c] = pos;
	}
}


static void PWM_CH1_IRQ(void)
{
	PWM_IRQ(CH1);
}

#if PWM_CH_NUM >= 2
static void PWM_CH2_IRQ(void)
{
	PWM_IRQ(CH2);
}
#endif

#if PWM_CH_NUM >= 3
static void PWM_CH3_IRQ(void)
{
	PWM_IRQ(CH3);
}
#endif

#if PWM_CH_NUM >= 4
static void PWM_CH4_IRQ(void)
{
	PWM_IRQ(CH4);
}
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
