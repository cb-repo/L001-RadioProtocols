/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PPM.h"

/*
 * PRIVATE DEFINITIONS
 */

#define PPM_EOF_TIME		4000
#define PPM_JITTER_ARRAY	3
#define PPM_THRESHOLD		500
#define PPM_TIMEOUT_CYCLES	3
#define PPM_TIMEOUT			(PPM_PERIOD * PPM_TIMEOUT_CYCLES)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

uint16_t PPM_Truncate (uint16_t);
void PPM_memset (void);
void PPM_IRQ (void);

/*
 * PRIVATE VARIABLES
 */

volatile uint16_t rxPPM[PPM_JITTER_ARRAY][PPM_NUM_CHANNELS] = {0};
volatile bool rxHeartbeatPPM = false;
PPM_Data dataPPM = {0};

/*
 * PUBLIC FUNCTIONS
 */

bool PPM_Detect (void)
{
	PPM_Init();

	uint32_t tick = CORE_GetTick();
	while ((PPM_TIMEOUT * 2) > CORE_GetTick() - tick)
	{
		CORE_Idle();
	}

	PPM_Deinit();

	return rxHeartbeatPPM;
}

void PPM_Init (void)
{
	PPM_memset();

	TIM_Init(PPM_TIM, PPM_TIM_FREQ, PPM_TIM_RELOAD);
	TIM_Start(PPM_TIM);

	GPIO_EnableInput(PPM_GPIO, PPM_PIN, GPIO_Pull_Down);
	GPIO_OnChange(PPM_GPIO, PPM_PIN, GPIO_IT_Rising, PPM_IRQ);
}

void PPM_Deinit (void)
{
	TIM_Deinit(PPM_TIM);

	GPIO_OnChange(PPM_GPIO, PPM_PIN, GPIO_IT_None, NULL);
	GPIO_Deinit(PPM_GPIO, PPM_PIN);
}

void PPM_Update (void)
{
	uint32_t now = CORE_GetTick();
	static uint32_t prev = 0;

	// Check for New Input Data
	if (rxHeartbeatPPM)
	{
		// Average and Assign Input to data Struct
		for (uint8_t j = 0; j < PPM_NUM_CHANNELS; j++)
		{
			uint8_t avg = 0;
			uint32_t ch = 0;
			for (uint8_t i = 0; i < PPM_JITTER_ARRAY; i++)
			{
				uint16_t trunc = PPM_Truncate(rxPPM[i][j]);
				if (trunc != 0)
				{
					ch += trunc;
					avg += 1;
				}
			}
			ch /= avg;
			dataPPM.ch[j] = ch;
		}
		rxHeartbeatPPM = false;
		prev = now;
	}

	// Check for Input Failsafe
	if (PPM_TIMEOUT <= (now - prev)) {
		dataPPM.inputLost = true;
		PPM_memset();
	} else {
		dataPPM.inputLost = false;
	}
}

PPM_Data* PPM_GetDataPtr (void)
{
	return &dataPPM;
}

/*
 * PRIVATE FUNCTIONS
 */

uint16_t PPM_Truncate (uint16_t r)
{
	uint16_t retVal = 0;

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

void PPM_memset (void)
{
	for (uint8_t j = 0; j < PPM_NUM_CHANNELS; j++)
	{
		for (uint8_t i = 0; i < PPM_JITTER_ARRAY; i++)
		{
			rxPPM[i][j] = 0;
		}
	}
}


/*
 * INTERRUPT ROUTINES
 */

void PPM_IRQ (void)
{
	uint16_t now = TIM_Read(PPM_TIM);	// Current IRQ Loop Time
	static uint16_t prev = 0;			// Previous IRQ Loop Time
	uint16_t pulse = 0;					// Pulse Width
	static uint8_t ch = 0;				// Channel Index
	static uint8_t jitter = 0;			// Jitter-Smoothing Array Index

	// Calculate the Pulse Width
	pulse = now - prev;

	// Check for Channel 1 Synchronization
	if (pulse > PPM_EOF_TIME)
	{
		ch = 0;
	}

	// Check for any Additional (Unsupported) Channels
	if (ch < PPM_NUM_CHANNELS)
	{
		// Assign Pulse to Correct Channel
		rxPPM[jitter][ch] = pulse;
		// Prep Variables for Next Loop
		prev = now;
		ch += 1;
		// Evaluate on last loop
		if (ch == (PPM_NUM_CHANNELS - 1))
		{
			// Kick Heartbeat
			rxHeartbeatPPM = true;
			//Increment Jitter Array Index
			jitter += 1;
			if (jitter >= PPM_JITTER_ARRAY) { jitter = 0; }
		}
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
