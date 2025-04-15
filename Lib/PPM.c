
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PPM.h"

#if defined(RADIO_USE_PPM)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#define PPM_EOF_TIME		4000
#define PPM_THRESHOLD		100
#define PPM_TIMEOUT_CYCLES	3
#define PPM_TIMEOUT			(PPM_PERIOD * PPM_TIMEOUT_CYCLES)


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static uint32_t	PPM_Truncate	( uint32_t );
static void 	PPM_resetArrays	( void );

static void 	PPM_CH_IRQ	( void );


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


volatile uint16_t rxPPM[PPM_CH_NUM] = {0};
volatile bool rxHeartbeatPPM = false;
PPM_Data dataPPM = {0};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
void PPM_Init ( void )
{
	PPM_resetArrays();
	rxHeartbeatPPM = false;
	dataPPM.inputLost = true;

	TIM_Init(TIM_RADIO, TIM_RADIO_FREQ, TIM_RADIO_RELOAD);
	TIM_Start(TIM_RADIO);

	GPIO_EnableInput(PPM_CH_Pin, GPIO_Pull_Down);
	GPIO_OnChange(PPM_CH_Pin, GPIO_IT_Rising, PPM_CH_IRQ);
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
void PPM_Deinit ( void )
{
	TIM_Deinit(TIM_RADIO);

	GPIO_OnChange(PPM_CH_Pin, GPIO_IT_None, NULL);
	GPIO_Deinit(PPM_CH_Pin);
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
bool PPM_DetInit ( void )
{
	PPM_Init();

	uint32_t tick = CORE_GetTick();
	while ((PPM_TIMEOUT * 2) > CORE_GetTick() - tick)
	{
		CORE_Idle();
	}

	if ( !rxHeartbeatPPM )
	{
		PPM_Deinit();
	}

	return rxHeartbeatPPM;
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
void PPM_Update ( void )
{
	// Init Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t prev = 0;
	static uint32_t ch_p[PPM_CH_NUM];
	// Check for New Input Data
	if (rxHeartbeatPPM)
	{
		// Assign Input to data Struct
		for (uint8_t i = 0; i < PPM_CH_NUM; i++)
		{
			ch_p[i] = dataPPM.ch[i];
			dataPPM.ch[i] = PPM_Truncate(rxPPM[i]);
		}

		// Reset Flags
		rxHeartbeatPPM = false;
		dataPPM.inputLost = false;
		prev = now;
	}

	// Assign Input to data Struct
	for (uint8_t i = 0; i < PPM_CH_NUM; i++)
	{
		if ( dataPPM.ch[i] != 0 &&
			 ((dataPPM.ch[i] >= (ch_p[i] + 20)) ||
			 (dataPPM.ch[i] <= (ch_p[i] - 20))))
		{
			rxHeartbeatPPM = false;
		}
	}

	// Check for Input Failsafe
	if (!dataPPM.inputLost && PPM_TIMEOUT <= (now - prev)) {
		dataPPM.inputLost = true;
		PPM_resetArrays();
	}
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
PPM_Data* PPM_GetDataPtr ( void )
{
	return &dataPPM;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
static uint32_t PPM_Truncate ( uint32_t r )
{
	uint32_t retVal = 0;

	if (r == 0) {
		retVal = 0;
	} else if (r < (PPM_MIN - PPM_THRESHOLD)) {
		retVal = 0;
	} else if (r < PPM_MIN) {
		retVal = PPM_MIN;
	} else if (r <= PPM_MAX) {
		retVal = r;
	} else if (r < (PPM_MAX + PPM_THRESHOLD))	{
		retVal = PPM_MAX;
	} else {
		retVal = 0;
	}

	return retVal;
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
static void PPM_resetArrays ( void )
{
	for (uint8_t i = 0; i < PPM_CH_NUM; i++)
	{
		rxPPM[i] = 0;
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
static void PPM_CH_IRQ ( void )
{
	uint32_t now = TIM_Read(TIM_RADIO);	// Current IRQ Loop Time
	uint32_t pulse = 0;					// Pulse Width
	static uint32_t tick = 0;			// Previous IRQ Loop Time
	static uint8_t ch = 0;				// Channel Index
	static bool sync = false;			// Sync Flag to Indicate Start of Transmission

	// Calculate the Pulse Width
	pulse = now - tick;

	// Check for Channel 1 Synchronization
	if (pulse > PPM_EOF_TIME)
	{
		ch = 0;
		sync = true;
	}
	// Assign Pulse to Channel
	else if (sync)
	{
		// Check for valid pulse
		if (pulse <= (PPM_MAX + PPM_THRESHOLD) && pulse >= (PPM_MIN - PPM_THRESHOLD)) {
			rxPPM[ch] = pulse;
			ch += 1;
		} else { // Pulse train is corrupted. Abort transmission.
			sync = false;
		}
		// If on Last Channel
		if (ch >= PPM_CH_NUM)
		{
			rxHeartbeatPPM = true;
			sync = false;
		}

	}

	// Set variables for next loop
	tick = now;

}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
