/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "PWM.h"
#include "Core.h"
#include "GPIO.h"
#include "TIM.h"

/*
 * PRIVATE DEFINITIONS
 */

#define PWM_THRESH		500

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

void RADIO_PWM1_IRQ (void);
void RADIO_PWM2_IRQ (void);
void RADIO_PWM3_IRQ (void);
void RADIO_PWM4_IRQ (void);

/*
 * PRIVATE VARIABLES
 */

volatile uint16_t rx[PWM_NUM_CHANNELS] = {0};
volatile uint32_t rxHeartbeat = 0;
PWM_Data data = {0};

//volatile uint16_t input[NUM_TOTALINPUTS] = {RADIO_CENTER, RADIO_CENTER, RADIO_CENTER, RADIO_CENTER};


/*
 * PUBLIC FUNCTIONS
 */

void PWM_Init (void)
{
	for (uint8_t i = 0; i < PWM_NUM_CHANNELS; i++)
	{
		rx[i] = PWM_CENTER;
	}

	TIM_Init(TIM_RADIO, TIM_RADIO_FREQ, TIM_RADIO_RELOAD);
	TIM_Start(TIM_RADIO);

	GPIO_EnableInput(PWM_S1_GPIO, PWM_S1_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S2_GPIO, PWM_S2_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S3_GPIO, PWM_S3_PIN, GPIO_Pull_Down);
	GPIO_EnableInput(PWM_S4_GPIO, PWM_S4_PIN, GPIO_Pull_Down);
	GPIO_OnChange(PWM_S1_GPIO, PWM_S1_PIN, GPIO_IT_Both, RADIO_PWM1_IRQ);
	GPIO_OnChange(PWM_S2_GPIO, PWM_S2_PIN, GPIO_IT_Both, RADIO_PWM2_IRQ);
	GPIO_OnChange(PWM_S3_GPIO, PWM_S3_PIN, GPIO_IT_Both, RADIO_PWM3_IRQ);
	GPIO_OnChange(PWM_S4_GPIO, PWM_S4_PIN, GPIO_IT_Both, RADIO_PWM4_IRQ);
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
	for (uint8_t i = 0; i < PWM_NUM_CHANNELS; i++)
	{
		data.ch[i] = rx[i];
	}
	// Check for failsafe + framelost
}

PWM_Data* PWM_GetDataPtr (void)
{
	return &data;
}

/*
 * PRIVATE FUNCTIONS
 */


/*
 * INTERRUPT ROUTINES
 */

void RADIO_PWM1_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick;

	if (GPIO_Read(PWM_S1_GPIO, PWM_S1_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;
		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESH) && pulse >= (PWM_MIN - PWM_THRESH))
		{
			rx[0] = pulse;
			rxHeartbeat = CORE_GetTick();
		}
	}
}

void RADIO_PWM2_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick;

	if (GPIO_Read(PWM_S2_GPIO, PWM_S2_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;

		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESH) && pulse >= (PWM_MIN - PWM_THRESH))
		{
			rx[1] = pulse;
			rxHeartbeat = CORE_GetTick();
		}
	}
}

void RADIO_PWM3_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick;

	if (GPIO_Read(PWM_S3_GPIO, PWM_S3_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;

		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESH) && pulse >= (PWM_MIN - PWM_THRESH))
		{
			rx[2] = pulse;
			rxHeartbeat = CORE_GetTick();
		}
	}
}

void RADIO_PWM4_IRQ (void)
{
	uint16_t now = TIM_Read(TIM_RADIO);
	uint16_t pulse = 0;
	static uint16_t tick;

	if (GPIO_Read(PWM_S4_GPIO, PWM_S4_PIN))
	{
		tick = now;
	}
	else
	{
		pulse = now - tick;

		// Check pulse is valid
		if (pulse <= (PWM_MAX + PWM_THRESH) && pulse >= (PWM_MIN - PWM_THRESH))
		{
			rx[3] = pulse;
			rxHeartbeat = CORE_GetTick();
		}
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
