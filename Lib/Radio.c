
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "Radio.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define RADIO_DETECT_TIMEOUT_MS   	1000	/* How long (ms) to wait for frames when trying a protocol */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* “v‑table” of the five functions every protocol must provide */
typedef struct {
	bool 				initialised;
	RADIO_protocol_t 	protocol;
    uint8_t				chCount;
    RADIO_chActive_t	chActiveCount[RADIO_CH_NUM_MAX];
    uint8_t 			chValidCount;
    uint32_t*		( *getData )( void );
    bool*			( *getInputLost )( void );
} RADIO_ops;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static inline bool RADIO_tryProtocol ( RADIO_protocol_t );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static RADIO_ops ops;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * RADIO_Init
 *  - Attempts to use 'initial' protocol first.
 *  - If no valid input appears, cycles through all other enabled protocols.
 *  - If still none succeed, falls back to 'initial'.
 *  - Returns the protocol actually in use.
 */
RADIO_protocol_t RADIO_Init ( RADIO_protocol_t initial )
{
	ops.initialised = true;

	if ( RADIO_tryProtocol(initial) )
	{
		return initial;
	}

	for ( RADIO_protocol_t p = 0; p < RADIO_NUM_PROTOCOL; p++ ) {
		if ( p == initial ) { continue; }
		if ( RADIO_tryProtocol(p) ) { return p; }
	}

	ops.protocol = initial;
    switch (initial) {
	#ifdef RADIO_USE_PPM
    case PPM:
		ops.chCount			= PPM_CH_NUM;
    	ops.getData 		= PPM_getData;
    	ops.getInputLost 	= PPM_getInputLost;
    	PPM_Init();
        break;
	#endif
	#ifdef RADIO_USE_IBUS
    case IBUS;
		ops.chCount			= IBUS_CH_NUM;
    	ops.getData 		= IBUS_getData;
    	ops.getInputLost 	= IBUS_getInputLost;
    	IBUS_Init();
        break;
	#endif
	#ifdef RADIO_USE_SBUS
    case SBUS:
		ops.chCount			= SBUS_CH_NUM;
    	ops.getData 		= SBUS_getData;
    	ops.getInputLost 	= SBUS_getInputLost;
    	SBUS_Init();
        break;
	#endif
	#ifdef RADIO_USE_CRSF
    case CRSF:
		ops.chCount			= CRSF_CH_NUM;
    	ops.getData 		= CRSF_getData;
    	ops.getInputLost 	= CRSF_getInputLost;
    	CRSF_Init();
        break;
	#endif
    case PWM:
    default:
		ops.chCount			= PWM_CH_NUM;
    	ops.getData 		= PWM_getData;
    	ops.getInputLost 	= PWM_getInputLost;
    	PWM_Init();
        break;
    }
    return initial;
}

/*
 * RADIO_Deinit
 *  -
 */
void RADIO_Deinit ( void )
{
	ops.initialised = false;

    switch (ops.protocol) {
	#ifdef RADIO_USE_PPM
    case PPM:
    	PPM_Deinit();
        break;
	#endif
	#ifdef RADIO_USE_IBUS
    case IBUS;
    	IBUS_Deinit();
        break;
	#endif
	#ifdef RADIO_USE_SBUS
    case SBUS:
    	SBUS_Deinit();
        break;
	#endif
	#ifdef RADIO_USE_CRSF
    case CRSF:
    	CRSF_Deinit();
        break;
	#endif
    case PWM:
    default:
    	PWM_Deinit();
        break;
    }
}

/*
 * RADIO_Update
 *  - Poll this in your main loop (every ~1ms) to refresh channel data & fault flags.
 */
void RADIO_Update ( void )
{
    switch (ops.protocol) {
	#ifdef RADIO_USE_PPM
    case PPM:
    	PPM_Update();
        break;
	#endif
	#ifdef RADIO_USE_IBUS
    case IBUS;
    	IBUS_Update();
        break;
	#endif
	#ifdef RADIO_USE_SBUS
    case SBUS:
    	SBUS_Update();
        break;
	#endif
	#ifdef RADIO_USE_CRSF
    case CRSF:
    	CRSF_Update();
        break;
	#endif
    case PWM:
    default:
    	PWM_Update();
        break;
    }

    // Update Active Channel Count
    for ( uint8_t i = 0; i < ops.chCount; i++ ) {
        if ( ops.getInputLost()[i] ) {
            ops.chActiveCount[i] = chOFF;
        } else if ( ops.getData()[i] > RADIO_CH_CENTERMAX ) {
            ops.chActiveCount[i] = chFWD;
        } else if ( ops.getData()[i] < RADIO_CH_CENTERMIN ) {
            ops.chActiveCount[i] = chRVS;
        } else {
            ops.chActiveCount[i] = chOFF;
        }
    }

    // Update Valid Channel Count
	if ( ops.protocol == PWM ) {
		uint8_t count = 0;
		for ( uint8_t ch = CH1 ; ch < ops.chCount; ch++ ) {
			count += !ops.getInputLost()[ch];
		}
		ops.chValidCount = count;
	} else {
		if ( ops.getInputLost() ) {
			ops.chValidCount = ops.chCount;
		} else {
			ops.chValidCount = 0;
		}
	}
}

/*
 * RADIO_getDataPtr
 *  - After RADIO_Update(), use this to read ch[], inputLost, etc.
 */
uint32_t* RADIO_getData ( void )
{
	if ( !ops.initialised ) { return NULL; }

    switch (ops.protocol) {
	#ifdef RADIO_USE_PPM
    case PPM:
    	return PPM_getData();
	#endif
	#ifdef RADIO_USE_IBUS
    case IBUS;
    	return IBUS_getData();
	#endif
	#ifdef RADIO_USE_SBUS
    case SBUS:
    	return SBUS_getData();
	#endif
	#ifdef RADIO_USE_CRSF
    case CRSF:
    	return CRSF_getData();
	#endif
    case PWM:
    default:
    	return PWM_getData();
    }
}

/*
 * RADIO_getChCount
 *  -
 */
uint8_t RADIO_getChCount ( void )
{
	if ( !ops.initialised ) { return 0; }

    return ops.chCount;
}

/*
 * RADIO_getPtrChActive =
 */
RADIO_chActive_t* RADIO_getChActiveCount ( void )
{
	if ( !ops.initialised ) { return NULL; }

    return ops.chActiveCount;
}

/*
 * RADIO_getValidChCount
 *  -
 */
uint8_t RADIO_getChValidCount ( void )
{
	if ( !ops.initialised ) { return 0; }

    return ops.chValidCount;
}

/*
 * RADIO_inFaultState
 *   - True if there’s currently no valid radio input.
 */
bool RADIO_inFaultStateCH ( RADIO_chIndex_t c )
{
	if ( !ops.initialised ) { return true; }

	if ( ops.protocol == PWM ) {
		return ops.getInputLost()[c];
	} else {
		return *ops.getInputLost();
	}
}

/*
 * RADIO_inFaultStateALL
 *  -
 */
bool RADIO_inFaultStateALL ( void )
{
	if ( !ops.initialised ) { return true; }

	if ( ops.protocol == PWM ) {
		for ( uint8_t i = 0; i < ops.chCount; i++ ) {
			if ( !ops.getInputLost()[i] ) {
				return false;
			}
		}
		return true;
	} else {
		return *ops.getInputLost();
	}
}

/*
 * RADIO_inFaultStateANY
 *  -
 */
bool RADIO_inFaultStateANY ( void )
{
	if ( !ops.initialised ) { return true; }

	if ( ops.protocol == PWM ) {
		for ( uint8_t i = 0; i < ops.chCount; i++ ) {
			if ( ops.getInputLost()[i] ) {
				return true;
			}
		}
		return false;
	} else {
		return *ops.getInputLost();
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static inline bool RADIO_tryProtocol ( RADIO_protocol_t p )
{
	ops.protocol = p;

	switch (p) {
	#ifdef RADIO_USE_PPM
	case PPM:
		ops.chCount			= PPM_CH_NUM;
    	ops.getData 		= PPM_getData;
    	ops.getInputLost	= PPM_getInputLost;
		return PPM_Detect();
	#endif
	#ifdef RADIO_USE_IBUS
	case IBUS;
		ops.chCount			= IBUS_CH_NUM;
    	ops.getData 		= IBUS_getData;
    	ops.getInputLost 	= IBUS_getInputLost;
		return IBUS_Detect();
	#endif
	#ifdef RADIO_USE_SBUS
	case SBUS:
		ops.chCount			= SBUS_CH_NUM;
    	ops.getData 		= SBUS_getData;
    	ops.getInputLost 	= SBUS_getInputLost;
		return SBUS_Detect();
	#endif
	#ifdef RADIO_USE_CRSF
	case CRSF:
		ops.chCount			= CRSF_CH_NUM;
    	ops.getData 		= CRSF_getData;
    	ops.getInputLost 	= CRSF_getInputLost;
		return CRSF_Detect();
	#endif
	case PWM:
	default:
		ops.chCount			= PWM_CH_NUM;
    	ops.getData 		= PWM_getData;
    	ops.getInputLost 	= PWM_getInputLost;
		return PWM_Detect();
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
