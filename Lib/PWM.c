/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PWM.h"

/*
 * PRIVATE DEFINITIONS
 */

#define PWM_EOF_TIME			4000
#define PWM_TIMEOUT_CYCLES		3
#define PWM_TIMEOUT				(PWM_PERIOD_MS * PWM_TIMEOUT_CYCLES)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

static uint32_t	PWM_Truncate	( uint32_t );
static void 	PWM_resetArrays	( void );

static void RADIO_S1_IRQ ( void );
#if PWM_NUM_CHANNELS >= 2
static void RADIO_S2_IRQ ( void );
#endif
#if PWM_NUM_CHANNELS >= 3
static void RADIO_S3_IRQ ( void );
#endif
#if PWM_NUM_CHANNELS >= 4
static void RADIO_S4_IRQ ( void );
#endif
#if PWM_NUM_CHANNELS > 4
#error // Too Many Channels Defined
#endif

/*
 * PRIVATE VARIABLES
 */

static volatile uint32_t	rx[PWM_NUM_CHANNELS];
static volatile bool 		rxHeartbeat[PWM_NUM_CHANNELS];

PWM_Data dataPWM;

/*
 * PUBLIC FUNCTIONS
 */

bool PWM_DetInit(void)
{
	// Init Loop Variables
	bool retVal = false;
	uint32_t tick = CORE_GetTick();

	// Run PWM Detect
	PWM_Init();
	while ((PWM_TIMEOUT * 2) > CORE_GetTick() - tick) {
		CORE_Idle();
	}

	// Consolidate Heartbeats to Single retVal
	for (uint8_t ch = 0; ch < PWM_NUM_CHANNELS; ch++) {
		retVal |= rxHeartbeat[ch];
	}

	if ( !retVal )
	{
		PWM_Deinit();
	}

	return retVal;
}

void PWM_Init ()
{
	// Zero Channel Data Array
	PWM_resetArrays();
	// Assume No Radio Signal on Init
	dataPWM.inputLost = true;

	// Start Timer to Measure Pulse Widths
	TIM_Init(TIM_RADIO, TIM_RADIO_FREQ, TIM_RADIO_RELOAD);
	TIM_Start(TIM_RADIO);

	// Enable All Radio GPIO As Inputs
	GPIO_EnableInput(RADIO_PWM1_Pin, GPIO_Pull_Down);
	// Assign IRQ For Each Input
	GPIO_OnChange(RADIO_PWM1_Pin, GPIO_IT_Both, RADIO_S1_IRQ);
	// Assume No Radio Signal On Init
	dataPWM.inputLostCh[0] = true;

#if PWM_NUM_CHANNELS >= 2
	GPIO_EnableInput(RADIO_PWM2_Pin, GPIO_Pull_Down);
	GPIO_OnChange(RADIO_PWM2_Pin, GPIO_IT_Both, RADIO_S2_IRQ);
	dataPWM.inputLostCh[1] = true;
#endif

#if PWM_NUM_CHANNELS >= 3
	GPIO_EnableInput(RADIO_PWM3_Pin, GPIO_Pull_Down);
	GPIO_OnChange(RADIO_PWM3_Pin, GPIO_IT_Both, RADIO_S3_IRQ);
	dataPWM.inputLostCh[2] = true;
#endif

#if PWM_NUM_CHANNELS >= 4
	GPIO_EnableInput(RADIO_PWM4_Pin, GPIO_Pull_Down);
	GPIO_OnChange(RADIO_PWM4_Pin, GPIO_IT_Both, RADIO_S4_IRQ);
	dataPWM.inputLostCh[3] = true;
#endif
}

void PWM_Deinit (void)
{
	// Stop and Deinitialise the Radio Timer
	TIM_Deinit(TIM_RADIO);

	// Unassign IRQ for Each Radio Input
	GPIO_OnChange(RADIO_PWM1_Pin, GPIO_IT_None, NULL);
	// De-Initialise Radio Input GPIO
	GPIO_Deinit(RADIO_PWM1_Pin);

#if PWM_NUM_CHANNELS >= 2
	GPIO_OnChange(RADIO_PWM2_Pin, GPIO_IT_None, NULL);
	GPIO_Deinit(RADIO_PWM2_Pin);
#endif

#if PWM_NUM_CHANNELS >= 3
	GPIO_OnChange(RADIO_PWM3_Pin, GPIO_IT_None, NULL);
	GPIO_Deinit(RADIO_PWM3_Pin);
#endif

#if PWM_NUM_CHANNELS >= 4
	GPIO_OnChange(RADIO_PWM4_Pin, GPIO_IT_None, NULL);
	GPIO_Deinit(RADIO_PWM4_Pin);
#endif
}

void PWM_Update (void)
{
	// Init Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t tick[PWM_NUM_CHANNELS] = {0};

	// Iterate through each input
	for ( uint8_t ch = 0; ch < PWM_NUM_CHANNELS; ch++ )
	{
		if ( rxHeartbeat[ch] ) {
			// Assign Data to Array
			dataPWM.ch[ch] = PWM_Truncate(rx[ch]);
			// Reset Flags
			rxHeartbeat[ch] = false;
			dataPWM.inputLostCh[ch] = false;
			tick[ch] = now;
		}

		if ( !dataPWM.inputLostCh[ch] && PWM_TIMEOUT <= (now - tick[ch]) )
		{
			// Trigger InputLost Flag
			dataPWM.inputLostCh[ch] = true;
			//Reset Channel Data
			dataPWM.ch[ch] = 0;
		}
	}

	//
	for (uint8_t ch = 0; ch < PWM_NUM_CHANNELS; ch++)
	{
		if (dataPWM.inputLostCh[ch]) {
			dataPWM.inputLost = true;
			break;
		}
		if (ch == (PWM_NUM_CHANNELS - 1)) {
			dataPWM.inputLost = false;
		}
	}
}

PWM_Data* PWM_GetDataPtr (void)
{
	return &dataPWM;
}

/*
 * PRIVATE FUNCTIONS
 */

static uint32_t PWM_Truncate (uint32_t r)
{
	uint32_t retVal = 0;

	if (r == 0) {
		retVal = 0;
	} else if (r < (PWM_MIN - PWM_THRESHOLD_PULSE)) {
		retVal = 0;
	} else if (r <= PWM_MIN) {
		retVal = PWM_MIN;
	} else if (r <= PWM_MAX) {
		retVal = r;
	} else if (r < (PWM_MAX + PWM_THRESHOLD_PULSE))	{
		retVal = PWM_MAX;
	} else {
		retVal = 0;
	}

	return retVal;
}

static void PWM_resetArrays (void)
{
	for (uint8_t ch = 0; ch < PWM_NUM_CHANNELS; ch++)
	{
		rx[ch] = 0;
		rxHeartbeat[ch] = false;
		dataPWM.inputLostCh[ch] = true;
	}
}



/*
 * INTERRUPT ROUTINES
 */

#if defined(RADIO_PWM1_Pin)
static void RADIO_S1_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(RADIO_PWM1_Pin))
	{
		// Assign Variables for Next Loop
		tickHigh = now;
	}
	else
	{
		//
		period = now - tickLow;
		//
		tickLow = now;
		// Calculate Signal Data
		pulse = now - tickHigh;
		// Check Pulse is Valid
		if ( pulse <= (PWM_MAX + PWM_THRESHOLD_PULSE) &&
			 pulse >= (PWM_MIN - PWM_THRESHOLD_PULSE) &&
			 period <= (PWM_PERIOD_US + PWM_THRESHOLD_PERIOD) &&
			 period >= (PWM_PERIOD_US - PWM_THRESHOLD_PERIOD) ) {
			// Assign Pulse to Data Array
			rx[PWM_CH1] = pulse;
			// Trigger New Data Flag
			rxHeartbeat[PWM_CH1] = true;
		}
	}
}
#endif

#if defined(RADIO_PWM2_Pin)
static void RADIO_S2_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(RADIO_PWM2_Pin))
	{
		// Assign Variables for Next Loop
		tickHigh = now;
	}
	else
	{
		//
		period = now - tickLow;
		//
		tickLow = now;
		// Calculate Signal Data
		pulse = now - tickHigh;
		// Check Pulse is Valid
		if ( pulse <= (PWM_MAX + PWM_THRESHOLD_PULSE) &&
			 pulse >= (PWM_MIN - PWM_THRESHOLD_PULSE) &&
			 period <= (PWM_PERIOD_US + PWM_THRESHOLD_PERIOD) &&
			 period >= (PWM_PERIOD_US - PWM_THRESHOLD_PERIOD) ) {
			// Assign Pulse to Data Array
			rx[PWM_CH2] = pulse;
			// Trigger New Data Flag
			rxHeartbeat[PWM_CH2] = true;
		}
	}
}
#endif

#if defined(RADIO_PWM3_Pin)
static void RADIO_S3_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(RADIO_PWM3_Pin))
	{
		// Assign Variables for Next Loop
		tickHigh = now;
	}
	else
	{
		//
		period = now - tickLow;
		//
		tickLow = now;
		// Calculate Signal Data
		pulse = now - tickHigh;
		// Check Pulse is Valid
		if ( pulse <= (PWM_MAX + PWM_THRESHOLD_PULSE) &&
			 pulse >= (PWM_MIN - PWM_THRESHOLD_PULSE) &&
			 period <= (PWM_PERIOD_US + PWM_THRESHOLD_PERIOD) &&
			 period >= (PWM_PERIOD_US - PWM_THRESHOLD_PERIOD) ) {
			// Assign Pulse to Data Array
			rx[PWM_CH3] = pulse;
			// Trigger New Data Flag
			rxHeartbeat[PWM_CH3] = true;
		}
	}
}
#endif

#if defined(RADIO_PWM4_Pin)
static void RADIO_S4_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(RADIO_PWM4_Pin))
	{
		// Assign Variables for Next Loop
		tickHigh = now;
	}
	else
	{
		//
		period = now - tickLow;
		//
		tickLow = now;
		// Calculate Signal Data
		pulse = now - tickHigh;
		// Check Pulse is Valid
		if ( pulse <= (PWM_MAX + PWM_THRESHOLD_PULSE) &&
			 pulse >= (PWM_MIN - PWM_THRESHOLD_PULSE) &&
			 period <= (PWM_PERIOD_US + PWM_THRESHOLD_PERIOD) &&
			 period >= (PWM_PERIOD_US - PWM_THRESHOLD_PERIOD) ) {
			// Assign Pulse to Data Array
			rx[PWM_CH4] = pulse;
			// Trigger New Data Flag
			rxHeartbeat[PWM_CH4] = true;
		}
	}
}
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
