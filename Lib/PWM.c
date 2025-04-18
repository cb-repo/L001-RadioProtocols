
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PWM.h"

#if defined(RADIO_USE_PWM)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#define PWM_DETECT_MS	(PWM_TIMEIN_CYCLES * PWM_PERIOD_MAX_MS * 2)


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


typedef struct {
    uint16_t pin;
    void (*irqHandler)(void);
} pwm_channel_t;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static void PWM_Process ( RADIO_chIndex );

static void PWM_IRQ 	( RADIO_chIndex );
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


static const pwm_channel_t pwmChannels[PWM_CH_NUM] = {	{ PWM_CH1_Pin, PWM_CH1_IRQ },
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

// Raw Pulse Widths Captured in IQR
static volatile uint32_t	rx[ PWM_CH_NUM ];

// Processed Channel Values and Fault Flags
uint32_t    ch[ PWM_CH_NUM ];
bool    	chFault[ PWM_CH_NUM ];


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * INITIALISES DATASTRUCTURES, STARTS TIMERS AND HANDLES CALIBRATION FOR THE PWM RADIO MODULE
 *
 * INPUTS: n/a
 * OUTPUTS: n/a
 */
void PWM_Init ( void )
{
	// RESET RADIO DATA ARRAYS
	for (uint8_t ch = 0; ch < PWM_CH_NUM; ch++) {
		rx[ch] = 0;
		ch[ch] = 0;
		chFault[ch] = true;
	}

	// START TIMER TO MEASURE PULSE WIDTHS
	TIM_Init(  TIM_RADIO, TIM_RADIO_FREQ, TIM_RADIO_RELOAD );
	TIM_Start( TIM_RADIO );

	// CONFIGURE EACH INPUT PIN AND ASSIGN IRQ
	for ( uint8_t c = CH1; c < PWM_CH_NUM; c++ ) {
		GPIO_EnableInput(	pwmChannels[i].pin, GPIO_Pull_Down);
		GPIO_OnChange(		pwmChannels[i].pin, GPIO_IT_Both, pwmChannels[i].irqHandler );
	}

	// RUN A PWM DATA UPDATE BEFORE PROGRESSING
	PWM_Update();
}


/*
 * DEINITIALISE IRQ'S AND STOPS TIMERS RELATED TO FUNCTION OF THE PWM RADIO MODULE
 *
 * INPUTS: n/a
 * OUTPUTS: n/a
 */
void PWM_Deinit ( void )
{
	// STOP AND DEINITIALISE THE RADIO TIMER
	TIM_Deinit(TIM_RADIO);

	// DEINITIALISE AND UNASIGN IRQ FOR EACH RADIO INPUT PIN
	for ( uint8_t c = CH1; c < PWM_CH_NUM; c++ ) {
		GPIO_OnChange(	pwmChannels[i].pin, GPIO_IT_None, NULL );
		GPIO_Deinit(	pwmChannels[i].pin );
	}
}


/*
 * HANDLES DETECTION OF VALID PWM SIGNALS
 *
 * THIS MODULE WILL INITIALISE WHAT IT NEEDS TO FOR CORRECT FUNCTIONALITY
 * IF DETECTION IS UNSICCESSFUL IT WILL DEINITIALISE ITSELF
 * IF DETECTIONS IS SUCCESSFUL IT WILL REMAIN ENABLED
 *
 * INPUTS: n/a
 * OUTPUTS: true = protocol detected, false = protocol not detected
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
		for ( uint8_t c = CH1; c < PWM_CH_MAX; c++ ) {
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
 * HANDLES EVERYTHING WITH ONGOING OPERATION OF PWM RADIO FUNCTIONALITY
 *
 * SHOULD BE CALLED EVERY LOOP (EVERY ~1ms)
 *
 * INPUTS: n/a
 * OUTPUTS: n/a
 */
void PWM_Update ( void )
{
	// INITIALISE FUCNTION VARIABLES
	static uint32_t tick[ PWM_CH_NUM ] 		= {0};
	static uint8_t 	validCount[ PWM_CH_NUM ]	= {0};
	uint32_t 		now 					= CORE_GetTick();

	// ITTERATE THROUGH EACH CHANNEL
	for ( uint8_t c = CH1; c < PWM_CH_NUM; c++ )
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
 * RETRIEVED A POINTER TO THE DATA STRUCTURE CONTAINING ALL PWM MODULE DATA
 *
 * INPUTS: n/a
 * OUTPUTS: Pointer of type RADIO_data
 */
uint32_t* PWM_getPtrData ( void )
{
	return &ch;
}


/*
 * RETRIEVED A POINTER TO
 *
 * INPUTS: n/a
 * OUTPUTS: Pointer of type RADIO_data
 */
bool* PWM_getPtrFault ( void )
{
	return &chFault;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * EXTRACTS RADIO DATA FROM TEMP ARRAY, PROCESSES IT AND MOVES IT TO THE OUTBOUND DATA ARRAY
 *
 * INPUTS: channel index to process of type RADIO_chIndex
 * OUTPUTS: n/a
 */
static void PWM_Process ( RADIO_chIndex c )
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


static void PWM_IRQ ( RADIO_chIndex c )
{
	// INITIALISE LOOP VARIABLES
	uint32_t 		now 					= TIM_Read(TIM_RADIO);
	bool 			pos 					= GPIO_Read(pwmChannels[i].pin);
	static bool 	pos_p[PWM_CH_NUM]		= {false};
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
	PWM_CaptureIRQHandler(CH1);
}

#if PWM_CH_NUM >= 2
static void PWM_CH2_IRQ(void)
{
	PWM_CaptureIRQHandler(CH2);
}
#endif

#if PWM_CH_NUM >= 3
static void PWM_CH3_IRQ(void)
{
	PWM_CaptureIRQHandler(CH3);
}
#endif

#if PWM_CH_NUM >= 4
static void PWM_CH4_IRQ(void)
{
	PWM_CaptureIRQHandler(CH4);
}
#endif


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
