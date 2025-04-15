
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "Radio.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#define RADIO_DETECT_TIMEOUT		1000
#define RADIO_DETECT_PERIOD			100

#define RADIO_DELAY_MEASURENEUTRAL	100

#define RADIO_ZERO_THRESHOLD		100
#define RADIO_INPUT_THRESHOLD		200

#define RADIO_ZEROARRAY_DELAY		100
#define RADIO_INPUTDET_DELAY		200


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


bool Radio_detectRadio 				( void );
void RADIO_resetChannelData			( void );
void RADIO_resetChannelZeroData		( void );
void RADIO_resetActiveChannelFlags 	( void );
void RADIO_updateActiveChannelFlags ( void );


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


bool 			init = false;
RADIO_config	config;
RADIO_data 		data;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * HANDLES ALL INITIALISATIONS OF DATA STRUCTURES, TIMERS, CALIBRATION, ETC FOR CHOSEN PROTOCOL
 *
 * INPUTS: 	POINTER TO RADIO_Properties STRUCT DETAILING WHAT PROTOCOL TO INITIALISE
 * RETURNS: N/A
 */
uint8_t RADIO_Init ( RADIO_config *r )
{
	// RESET RADIO DATA ARRAY
	data.inputLost = true;
	RADIO_resetChannelData();
	RADIO_resetChannelZeroData();
	RADIO_resetActiveChannelFlags();

	// SAVE INIT PROPERTIES TO THE RADIO PROPERTIES STRUCT
	config.baudSBUS = r->baudSBUS;
	config.protocol = r->protocol;

	// INIT PROTOCOL SPECIFIC INFO
	if (config.protocol == PWM) {
		data.ch_num = PWM_CH_NUM;
		PWM_Init();
		ptrModuleData.pwm = PWM_GetDataPtr();
	}

#if defined( RADIO_USE_CRSF )
	else if (config.protocol == CRSF) {
			data.ch_num = CRSF_CH_NUM;
			CRSF_Init();
			ptrModuleData.sbus = CRSF_GetDataPtr();
	}
#endif

#if defined( RADIO_USE_PPM )
	else if (config.protocol == PPM) {
		data.ch_num = PWM_CH_NUM;
		PPM_Init();
		ptrModuleData.ppm = PPM_GetDataPtr();
	}
#endif

#if defined( RADIO_USE_SBUS )
	else if (config.protocol == SBUS) {
		data.ch_num = PWM_CH_NUM;
		SBUS_Init(radio.Baud_SBUS);
		ptrModuleData.sbus = SBUS_GetDataPtr();
	}
#endif

#if defined( RADIO_USE_IBUS )
	else if (config.protocol == IBUS) {
		data.ch_num = PWM_CH_NUM;
		IBUS_Init();
		ptrModuleData.ibus = IBUS_GetDataPtr();
	}
#endif

	//
	init = true;

	// RUN A RADIO DATA UPDATE BEFORE PROGRESSING
	RADIO_Update();

	return retVal;
}


/*
 *
 * DETECTS IF A RADIO IS CURRENTLY CONNECTED AND IF IT USES A DIFFERENT PROTOCOL TO THE CURRENT CONFIG
 *
 * INPUTS: 	POINTER TO RADIO_Properties STRUCT CONTAING CURRENT PROTOCOL CONFIG
 * RETURNS: TRUE	- IF A RADIO IS DETECTED AND THE PROTOCOL IS DIFFERENT TO CURRENT CONFIG
 * 					  NOTE: RADIO_Properties STRUCT WILL BE UPDATED WITH NEW RADIO CONFIG
 * 			FALSE	- NO RADIO IS DETECTED OR ITS THE SAME AS CURRENT CONFIG
 */
bool RADIO_Detect ( RADIO_config * r )
{
	// INIT FUNCTION VARIABLES
	bool retVal = false;
	RADIO_Properties detProperties = {SBUS_BAUD, RADIO_PWM};

	// DEINIT ALL PROTOCOLS IRRELEVANT OF CURRENT CONFIG, JUST TO BE SAFE
	PWM_Deinit();
	PPM_Deinit();
	SBUS_Deinit();
	IBUS_Deinit();

	// TEST ALL PROTOCOLS
	while (1)
	{
		// TEST FOR PPM RADIO
		detProperties.Protocol = RADIO_PPM;
		RADIO_Init(&detProperties);
		if ( Radio_DetectRadio() )
		{
			r->Protocol = RADIO_PPM;
			retVal = true;
			break;
		}
		PPM_Deinit();

		// TEST FOR SBUS RADIO AT STANDARD BAUD
		detProperties.Protocol = RADIO_SBUS;
		detProperties.Baud_SBUS = SBUS_BAUD;
		RADIO_Init(&detProperties);
		if ( Radio_DetectRadio() )
		{
			r->Protocol = RADIO_SBUS;
			r->Baud_SBUS = SBUS_BAUD;
			retVal = true;
			break;
		}
		SBUS_Deinit();

		// TEST FOR SBUS RADIO AT FAST BAUD
		detProperties.Protocol = RADIO_SBUS;
		detProperties.Baud_SBUS = SBUS_BAUD_FAST;
		RADIO_Init(&detProperties);
		if ( Radio_DetectRadio() )
		{
			r->Protocol = RADIO_SBUS;
			r->Baud_SBUS = SBUS_BAUD_FAST;
			retVal = true;
			break;
		}
		SBUS_Deinit();

		// TEST FOR IBUS RADIO
		detProperties.Protocol = RADIO_IBUS;
		RADIO_Init(&detProperties);
		if ( Radio_DetectRadio() )
		{
			r->Protocol = RADIO_IBUS;
			retVal = true;
			break;
		}
		IBUS_Deinit();

		// TEST FOR PWM RADIO
		detProperties.Protocol = RADIO_PWM;
		RADIO_Init(&detProperties);
		if ( Radio_DetectRadio() )
		{
			r->Protocol = RADIO_PWM;
			retVal = true;
			break;
		}
		PWM_Deinit();

		// NO NEW RADIO DETECTED, REINIT INITIAL CONFIG
		RADIO_Init(r);
		retVal = false;
		break;
	}

	// RUN A RADIO DATA UPDATE BEFORE PROGRESSING
	RADIO_Update();

	return retVal;
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
	switch (config.protocol)
	{

#if defined( RADIO_USE_PWM )
	case PWM:
		// UPDATE PROTOCOL SPECIFIC DATA
		PWM_Update();
		// PULL 'INPUTLOST' FLAG FROM PROTOCOL MODULE
		data.inputLost = ptrModuleData.pwm->inputLost;
		// PULL CHANNEL DATA FROM PROTOCOL MODULE
		memcpy(data.ch, ptrModuleData.pwm->ch, sizeof(ptrModuleData.pwm->ch));
		break;
#endif

#if defined( RADIO_USE_CRSF )
	case CRSF:
		CRSF_Update();
		data.inputLost = ptrModuleData.crsf->inputLost;
		memcpy(data.ch, ptrModuleData.crsf->ch, sizeof(ptrModuleData.crsf->ch));
		break;
#endif

#if defined( RADIO_USE_PPM )
	case PPM:
		PPM_Update();
		data.inputLost = ptrModuleData.ppm->inputLost;
		memcpy(data.ch, ptrModuleData.ppm->ch, sizeof(ptrModuleData.ppm->ch));
		break;
#endif

#if defined( RADIO_USE_IBUS )
	case IBUS:
		IBUS_Update();
		data.inputLost = ptrModuleData.ibus->inputLost;
		memcpy(data.ch, ptrModuleData.ibus->ch, sizeof(ptrModuleData.ibus->ch));
		break;
#endif

#if defined( RADIO_USE_SBUS )
	case SBUS:
		SBUS_Update();
		data.inputLost = ptrModuleData.sbus->inputLost;
		memcpy(data.ch, ptrModuleData.sbus->ch, sizeof(ptrModuleData.sbus->ch));
		break;
#endif

	default:
		config.protocol = PWM;
		ptrModuleData.pwm = PWM_getDataPtr();
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
			RADIO_setChannelZeroPosition();
			// SET FLAG SO DOESNT EVALUATE AGAIN
			data.chZeroSet = true;
		}
	}

	// UPDATE CHANNEL ACTIVE FLAGS IF CHANNEL ZERO/NEUTRAL POSITIONS HAVE BEEN RECORDED
	if ( data.chZeroSet && !data.inputLost )
	{
		RADIO_updateActiveChannelFlags();
	}
}


/*
 * PROVIDES A POINTER TO THE RADIO DATA STRUCTURE
 * STRUCTURE CONTAINS ALL RELEVANT RADIO INFORMATION TO CONTROL DEVICE
 *
 * INPUTS: 	N/A
 * RETURNS: POINTER TO THE RADIO DATA STRUCTURE
 */
RADIO_data* RADIO_getDataPtr (void)
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
void RADIO_setChannelZeroPosition (void)
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
	RADIO_resetActiveChannelFlags();
}


/*
 *
 */
bool RADIO_inFaultState ( void )
{
	if ( init ) {
		return data.inputLost;
	} else {
		return true;
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * DETECTS THE PRESENCE OF A RADIO
 *
 * INPUTS: 	N/A
 * RETURNS: TRUE 	- IF RADIO IS CONNECTED AND VALID DATA IS REVIECED
 * 			FALSE 	- NO RADIO OR VALID DATA RECIEVED
 */
bool RADIO_detectRadio (void)
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
void RADIO_resetChannelData (void)
{
	// ITTERATE THROUGH ENTIRE CHANNEL ARRAY
	for (uint8_t ch = 0; ch < RADIO_CH_NUM_MAX; ch++)
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
void RADIO_resetChannelZeroData (void)
{
	data.chZeroSet = false;
	// ITTERATE THROUGH ENTIRE CHANNEL ARRAY
	for (uint8_t ch = 0; ch < RADIO_CH_NUM_MAX; ch++)
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
void RADIO_resetActiveChannelFlags (void)
{
	// ITTERATE THROUGH ENTIRE CHANNEL ARRAY
	for (uint8_t ch = 0; ch < RADIO_CH_NUM_MAX; ch++)
	{
		// RESET CHANNEL ACTIVE FLAG ARRAY
		data.chActive[ch] = chOFF;
	}
}


/*
 * UPDATES CHANNEL ACTIVE FLAGS IN RADIO DATA STRUCT BASED ON POSITION RELATIVE TO NEUTRAL/ZERO FOR EACH CHANNEL
 *
 * INPUTS: 	N/A
 * RETURNS: N/A
 */
void RADIO_updateActiveChannelFlags (void)
{
	// ITTERATE THROUGH EACH CHANNEL
	for (uint8_t ch = 0; ch < data.ch_num; ch++)
	{
		// CHECK IF CHANNEL DATA IS APPROPRIATE
		if ( (data.ch[ch] == 0) ||
			 (data.ch[ch] < (RADIO_CH_MIN - RADIO_INPUT_THRESHOLD)) ||
			 (data.ch[ch] > (RADIO_CH_MAX + RADIO_INPUT_THRESHOLD)) )
		{
			data.chActive[ch] = chOFF;
		}
		// CHECK IF THE INPUT EXCEEDS THE POSITIVE ACTIVE THRESHOLD
		else if (data.ch[ch] >= (data.chZero[ch] + RADIO_INPUT_THRESHOLD))
		{
			data.chActive[ch] = chFWD;
		}
		// CHECK IF THE INPUT EXCEEDS THE NEGATIVE ACTIVE THRESHOLD
		else if (data.ch[ch] <= (data.chZero[ch] - RADIO_INPUT_THRESHOLD))
		{
			data.chActive[ch] = chRVS;
		}
		// CHECK IF THE INPUT IS WITHIN THE NEUTRAL/ZERO BAND
		else if ( (data.ch[ch] <= (data.chZero[ch] + RADIO_ZERO_THRESHOLD)) &&
				  (data.ch[ch] >= (data.chZero[ch] - RADIO_ZERO_THRESHOLD)) )
		{
			data.chActive[ch] = chOFF;
		}
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS   									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
