
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

#define RADIO_ZEROARRAY_DELAY	100
#define RADIO_INPUTDET_DELAY	100


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

bool SYSTEM_DetectRadio 			( void );
void RADIO_ResetChannelData			( void );
void RADIO_ResetChannelZeroData		( void );
void RADIO_ResetActiveChannelFlags 	( void );
void RADIO_UpdateActiveChannelFlags ( void );


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
bool RADIO_DetInit (RADIO_Properties * r)
{
	// INIT FUNCTION VARIABLES
	bool retVal = false;
	RADIO_Properties detProperties = {SBUS_BAUD, PWM};

	// DEINIT ALL PROTOCOLS IRRELEVANT OF CURRENT CONFIG, JUST TO BE SAFE
	PWM_Deinit();
	PPM_Deinit();
	SBUS_Deinit();
	IBUS_Deinit();

	// TEST ALL PROTOCOLS
	while (1)
	{
		// TEST FOR PPM RADIO
		detProperties.Protocol = PPM;
		RADIO_Init(&detProperties);
		if ( SYSTEM_DetectRadio() )
		{
			r->Protocol = PPM;
			retVal = true;
			break;
		}
		PPM_Deinit();

		// TEST FOR SBUS RADIO AT STANDARD BAUD
		detProperties.Protocol = SBUS;
		detProperties.Baud_SBUS = SBUS_BAUD;
		RADIO_Init(&detProperties);
		if ( SYSTEM_DetectRadio() )
		{
			r->Protocol = SBUS;
			r->Baud_SBUS = SBUS_BAUD;
			retVal = true;
			break;
		}
		SBUS_Deinit();

		// TEST FOR SBUS RADIO AT FAST BAUD
		detProperties.Protocol = SBUS;
		detProperties.Baud_SBUS = SBUS_BAUD_FAST;
		RADIO_Init(&detProperties);
		if ( SYSTEM_DetectRadio() )
		{
			r->Protocol = SBUS;
			r->Baud_SBUS = SBUS_BAUD_FAST;
			retVal = true;
			break;
		}
		SBUS_Deinit();

		// TEST FOR IBUS RADIO
		detProperties.Protocol = IBUS;
		RADIO_Init(&detProperties);
		if ( SYSTEM_DetectRadio() )
		{
			r->Protocol = IBUS;
			retVal = true;
			break;
		}
		IBUS_Deinit();

		// TEST FOR PWM RADIO
		detProperties.Protocol = PWM;
		RADIO_Init(&detProperties);
		if ( SYSTEM_DetectRadio() )
		{
			r->Protocol = PWM;
			retVal = true;
			break;
		}
		PWM_Deinit();

		// NO NEW RADIO DETECTED, REINIT INITIAL CONFIG
		RADIO_Init(r);
		retVal = false;
		break;
	}

//	// RESET
//	data.inputLost = true;
//
//	// SEQUENTIALLY RUN DETECTION ALGO FOR EACH PROTOCOL
//	if (PPM_DetInit())
//	{
//		// IS THE PROCOTOL DIFFERENT TO THE CURRENT CONFIG
//		if (r->Protocol != PPM)
//		{
//			// UPDATE STRUCT WITH NEW PROTOCOL
//			r->Protocol = PPM;
//			// SAVE PROTOCOL SPECIFIC CHANNEL NUMBERS
//			data.ch_num = PPM_NUM_CHANNELS;
//			// GET POINTER TO THE PROTOCOL SPECIFIC STAT STRUCTURE
//			ptrModuleData.ppm = PPM_GetDataPtr();
//			// SET PROTOCOL DETECTED FLAG
//			retVal = true;
//		}
//	}
//	else if (SBUS_DetInit(SBUS_BAUD))
//	{
//		if (r->Protocol != SBUS || r->Baud_SBUS != SBUS_BAUD)
//		{
//			r->Protocol = SBUS;
//			r->Baud_SBUS = SBUS_BAUD;
//			data.ch_num = SBUS_NUM_CHANNELS;
//			ptrModuleData.sbus = SBUS_GetDataPtr();
//			retVal = true;
//		}
//	}
//	else if (SBUS_DetInit(SBUS_BAUD_FAST))
//	{
//		if (r->Protocol != SBUS || r->Baud_SBUS != SBUS_BAUD_FAST)
//		{
//			r->Protocol = SBUS;
//			r->Baud_SBUS = SBUS_BAUD;
//			data.ch_num = SBUS_NUM_CHANNELS;
//			ptrModuleData.sbus = SBUS_GetDataPtr();
//			retVal = true;
//		}
//	}
//	else if (IBUS_DetInit())
//	{
//		if (r->Protocol != IBUS)
//		{
//			r->Protocol = IBUS;
//			data.ch_num = IBUS_NUM_CHANNELS;
//			ptrModuleData.ibus = IBUS_GetDataPtr();
//			retVal = true;
//		}
//	}
//	else if (PWM_DetInit())
//	{
//		if (r->Protocol != PWM)
//		{
//			r->Protocol = PWM;
//			data.ch_num = PWM_NUM_CHANNELS;
//			ptrModuleData.pwm = PWM_GetDataPtr();
//			retVal = true;
//		}
//	}

	// RUN A RADIO DATA UPDATE BEFORE PROGRESSING
	RADIO_Update();

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
	// RESET RADIO DATA STRUCT VARIABLES
	data.inputLost = true;
	RADIO_ResetChannelData();
	RADIO_ResetChannelZeroData();
	RADIO_ResetActiveChannelFlags();

	// SAVE INIT PROPERTIES TO THE RADIO PROPERTIES STRUCT
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
	// CREATE LOOP VARIABLES
	static uint32_t tick = 0;

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

	// IF THE CHANNEL NEUTRAL/ZERO POSITION ARRAY HAS NOT BEEN SET - ONLY SETS ONCE ON FIRST RADIO CONNECTION
	if ( !data.chZeroSet && !data.inputLost )
	{
		// ON FIRST CHECK ASSIGN VALUE TO TICK
		if ( tick == 0 )
		{
			tick = CORE_GetTick();
		}
		// ON SUBSEQUENT LOOP CHECK IF DELAY TIME HAS FINISHED - ALLOWING INPUT TO BECOME STABLE
		else if ( RADIO_ZEROARRAY_DELAY < (CORE_GetTick() - tick) )
		{
			// SET THE CHANNEL ZERO POSITIONS, THIS WILL ALSO RESET CHANNEL ACTIVE FLAGS
			RADIO_SetChannelZeroPosition();
			// SET FLAG SO DOESNT EVALUATE AGAIN
			data.chZeroSet = true;
		}
	}

	// UPDATE CHANNEL ACTIVE FLAGS IF CHANNEL ZERO/NEUTRAL POSITIONS HAVE BEEN RECORDED
	if ( data.chZeroSet && !data.inputLost )
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
 * DETECTS THE PRESENCE OF A RADIO
 *
 * INPUTS: 	N/A
 * RETURNS: TRUE 	- IF RADIO IS CONNECTED AND VALID DATA IS REVIECED
 * 			FALSE 	- NO RADIO OR VALID DATA RECIEVED
 */
bool SYSTEM_DetectRadio (void)
{
	// INIT FUNCTION VARIABLES
	uint32_t tick = CORE_GetTick();
	bool retVal = false;

	// GIVE TIME FOR RADIO TO CONNECT AND RECIEVE DATA
	while ( RADIO_INPUTDET_DELAY > (CORE_GetTick() - tick) )
	{
		// UPDATE RADIO DATA
		RADIO_Update();
		// IF RADIO CONNECTION VALID
		if ( !data.inputLost )
		{
			retVal = true;
			break;
		}
		// LOOP PACING
		CORE_Idle();
	}

	return retVal;
}

/*
 * RESET THE CHANNEL DATA ARRAY
 *
 * INPUTS: 	N/A
 * RETURNS: N/A
 */
void RADIO_ResetChannelData (void)
{
	// ITTERATE THROUGH ENTIRE CHANNEL ARRAY
	for (uint8_t ch = 0; ch < RADIO_NUM_CHANNELS; ch++)
	{
		// RESET CHANNEL DATA
		data.ch[ch] = 0;
	}
}


/*
 * RESET THE CHANNEL NEUTRAL/ZERO POSITION DATA ARRAY
 *
 * INPUTS: 	N/A
 * RETURNS: N/A
 */
void RADIO_ResetChannelZeroData (void)
{
	data.chZeroSet = false;
	// ITTERATE THROUGH ENTIRE CHANNEL ARRAY
	for (uint8_t ch = 0; ch < RADIO_NUM_CHANNELS; ch++)
	{
		// RESET CHANNEL ZERO ARRAY
		data.chZero[ch] = 0;
	}
}


/*
 * RESET THE FLAGS INDICATING IF A CHANNELIS ACTVE
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
		// CHECK IF CHANNEL DATA IS APPROPRIATE
		if ( (data.ch[ch] == 0) ||
			 (data.ch[ch] < (RADIO_CH_MIN - RADIO_INPUT_THRESHOLD)) ||
			 (data.ch[ch] > (RADIO_CH_MAX + RADIO_INPUT_THRESHOLD)) )
		{
			data.chActive[ch] = chActive_False;
		}
		// CHECK IF THE INPUT EXCEEDS THE POSITIVE ACTIVE THRESHOLD
		else if (data.ch[ch] >= (data.chZero[ch] + RADIO_INPUT_THRESHOLD))
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
