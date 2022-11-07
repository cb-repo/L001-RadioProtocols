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

/*
 * PUBLIC FUNCTIONS
 */

RADIO_Properties RADIO_Detect (RADIO_Properties r)
{
	PWM_Deinit();
	PPM_Deinit();
	SBUS_Deinit();
	IBUS_Deinit();

	uint32_t tick = CORE_GetTick();
	while (1)
	{
		uint32_t now = CORE_GetTick();

		PPM_Properties ppm = {0};
		ppm.GPIO = r.GPIO_UART;
		ppm.Pin = r.Pin_UART;
		ppm.Timer = r.Timer;
		ppm.Tim_Freq = r.TimerFreq;
		ppm.Tim_Reload = r.TimerFreq;
		if (PPM_Detect(ppm))
		{
			r.Protocol = PPM;
			r.prtData.ptrDataPPM = PPM_GetDataPtr();
			break;
		}

		SBUS_Properties sbus = {0};
		sbus.GPIO = r.GPIO_UART;
		sbus.Pin = r.Pin_UART;
		sbus.UART = r.UART;
		sbus.Baud = r.Baud_SBUS;
		if (SBUS_Detect(sbus))
		{
			r.Protocol = SBUS;
			r.prtData.ptrDataSBUS = SBUS_GetDataPtr();
			break;
		}

		IBUS_Properties ibus = {0};
		ibus.GPIO = r.GPIO_UART;
		ibus.Pin = r.Pin_UART;
		ibus.UART = r.UART;
		if (IBUS_Detect(ibus))
		{
			r.Protocol = IBUS;
			r.prtData.ptrDataIBUS = IBUS_GetDataPtr();
			break;
		}

		PWM_Properties pwm = {0};
		memcpy(pwm.GPIO, r.GPIO_PWM, sizeof(pwm.GPIO));
		memcpy(pwm.Pin, r.Pin_PWM, sizeof(pwm.Pin));
		pwm.Timer = r.Timer;
		pwm.Tim_Freq = r.TimerFreq;
		pwm.Tim_Reload = r.TimerReload;
		if (PWM_Detect(pwm))
		{
			r.Protocol = PWM;
			r.prtData.ptrDataPWM = PWM_GetDataPtr();
			break;
		}

		if ( RADIO_DETECT_TIMEOUT <= (now - tick))
		{
			r.Protocol = PWM;
			break;
		}

		while (RADIO_DETECT_PERIOD > (CORE_GetTick() - now))
		{
			CORE_Idle();
		}
	}

	return r;
}

void RADIO_Init (RADIO_Properties r)
{
	radio = r;

	if (radio.Protocol == PPM)
	{
		data.ch_num = PPM_NUM_CHANNELS;
		PPM_Properties ppm = {0};
		ppm.GPIO = radio.GPIO_UART;
		ppm.Pin = radio.Pin_UART;
		ppm.Timer = radio.Timer;
		ppm.Tim_Freq = radio.TimerFreq;
		ppm.Tim_Reload = radio.TimerFreq;
		PPM_Init(ppm);
	}
	else if (radio.Protocol == SBUS)
	{
		data.ch_num = SBUS_NUM_CHANNELS;
		SBUS_Properties sbus = {0};
		sbus.GPIO = radio.GPIO_UART;
		sbus.Pin = radio.Pin_UART;
		sbus.UART = radio.UART;
		sbus.Baud = radio.Baud_SBUS;
		SBUS_Init(sbus);
	}
	else if (radio.Protocol == IBUS)
	{
		data.ch_num = IBUS_NUM_CHANNELS;
		IBUS_Properties ibus = {0};
		ibus.GPIO = radio.GPIO_UART;
		ibus.Pin = radio.Pin_UART;
		ibus.UART = radio.UART;
		IBUS_Init(ibus);
	}
	else // radio.Protocol == PWM
	{
		data.ch_num = PWM_NUM_CHANNELS;
		PWM_Properties pwm = {0};
		memcpy(pwm.GPIO, radio.GPIO_PWM, sizeof(pwm.GPIO));
		memcpy(pwm.Pin, radio.Pin_PWM, sizeof(pwm.Pin));
		pwm.Timer = radio.Timer;
		pwm.Tim_Freq = radio.TimerFreq;
		pwm.Tim_Reload = radio.TimerReload;
		PWM_Init(pwm);
	}
}

void RADIO_Update (void)
{
	switch (radio.Protocol) {

	case PWM:
		PWM_Update();
		data.failsafe = radio.prtData.ptrDataPWM->failsafe;
		memcpy(data.ch, radio.prtData.ptrDataPWM->ch, sizeof(radio.prtData.ptrDataPWM->ch));
		break;

	case PPM:
		PPM_Update();
		data.failsafe = radio.prtData.ptrDataPPM->failsafe;
		memcpy(data.ch, radio.prtData.ptrDataPPM->ch, sizeof(radio.prtData.ptrDataPPM->ch));
		break;

	case IBUS:
		IBUS_Update();
		data.failsafe = radio.prtData.ptrDataIBUS->failsafe;
		memcpy(data.ch, radio.prtData.ptrDataIBUS->ch, sizeof(radio.prtData.ptrDataIBUS->ch));
		break;

	case SBUS:
		SBUS_Update();
		data.failsafe = radio.prtData.ptrDataSBUS->failsafe;
		memcpy(data.ch, radio.prtData.ptrDataSBUS->ch, sizeof(radio.prtData.ptrDataSBUS->ch));
		break;

	default:
		radio.Protocol = PWM;
		radio.prtData.ptrDataPWM = PWM_GetDataPtr();
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
