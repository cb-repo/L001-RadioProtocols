/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PWM.h"

/*
 * PRIVATE DEFINITIONS
 */

#define PWM_EOF_TIME			4000
#define PWM_JITTER_ARRAY		3
#define PWM_THRESHOLD_PULSE		100		// [us]
#define PWM_THRESHOLD_PERIOD	1000	// [us]
#define PWM_TIMEOUT_CYCLES		3
#define PWM_TIMEOUT				(PWM_PERIOD_MS * PWM_TIMEOUT_CYCLES)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

uint32_t	PWM_Truncate	( uint32_t );
void 		PWM_memset 		( void );

void 		PWM1_IRQ 		( void );
void 		PWM2_IRQ 		( void );
void 		PWM3_IRQ 		( void );
void 		PWM4_IRQ 		( void );

/*
 * PRIVATE VARIABLES
 */

volatile uint32_t rxPWM[PWM_NUM_CHANNELS] = {0};
volatile bool rxHeartbeatPWM[PWM_NUM_CHANNELS] = {0};
PWM_Data dataPWM = {0};

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
		retVal |= rxHeartbeatPWM[ch];
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
	PWM_memset();
	// Assume No Radio Signal on Init
	dataPWM.inputLost = true;
	for (uint8_t ch = 0; ch < PWM_NUM_CHANNELS; ch++) {
		rxHeartbeatPWM[ch] = false;
		dataPWM.inputLostCh[ch] = true;
	}

	// Start Timer to Measure Pulsewidths
	TIM_Init(PWM_TIM, PWM_TIM_FREQ, PWM_TIM_RELOAD);
	TIM_Start(PWM_TIM);
	// Enable All Radio GPIO As Inputs
	GPIO_EnableInput(PWM_S1_GPIO, PWM_S1_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S2_GPIO, PWM_S2_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S3_GPIO, PWM_S3_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S4_GPIO, PWM_S4_PIN, GPIO_Pull_Down);
	// Assign IRQ For Each Input
	GPIO_OnChange(PWM_S1_GPIO, PWM_S1_PIN, GPIO_IT_Both, PWM1_IRQ);
	GPIO_OnChange(PWM_S2_GPIO, PWM_S2_PIN, GPIO_IT_Both, PWM2_IRQ);
	GPIO_OnChange(PWM_S3_GPIO, PWM_S3_PIN, GPIO_IT_Both, PWM3_IRQ);
	GPIO_OnChange(PWM_S4_GPIO, PWM_S4_PIN, GPIO_IT_Both, PWM4_IRQ);
}

void PWM_Deinit (void)
{
	// Stop and Deinitialise the Radio Timer
	TIM_Deinit(TIM_RADIO);
	// Unassign IRQ for Each Radio Input
	GPIO_OnChange(PWM_S1_GPIO, PWM_S1_PIN, GPIO_IT_None, NULL);
	GPIO_OnChange(PWM_S2_GPIO, PWM_S2_PIN, GPIO_IT_None, NULL);
	GPIO_OnChange(PWM_S3_GPIO, PWM_S3_PIN, GPIO_IT_None, NULL);
	GPIO_OnChange(PWM_S4_GPIO, PWM_S4_PIN, GPIO_IT_None, NULL);
	// De-Initialise Radio Input GPIO
	GPIO_Deinit(PWM_S1_GPIO, PWM_S1_PIN);
	GPIO_Deinit(PWM_S2_GPIO, PWM_S2_PIN);
	GPIO_Deinit(PWM_S3_GPIO, PWM_S3_PIN);
	GPIO_Deinit(PWM_S4_GPIO, PWM_S4_PIN);
}

void PWM_Update (void)
{
	// Init Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t tick[PWM_NUM_CHANNELS] = {0};

	// Iterate through each input
	for (uint8_t ch = 0; ch < PWM_NUM_CHANNELS; ch++)
	{

		if (rxHeartbeatPWM[ch])
		{
			// Assign Data to Array
			dataPWM.ch[ch] = PWM_Truncate(rxPWM[ch]);
			// Reset Flags
			rxHeartbeatPWM[ch] = false;
			dataPWM.inputLostCh[ch] = false;
			tick[ch] = now;
		}

		if (!dataPWM.inputLostCh[ch] && PWM_TIMEOUT <= (now - tick[ch]))
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

uint32_t PWM_Truncate (uint32_t r)
{
	uint32_t retVal = 0;

	if (r == 0) {
		retVal = 0;
	} else if (r < (PWM_MIN - PWM_THRESHOLD_PULSE)) {
		retVal = 0;
	} else if (r < PWM_MIN) {
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

void PWM_memset (void)
{
	for (uint8_t ch = 0; ch < PWM_NUM_CHANNELS; ch++)
	{
		rxPWM[ch] = 0;
	}
}



/*
 * INTERRUPT ROUTINES
 */

void PWM1_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(PWM_S1_GPIO, PWM_S1_PIN))
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
			rxPWM[PWM_CH1] = pulse;
			// Trigger New Data Flag
			rxHeartbeatPWM[PWM_CH1] = true;
		}
	}
}

void PWM2_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(PWM_S2_GPIO, PWM_S2_PIN))
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
			rxPWM[PWM_CH2] = pulse;
			// Trigger New Data Flag
			rxHeartbeatPWM[PWM_CH2] = true;
		}
	}
}

void PWM3_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(PWM_S3_GPIO, PWM_S3_PIN))
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
			rxPWM[PWM_CH3] = pulse;
			// Trigger New Data Flag
			rxHeartbeatPWM[PWM_CH3] = true;
		}
	}
}

void PWM4_IRQ (void)
{
	// Init Loop Variables
	uint32_t now = TIM_Read(TIM_RADIO);
	uint32_t pulse = 0;
	uint32_t period = 0;
	static uint32_t tickHigh = 0;
	static uint32_t tickLow = 0;

	if (GPIO_Read(PWM_S4_GPIO, PWM_S4_PIN))
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
			rxPWM[PWM_CH4] = pulse;
			// Trigger New Data Flag
			rxHeartbeatPWM[PWM_CH4] = true;
		}
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
