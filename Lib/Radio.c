
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "Radio.h"

/*
 * PRIVATE DEFINITIONS
 */


#define RADIO_DETECT_TIMEOUT		1000
#define RADIO_DETECT_PERIOD			100

#define RADIO_DELAY_MEASURENEUTRAL	100

#define RADIO_ZERO_THRESHOLD	100
#define RADIO_INPUT_THRESHOLD	200


/*
 * PRIVATE TYPES
 */


// CONTAINS POINTER TO THE DATA STRUCTURE FOR THE ACTIVE PROTOCOL
typedef union {
	PWM_Data* 	pwm;
	PPM_Data* 	ppm;
	IBUS_Data* 	ibus;
	SBUS_Data*	sbus;
} RADIO_ptrModuleData;


/*
 * PRIVATE PROTOTYPES
 */


void	RADIO_ResetActiveChannelFlags 	( void );
void 	RADIO_UpdateActiveChannelFlags 	( void );


/*
 * PRIVATE VARIABLES
 */

RADIO_Properties	radio;
RADIO_Data 			data;
RADIO_ptrModuleData ptrModuleData;


/*
 * PUBLIC FUNCTIONS
 */


/*
 * DETECTS IF A RADIO IS CURRENTLY CONNECTED AND IF IT USES A DIFFERENT PROTOCOL TO THE CURRENT CONFIG
 *
 * INPUTS: 	POINTER TO RADIO_Properties STRUCT CONTAING CURRENT PROTOCOL CONFIG
 * RETURNS: TRUE	- IF A RADIO IS DETECTED AND THE PROTOCOL IS DIFFERENT TO CURRENT CONFIG
 * 					  NOTE: RADIO_Properties STRUCT WILL BE UPDATED WITH NEW RADIO CONFIG
 * 			FALSE	- NO RADIO IS DETECTED OR ITS THE SAME AS CURRENT CONFIG
 */
bool RADIO_DetectNew (RADIO_Properties * r)
{
	// INIT FUNCTION VARIABLES
	bool retVal = false;

	// DEINIT ALL PROTOCOLS IRRELEVANT OF CURRENT CONFIG, JUST TO BE SAFE
	PWM_Deinit();
	PPM_Deinit();
	SBUS_Deinit();
	IBUS_Deinit();

	// SEQUENTIALLY RUN DETECTION ALGO FOR EACH PROTOCOL
	if (PPM_Detect())
	{
		// IS THE PROCOTOL DIFFERENT TO THE CURRENT CONFIG
		if (r->Protocol != PPM)
		{
			// UPDATE STRUCT WITH NEW PROTOCOL
			r->Protocol = PPM;
			// SET PROTOCOL DETECTED FLAG
			retVal = true;
		}
	}
	else if (SBUS_Detect(SBUS_BAUD))
	{
		if (r->Protocol != SBUS && r->Baud_SBUS != SBUS_BAUD)
		{
			r->Protocol = SBUS;
			r->Baud_SBUS = SBUS_BAUD;
			retVal = true;
		}
	}
	else if (SBUS_Detect(SBUS_BAUD_FAST))
	{
		if (r->Protocol != SBUS && r->Baud_SBUS != SBUS_BAUD_FAST)
		{
			r->Protocol = SBUS;
			r->Baud_SBUS = SBUS_BAUD;
			retVal = true;
		}
	}
	else if (IBUS_Detect())
	{
		if (r->Protocol != IBUS)
		{
			r->Protocol = IBUS;
			retVal = true;
		}
	}
	else if (PWM_Detect())
	{
		if (r->Protocol != PWM)
		{
			r->Protocol = PWM;
			retVal = true;
		}
	}

	return retVal;
}


/*
 * HANDLES ALL INITIALISATIONS OF DATA STRUCTURES, TIMERS, CALIBRATION, ETC FOR CHOSEN PROTOCOL
 *
 * INPUTS: 	POINTER TO RADIO_Properties STRUCT DETAILING WHAT PROTOCOL TO INITIALISE
 * RETURNS: N/A
 */
void RADIO_Init (RADIO_Properties *r)
{
	// SAVE INIT INFO TO MODULE STRUCTURE
	radio.Baud_SBUS = r->Baud_SBUS;
	radio.Protocol = r->Protocol;

	// INIT PROTOCOL SPECIFIC INFO
	if (radio.Protocol == PPM)
	{
		// SAVE PROTOCOL SPECIFIC CHANNEL NUMBERS
		data.ch_num = PPM_NUM_CHANNELS;
		// RUN PROTOCOL SPECIFIC UPDATES
		PPM_Init();
		// GET POINTER TO THE PROTOCOL SPECIFIC STAT STRUCTURE
		ptrModuleData.ppm = PPM_GetDataPtr();
	}
	else if (radio.Protocol == SBUS)
	{
		data.ch_num = SBUS_NUM_CHANNELS;
		SBUS_Init(radio.Baud_SBUS);
		ptrModuleData.sbus = SBUS_GetDataPtr();
	}
	else if (radio.Protocol == IBUS)
	{
		data.ch_num = IBUS_NUM_CHANNELS;
		IBUS_Init();
		ptrModuleData.ibus = IBUS_GetDataPtr();
	}
	else // radio.Protocol == PWM
	{
		data.ch_num = PWM_NUM_CHANNELS;
		PWM_Init();
		ptrModuleData.pwm = PWM_GetDataPtr();
	}

	// SET THE CHANNEL ZERO POSITIONS, THIS WILL ALSO RESET CHANNEL ACTIVE FLAGS
	RADIO_SetChannelZeroPosition();			// Good idea to call this at another point as well because radio might not have valid data at this point
	// RUN A RADIO DATA UPDATE BEFORE PROGRESSING
	RADIO_Update();
}

/*
 * UPDATES ALL RELEVANT RADIO DATA
 * SHOULD BE POLLED REGULARLY, RECOMMENDED PERIOD = ~1MS
 *
 * INPUTS: 	N/A
 * RETURNS: N/A
 */
void RADIO_Update (void)
{
	// UPDATE AND PULL DATA FROM DEDICATED PROTOCOL MODULES TO GENERIC RADIO DATA STRUCT
	switch (radio.Protocol) {
	case PWM:
		// UPDATE PROTOCOL SPECIFIC DATA
		PWM_Update();
		// PULL 'INPUTLOST' FLAG FROM PROTOCOL MODULE
		data.inputLost = ptrModuleData.pwm->inputLost;
		// PULL CHANNEL DATA FROM PROTOCOL MODULE
		memcpy(data.ch, ptrModuleData.pwm->ch, sizeof(ptrModuleData.pwm->ch));
		break;
	case PPM:
		PPM_Update();
		data.inputLost = ptrModuleData.ppm->inputLost;
		memcpy(data.ch, ptrModuleData.ppm->ch, sizeof(ptrModuleData.ppm->ch));
		break;
	case IBUS:
		IBUS_Update();
		data.inputLost = ptrModuleData.ibus->inputLost;
		memcpy(data.ch, ptrModuleData.ibus->ch, sizeof(ptrModuleData.ibus->ch));
		break;
	case SBUS:
		SBUS_Update();
		data.inputLost = ptrModuleData.sbus->inputLost;
		memcpy(data.ch, ptrModuleData.sbus->ch, sizeof(ptrModuleData.sbus->ch));
		break;
	default:
		radio.Protocol = PWM;
		ptrModuleData.pwm = PWM_GetDataPtr();
		break;
	}

	// UPDATE CHANNEL ACTIVE FLAGS
	if ( !data.inputLost )
	{
		RADIO_UpdateActiveChannelFlags();
	}
}


/*
 * PROVIDES A POINTER TO THE RADIO DATA STRUCTURE
 * STRUCTURE CONTAINS ALL RELEVANT RADIO INFORMATION TO CONTROL DEVICE
 *
 * INPUTS: 	N/A
 * RETURNS: POINTER TO THE RADIO DATA STRUCTURE
 */
RADIO_Data* RADIO_GetDataPtr (void)
{
	return &data;
}


/*
 * ASSIGNS THE CURRENT CHANNEL DATA TO THE NEUTRAL/ZERO POSITION ARRAY
 * NEUTRAL/ZERO POSITION DATA IS USED TO CALCUIATE THE CHANNEL ACTIVE FLAG
 * NOTE:	- THIS DOES NOT UPDATE THE RADIO INPUTS, CALL RADIO_Update() FIRST IF YOU WANT THAT
 * 			- IT WILL NOT ATTEMPT TO ASSIGN THE DATA TO THE ARRAY IF NOT RADIO IS CONNECTED
 *
 * INPUTS: 	N/A
 * RETURNS: N/A
 */
void RADIO_SetChannelZeroPosition (void)
{
	// ONLY ASSIGN IF RADIO CONNECTED
	if (!data.inputLost)
	{
		// ITTERATE THROUGH EACH CHANNEL
		for (uint8_t ch = 0; ch < data.ch_num; ch++)
		{
			// RESET CHANNEL ACTIVE FLAG ARRAY
			data.chZero[ch] = data.ch[ch];
		}
	}
	// RESET CHANNEL ACTIVE FLAGS - CANT BE TRUE IF JUST RESET ZERO POSITION TO CURRENT
	RADIO_ResetActiveChannelFlags();
}


/*
 * PRIVATE FUNCTIONS
 */


/*
 * RESET THE FLAGS INDICATING IF A CHANNEL/RADIO INPUT IS ACTVE
 *
 * INPUTS: 	N/A
 * RETURNS: N/A
 */
void RADIO_ResetActiveChannelFlags (void)
{
	// ITTERATE THROUGH ENTIRE CHANNEL ARRAY
	for (uint8_t ch = 0; ch < RADIO_NUM_CHANNELS; ch++)
	{
		// RESET CHANNEL ACTIVE FLAG ARRAY
		data.chActive[ch] = chActive_False;
	}
}

/*
 * UPDATES CHANNEL ACTIVE FLAGS IN RADIO DATA STRUCT BASED ON POSITION RELATIVE TO NEUTRAL/ZERO FOR EACH CHANNEL
 *
 * INPUTS: 	N/A
 * RETURNS: N/A
 */
void RADIO_UpdateActiveChannelFlags (void)
{
	// ITTERATE THROUGH EACH CHANNEL
	for (uint8_t ch = 0; ch < data.ch_num; ch++)
	{
		// CHECK IF THE INPUT EXCEEDS THE POSITIVE ACTIVE THRESHOLD
		if (data.ch[ch] >= (data.chZero[ch] + RADIO_INPUT_THRESHOLD))
		{
			data.chActive[ch] = chActive_True;
		}
		// CHECK IF THE INPUT EXCEEDS THE NEGATIVE ACTIVE THRESHOLD
		else if (data.ch[ch] <= (data.chZero[ch] - RADIO_INPUT_THRESHOLD))
		{
			data.chActive[ch] = chActive_TrueRev;
		}
		// CHECK IF THE INPUT IS WITHIN THE NEUTRAL/ZERO BAND
		else if ( (data.ch[ch] <= (data.chZero[ch] + RADIO_ZERO_THRESHOLD)) &&
				  (data.ch[ch] >= (data.chZero[ch] - RADIO_ZERO_THRESHOLD)) )
		{
			data.chActive[ch] = chActive_False;
		}
	}
}


/*
 * INTERRUPT ROUTINES
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
