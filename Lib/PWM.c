/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PWM.h"

/*
 * PRIVATE DEFINITIONS
 */

#define PWM_EOF_TIME		4000
#define PWM_JITTER_ARRAY	3
#define PWM_THRESHOLD		100
#define PWM_TIMEOUT_CYCLES	3
#define PWM_TIMEOUT			(PWM_PERIOD * PWM_TIMEOUT_CYCLES)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

uint16_t PWM_Truncate (uint16_t);
void PWM_memset (void);
void PWM1_IRQ (void);
void PWM2_IRQ (void);
void PWM3_IRQ (void);
void PWM4_IRQ (void);

/*
 * PRIVATE VARIABLES
 */

volatile uint16_t rxPWM[PWM_JITTER_ARRAY][PWM_NUM_CHANNELS] = {0};
volatile bool rxHeartbeatPWM[PWM_NUM_CHANNELS] = {0};
PWM_Data dataPWM = {0};

/*
 * PUBLIC FUNCTIONS
 */

// TODO: Fix the false-positive when running PPM protocol
bool PWM_Detect(void)
{
	PWM_Init();

	uint32_t tick = CORE_GetTick();
	while ((PWM_TIMEOUT * 2) > CORE_GetTick() - tick)
	{
		CORE_Idle();
	}

	PWM_Deinit();

	bool retVal = false;
	for (uint8_t i = 0; i < PWM_NUM_CHANNELS; i++)
	{
		retVal = retVal || rxHeartbeatPWM[i];
	}
	return retVal;
}

void PWM_Init ()
{
	PWM_memset();

	TIM_Init(PWM_TIM, PWM_TIM_FREQ, PWM_TIM_RELOAD);
	TIM_Start(PWM_TIM);

	GPIO_EnableInput(PWM_S1_GPIO, PWM_S1_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S2_GPIO, PWM_S2_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S3_GPIO, PWM_S3_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S4_GPIO, PWM_S4_PIN, GPIO_Pull_Down);
	GPIO_OnChange(PWM_S1_GPIO, PWM_S1_PIN, GPIO_IT_Both, PWM1_IRQ);
	GPIO_OnChange(PWM_S2_GPIO, PWM_S2_PIN, GPIO_IT_Both, PWM2_IRQ);
	GPIO_OnChange(PWM_S3_GPIO, PWM_S3_PIN, GPIO_IT_Both, PWM3_IRQ);
	GPIO_OnChange(PWM_S4_GPIO, PWM_S4_PIN, GPIO_IT_Both, PWM4_IRQ);
}

void PWM_Deinit (void)
{
	TIM_Deinit(TIM_RADIO);

	GPIO_OnChange(PWM_S1_GPIO, PWM_S1_PIN, GPIO_IT_None, NULL);
	GPIO_OnChange(PWM_S2_GPIO, PWM_S2_PIN, GPIO_IT_None, NULL);
	GPIO_OnChange(PWM_S3_GPIO, PWM_S3_PIN, GPIO_IT_None, NULL);
	GPIO_OnChange(PWM_S4_GPIO, PWM_S4_PIN, GPIO_IT_None, NULL);
	GPIO_Deinit(PWM_S1_GPIO, PWM_S1_PIN);
	GPIO_Deinit(PWM_S2_GPIO, PWM_S2_PIN);
	GPIO_Deinit(PWM_S3_GPIO, PWM_S3_PIN);
	GPIO_Deinit(PWM_S4_GPIO, PWM_S4_PIN);
}

void PWM_Update (void)
{
	uint32_t now = CORE_GetTick();
	static uint32_t prev = 0;

	// Check for New Input Data
	if (rxHeartbeatPWM[0] || rxHeartbeatPWM [1] || rxHeartbeatPWM [2] || rxHeartbeatPWM [3])
	{
		// Average and Assign Input to data Struct
		for (uint8_t j = 0; j < PWM_NUM_CHANNELS; j++)
		{
			uint8_t avg = 0;
			uint32_t ch = 0;
			for (uint8_t i = 0; i < PWM_JITTER_ARRAY; i++)
			{
				uint16_t trunc = PWM_Truncate(rxPWM[i][j]);
				if (trunc != 0)
				{
					ch += trunc;
					avg += 1;
				}
			}
			ch /= avg;
			dataPWM.ch[j] = ch;
		}
		rxHeartbeatPWM[0] = false;
		rxHeartbeatPWM[1] = false;
		rxHeartbeatPWM[2] = false;
		rxHeartbeatPWM[3] = false;
		prev = now;
	}

	// Check for Input Failsafe
	if (PWM_TIMEOUT <= (now - prev)) {
		dataPWM.inputLost = true;
		PWM_memset();
	} else {
		dataPWM.inputLost = false;
	}
}

PWM_Data* PWM_GetDataPtr (void)
{
	return &dataPWM;
}

/*
 * PRIVATE FUNCTIONS
 */

uint16_t PWM_Truncate (uint16_t r)
{
	uint16_t retVal = 0;

	if (r == 0) {
		retVal = 0;
	} else if (r < (PWM_MIN - PWM_THRESHOLD)) {
		retVal = 0;
	} else if (r < PWM_MIN) {
		retVal = PWM_MIN;
	} else if (r <= PWM_MAX) {
		retVal = r;
	} else if (r < (PWM_MAX + PWM_THRESHOLD))	{
		retVal = PWM_MAX;
	} else {
		retVal = 0;
	}

	return retVal;
}

void PWM_memset (void)
{
	for (uint8_t j = 0; j < PWM_NUM_CHANNELS; j++)
	{
		for (uint8_t i = 0; i < PWM_JITTER_ARRAY; i++)
		{
			rxPWM[i][j] = 0;
		}
	}
}

/*
 * INTERRUPT ROUTINES
 */

void PWM1_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick = 0;
	static uint8_t jitter = 0;

	if (GPIO_Read(PWM_S1_GPIO, PWM_S1_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;
		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESHOLD) && pulse >= (PWM_MIN - PWM_THRESHOLD))
		{
			rxPWM[jitter][0] = pulse;
			rxHeartbeatPWM[0] = CORE_GetTick();
			jitter += 1;
		}
	}

	// Check for Jitter Array Index Reset
	if (jitter >= PWM_JITTER_ARRAY)
	{
		jitter = 0;
	}
}

void PWM2_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick = 0;
	static uint8_t jitter = 0;

	if (GPIO_Read(PWM_S2_GPIO, PWM_S2_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;

		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESHOLD) && pulse >= (PWM_MIN - PWM_THRESHOLD))
		{
			rxPWM[jitter][1] = pulse;
			rxHeartbeatPWM[1] = CORE_GetTick();
			jitter += 1;
		}
	}

	// Check for Jitter Array Index Reset
	if (jitter >= PWM_JITTER_ARRAY)
	{
		jitter = 0;
	}
}

void PWM3_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick = 0;
	static uint8_t jitter = 0;

	if (GPIO_Read(PWM_S3_GPIO, PWM_S3_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;

		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESHOLD) && pulse >= (PWM_MIN - PWM_THRESHOLD))
		{
			rxPWM[jitter][2] = pulse;
			rxHeartbeatPWM[2] = CORE_GetTick();
			jitter += 1;
		}
	}

	// Check for Jitter Array Index Reset
	if (jitter >= PWM_JITTER_ARRAY)
	{
		jitter = 0;
	}
}

void PWM4_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick = 0;
	static uint8_t jitter = 0;

	if (GPIO_Read(PWM_S4_GPIO, PWM_S4_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;

		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESHOLD) && pulse >= (PWM_MIN - PWM_THRESHOLD))
		{
			rxPWM[jitter][3] = pulse;
			rxHeartbeatPWM[3] = CORE_GetTick();
			jitter += 1;
		}
	}

	// Check for Jitter Array Index Reset
	if (jitter >= PWM_JITTER_ARRAY)
	{
		jitter = 0;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
