/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "Radio.h"

/*
 * PRIVATE DEFINITIONS
 */

#define RADIO_DETECT_TIMEOUT	1000
#define RADIO_DETECT_PERIOD		100

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

/*
 * PRIVATE VARIABLES
 */

RADIO_Properties radio = {0};
RADIO_Data data = {0};
RADIO_ptrData ptrData;

/*
 * PUBLIC FUNCTIONS
 */

void RADIO_Detect (RADIO_Properties * r)
{
	PWM_Deinit();
	PPM_Deinit();
	SBUS_Deinit();
	IBUS_Deinit();

	uint32_t tick = CORE_GetTick();

	while (1)
	{
		uint32_t now = CORE_GetTick();
//		if (PPM_Detect())
//		{
//			r->Protocol = PPM;
//			break;
//		}

		if (SBUS_Detect(SBUS_BAUD))
		{
			r->Protocol = SBUS;
			r->Baud_SBUS = SBUS_BAUD;
			break;
		}

		if (SBUS_Detect(SBUS_BAUD_FAST))
		{
			r->Protocol = SBUS;
			r->Baud_SBUS = SBUS_BAUD_FAST;
			break;
		}

		if (IBUS_Detect())
		{
			r->Protocol = IBUS;
			break;
		}

		if (PWM_Detect())
		{
			r->Protocol = PWM;
			break;
		}

		if ( RADIO_DETECT_TIMEOUT <= (now - tick))
		{
			r->Protocol = PWM;
			break;
		}

		CORE_Idle();
	}
}

void RADIO_Init (RADIO_Properties *r)
{
	radio.Baud_SBUS = r->Baud_SBUS;
	radio.Protocol = r->Protocol;

	if (radio.Protocol == PPM)
	{
		data.ch_num = PPM_NUM_CHANNELS;
		PPM_Init();
		ptrData.ppm = PPM_GetDataPtr();
	}
	else if (radio.Protocol == SBUS)
	{
		data.ch_num = SBUS_NUM_CHANNELS;
		SBUS_Init(radio.Baud_SBUS);
		ptrData.sbus = SBUS_GetDataPtr();
	}
	else if (radio.Protocol == IBUS)
	{
		data.ch_num = IBUS_NUM_CHANNELS;
		IBUS_Init();
		ptrData.ibus = IBUS_GetDataPtr();
	}
	else // radio.Protocol == PWM
	{
		data.ch_num = PWM_NUM_CHANNELS;
		PWM_Init();
		ptrData.pwm = PWM_GetDataPtr();
	}
}

void RADIO_Update (void)
{
	switch (radio.Protocol) {

	case PWM:
		PWM_Update();
		data.inputLost = ptrData.pwm->inputLost;
		memcpy(data.ch, ptrData.pwm->ch, sizeof(ptrData.pwm->ch));
		break;

	case PPM:
		PPM_Update();
		data.inputLost = ptrData.ppm->inputLost;
		memcpy(data.ch, ptrData.ppm->ch, sizeof(ptrData.ppm->ch));
		break;

	case IBUS:
		IBUS_Update();
		data.inputLost = ptrData.ibus->inputLost;
		memcpy(data.ch, ptrData.ibus->ch, sizeof(ptrData.ibus->ch));
		break;

	case SBUS:
		SBUS_Update();
		data.inputLost = ptrData.sbus->inputLost;
		memcpy(data.ch, ptrData.sbus->ch, sizeof(ptrData.sbus->ch));
		break;

	default:
		radio.Protocol = PWM;
		ptrData.pwm = PWM_GetDataPtr();
		break;
	}
}

RADIO_Data* RADIO_GetDataPtr (void)
{
	return &data;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * INTERRUPT ROUTINES
 */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
