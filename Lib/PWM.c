
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


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static void PWM_Process ( RADIO_chIndex );

static void 	PWM_CH1_IRQ 	( void );
#if PWM_CH_NUM >= 2
static void 	PWM_CH2_IRQ 	( void );
#endif
#if PWM_CH_NUM >= 3
static void 	PWM_CH3_IRQ 	( void );
#endif
#if PWM_CH_NUM >= 4
static void 	PWM_CH4_IRQ 	( void );
#endif
#if PWM_CH_NUM > 4
#error // Too Many Channels Defined
#endif


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static volatile uint32_t rx[ PWM_CH_NUM ];
RADIO_dataModule dataPWM;


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
		dataPWM.ch[ch] = 0;
		dataPWM.chActive[ch] = chOFF;
		dataPWM.chFault[ch] = true;
		dataPWM.allFault = true;
		dataPWM.anyFault = true;
	}

	// START TIMER TO MEASURE PULSE WIDTHS
	TIM_Init(  TIM_RADIO, TIM_RADIO_FREQ, TIM_RADIO_RELOAD );
	TIM_Start( TIM_RADIO );

	// SETUP GPIO AS INPUTS AND ASSIGN IRQ
	GPIO_EnableInput(	PWM_CH1_Pin, GPIO_Pull_Down);
	GPIO_OnChange(		PWM_CH1_Pin, GPIO_IT_Both, PWM_CH1_IRQ );
	#if PWM_CH_NUM >= 2
	GPIO_EnableInput(	PWM_CH2_Pin, GPIO_Pull_Down);
	GPIO_OnChange(		PWM_CH2_Pin, GPIO_IT_Both, PWM_CH2_IRQ );
	#endif
	#if PWM_CH_NUM >= 3
	GPIO_EnableInput(	PWM_CH3_Pin, GPIO_Pull_Down);
	GPIO_OnChange(		PWM_CH3_Pin, GPIO_IT_Both, PWM_CH3_IRQ );
	#endif
	#if PWM_CH_NUM >= 4
	GPIO_EnableInput(	PWM_CH4_Pin, GPIO_Pull_Down);
	GPIO_OnChange(		PWM_CH4_Pin, GPIO_IT_Both, PWM_CH4_IRQ );
	#endif

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
	GPIO_OnChange(	PWM_CH1_Pin, GPIO_IT_None, NULL );
	GPIO_Deinit(	PWM_CH1_Pin );
	#if PWM_CH_NUM >= 2
	GPIO_OnChange(	PWM_CH2_Pin, GPIO_IT_None, NULL );
	GPIO_Deinit(	PWM_CH2_Pin );
	#endif
	#if PWM_CH_NUM >= 3
	GPIO_OnChange(	PWM_CH3_Pin, GPIO_IT_None, NULL );
	GPIO_Deinit(	PWM_CH3_Pin );
	#endif
	#if PWM_CH_NUM >= 4
	GPIO_OnChange(	PWM_CH4_Pin, GPIO_IT_None, NULL );
	GPIO_Deinit(	PWM_CH4_Pin );
	#endif
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
	bool retVal = false;

	// RUN PWM PROTOCOL DETECTION
	PWM_Init();
	while ( PWM_DETECT_MS > (CORE_GetTick() - tick) )
	{
		PWM_Update();
		if ( !dataPWM.allFault ) {
			retVal = true;
			break;
		}
		CORE_Idle();
	}

	// DEINITIALISE IF NO RADIO DETECTED
	if ( !retVal ) 	{
		PWM_Deinit();
	}

	//
	return retVal;
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
	static uint32_t tick[ PWM_CH_NUM ] = { 0 };
	static uint8_t 	validData[ PWM_CH_NUM ] = { 0 };

	uint32_t now = CORE_GetTick();

	// ITTERATE THROUGH EACH CHANNEL
	for ( uint8_t ch = CH1; ch < PWM_CH_NUM; ch++ )
	{
		// STATE - TIMEDOUT (OR STARTUP)
		if ( dataPWM.chFault[ch] )
		{
			// CHECK FOR DATA IN THE RECIEVE BUFFER
			if ( rx[ch] )
			{
				// INCREMENT VALID DATA COUNTER
				validData[ch] += 1;
				// HAVE WE REACHED TIME IN CONDITION
				if ( validData[ch] >= PWM_TIMEIN_CYCLES ) {
					// RESET FAULT FLAG AND PROCEED TO NORMAL OPERATION - DONT RESET RX ARRAY
					dataPWM.chFault[ch] = false;
				} else {
					// RESET DATA FOR NEXT LOOP
					rx[ch] = 0;
					tick[ch] = now;
				}
			}
			// CHECK FOR TIMEIN COUNT RESET
			else if ( validData[ch] && ((now - tick[ch]) >= PWM_PERIOD_MAX_MS) )
			{
				validData[ch] = 0;
				tick[ch] = now;
			}
		}

		// STATE - NORMAL OPERATION
		if ( !dataPWM.chFault[ch] )
		{
			// CHECK FOR NEW DATA
			if ( rx[ch] )
			{
				// PROCESS DATA
				PWM_Process(ch);
				// RESET RELEVANT FLAGS
				tick[ch] = now;
			}

			// CHECK FOR TIMEOUT CONDITION
			else if ((now - tick[ch]) >= PWM_TIMEOUT_MS)
			{
				// SET RELEVANT FLAGS
				dataPWM.chFault[ch] = true;
				dataPWM.ch[ch] = 0;
				dataPWM.chActive[ch] = chOFF;
				tick[ch] = now;
				validData[ch] = 0;
			}
		}
	}

	//
	#if PWM_CH_NUM >= 4
	dataPWM.allFault = dataPWM.chFault[CH1] && dataPWM.chFault[CH2] && dataPWM.chFault[CH3] && dataPWM.chFault[CH4];
	dataPWM.anyFault = dataPWM.chFault[CH1] || dataPWM.chFault[CH2] || dataPWM.chFault[CH3] || dataPWM.chFault[CH4];
	#elif PWM_CH_NUM >= 3
	dataPWM.allFault = dataPWM.chFault[CH1] && dataPWM.chFault[CH2] && dataPWM.chFault[CH3];
	dataPWM.anyFault = dataPWM.chFault[CH1] || dataPWM.chFault[CH2] || dataPWM.chFault[CH3];
	#elif PWM_CH_NUM >= 2
	dataPWM.allFault = dataPWM.chFault[CH1] && dataPWM.chFault[CH2];
	dataPWM.anyFault = dataPWM.chFault[CH1] || dataPWM.chFault[CH2];
	#else
	dataPWM.allFault = dataPWM.chFault[CH1];
	dataPWM.anyFault = dataPWM.chFault[CH1];
	#endif

}


/*
 * RETRIEVED A POINTER TO THE DATA STRUCTURE CONTAINING ALL PWM MODULE DATA
 *
 * INPUTS: n/a
 * OUTPUTS: Pointer of type RADIO_data
 */
RADIO_dataModule* PWM_getDataPtr ( void )
{
	return &dataPWM;
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
	uint32_t r = rx[c];
	rx[c] = 0;

	// TRUNCATE RADIO DATA AND MOVE TO OUTBOUND ARRAY
	// WE ALREADY KNOW DATA IS GREATER THAN RADIO_CH_ABSMIN AND SMALLER THAN RADIO_CH_ABSMAX
	if ( r < RADIO_CH_MIN ) {
		dataPWM.ch[c] = RADIO_CH_MIN;
	}
	else if ( r > RADIO_CH_MAX ) {
		dataPWM.ch[c] = RADIO_CH_MIN;
	}
	else {
		dataPWM.ch[c] = r;
	}

	// UPDATE CHANNEL'S ACTIVE FLAG
	if ( r > RADIO_CH_CENTERMAX ) {
		dataPWM.chActive[c] = chFWD;
	}
	else if ( r < RADIO_CH_CENTERMIN ) {
		dataPWM.chActive[c] = chRVS;
	}
	else {
		dataPWM.chActive[c] = chOFF;
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS   									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static void PWM_CH1_IRQ ( void )
{
	// INITIALISE LOOP VARIABLES
	uint32_t 		now 		= TIM_Read(TIM_RADIO);
	bool 			pos 		= GPIO_Read(PWM_CH1_Pin);
	static bool 	pos_p 		= false;
	static uint32_t	tickHigh 	= 0;
	static uint32_t tickLow 	= 0;

	// IGNORE NOISE ON SIGNAL I/P THAT RETURNS FASTER THAN INTERRRUPT SERVICE
	if ( pos_p != pos )
	{
		// RISING EDGE PULSE DETECTED
		if ( pos ) {
			// ASSIGN VARIABLES TO USE ON PULSE LOW
			tickHigh = now;
		}
		// FALLING EDGE PULSE DETECTED
		else {
			// CALCULATE SIGNAL PERIOD AND PULSE WIDTH
			uint32_t period = now - tickLow;
			uint32_t pulse = now - tickHigh;
			// CHECK SIGNAL IS VALID
			if ( pulse <= RADIO_CH_ABSMAX 		&& pulse >= RADIO_CH_ABSMIN &&
				 period <= PWM_PERIOD_MAX_US	&& period >= PWM_PERIOD_MIN_US )
			{
				// ASSIGN PULSE TO TEMP DATA ARRAY
				rx[CH1] = pulse;
			}
			// UPDATE VARIABLES FOR NEXT LOOP
			tickLow = now;
		}
	}

	//
	pos_p = pos;
}


#if PWM_CH_NUM >= 2
static void PWM_CH2_IRQ ( void )
{
	uint32_t 		now 		= TIM_Read(TIM_RADIO);
	bool 			pos 		= GPIO_Read(PWM_CH2_Pin);
	static bool 	pos_p 		= false;
	static uint32_t	tickHigh 	= 0;
	static uint32_t tickLow 	= 0;

	if ( pos_p != pos ) {
		if ( pos ) {
			tickHigh = now;
		} else {
			uint32_t period = now - tickLow;
			uint32_t pulse = now - tickHigh;
			if ( pulse <= RADIO_CH_ABSMAX 		&& pulse >= RADIO_CH_ABSMIN &&
				 period <= PWM_PERIOD_MAX_US	&& period >= PWM_PERIOD_MIN_US )
			{
				rx[CH2] = pulse;
			}
			tickLow = now;
		}
	}
	pos_p = pos;
}
#endif


#if PWM_CH_NUM >= 3
static void PWM_CH3_IRQ ( void )
{
	uint32_t 		now 		= TIM_Read(TIM_RADIO);
	bool 			pos 		= GPIO_Read(PWM_CH3_Pin);
	static bool 	pos_p 		= false;
	static uint32_t	tickHigh 	= 0;
	static uint32_t tickLow 	= 0;

	if ( pos_p != pos ) {
		if ( pos ) {
			tickHigh = now;
		} else {
			uint32_t period = now - tickLow;
			uint32_t pulse = now - tickHigh;
			if ( pulse <= RADIO_CH_ABSMAX 		&& pulse >= RADIO_CH_ABSMIN &&
				 period <= PWM_PERIOD_MAX_US	&& period >= PWM_PERIOD_MIN_US )
			{
				rx[CH3] = pulse;
			}
			tickLow = now;
		}
	}
	pos_p = pos;
}
#endif


#if PWM_CH_NUM >= 4
static void PWM_CH4_IRQ ( void )
{
	uint32_t 		now 		= TIM_Read(TIM_RADIO);
	bool 			pos 		= GPIO_Read(PWM_CH4_Pin);
	static bool 	pos_p 		= false;
	static uint32_t	tickHigh 	= 0;
	static uint32_t tickLow 	= 0;

	if ( pos_p != pos ) {
		if ( pos ) {
			tickHigh = now;
		} else {
			uint32_t period = now - tickLow;
			uint32_t pulse = now - tickHigh;
			if ( pulse <= RADIO_CH_ABSMAX 		&& pulse >= RADIO_CH_ABSMIN &&
				 period <= PWM_PERIOD_MAX_US	&& period >= PWM_PERIOD_MIN_US )
			{
				rx[CH4] = pulse;
			}
			tickLow = now;
		}
	}
	pos_p = pos;
}
#endif


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
