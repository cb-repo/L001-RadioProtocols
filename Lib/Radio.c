
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
	RADIO_protocol_t protocol;
    void   			( *init )( void );
    void   			( *deinit )( void );
    bool   			( *detect )( void );
    void   			( *update )( void );
    uint32_t*		( *getData )( void );
    bool*			( *getInputLost )( void );
    uint8_t			chCount;
} RADIO_ops;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void		RADIO_setProtocol 			( RADIO_protocol_t );
static void 	RADIO_updateChActive		( void );
static uint8_t	RADIO_updateValidChCount 	( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static bool 			initialised = false;

static RADIO_ops    	ops;
static RADIO_chActive_t	chActiveCount[ RADIO_CH_NUM_MAX ];
static uint8_t 			chValidCount;

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
	initialised = true;

	RADIO_setProtocol(initial);
	if ( ops.detect ) { return initial; }

    for ( RADIO_protocol_t p = 0; p < RADIO_NUM_PROTOCOL; p++ ) {
        if ( p == initial ) { continue; }
    	RADIO_setProtocol(p);
        if ( ops.detect ) { return p; }
    }

    RADIO_setProtocol(initial);
    ops.init();
    return initial;
}

/*
 * RADIO_Deinit
 *  -
 */
void RADIO_Deinit ( void )
{
	initialised = false;
    ops.deinit();
}

/*
 * RADIO_Update
 *  - Poll this in your main loop (every ~1ms) to refresh channel data & fault flags.
 */
void RADIO_Update ( void )
{
    ops.update();
    RADIO_updateChActive();
    RADIO_updateValidChCount();
}

/*
 * RADIO_getDataPtr
 *  - After RADIO_Update(), use this to read ch[], inputLost, etc.
 */
uint32_t* RADIO_getData ( void )
{
    return ops.getData();
}

/*
 * RADIO_getPtrFault
 *  -
 */
bool* RADIO_getInputLost ( void )
{
    return ops.getInputLost();
}

/*
 * RADIO_getChCount
 *  -
 */
uint8_t RADIO_getChCount ( void )
{
    return ops.chCount;
}

/*
 * RADIO_getPtrChActive =
 */
RADIO_chActive_t* RADIO_getChActiveCount ( void )
{
    return chActiveCount;
}

/*
 * RADIO_getValidChCount
 *  -
 */
uint8_t RADIO_getChValidCount ( void )
{
    return chValidCount;
}

/*
 * RADIO_inFaultState
 *   - True if there’s currently no valid radio input.
 */
bool RADIO_inFaultStateCH ( RADIO_chIndex_t c )
{
	if ( !initialised ) {
		return true;
	}

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
	if ( !initialised ) {
		return true;
	}

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
	if ( !initialised ) {
		return true;
	}

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


/*
 * RADIO_setProtocol
 *  - Fill in 'radio' ops & ch_num
 */
static void RADIO_setProtocol ( RADIO_protocol_t p )
{
	ops.protocol = p;

    switch (p) {

#ifdef RADIO_USE_PPM
    case PPM:
        ops.init	= PPM_Init;
        ops.deinit	= PPM_Deinit;
        ops.detect	= PPM_Detect;
        ops.update	= PPM_Update;
        ops.getData	= PPM_getDataPtr;
        ops.getInputLost = PPM_getInputLostPtr;
        ops.chCount	= PPM_CH_NUM;
        break;
#endif

#ifdef RADIO_USE_IBUS
    case IBUS:
        ops.init	= IBUS_Init;
        ops.deinit	= IBUS_Deinit;
        ops.detect	= IBUS_Detect;
        ops.update	= IBUS_Update;
        ops.getData	= IBUS_getDataPtr;
        ops.getInputLost = IBUS_getInputLostPtr;
        ops.chCount	= IBUS_CH_NUM;
        break;
#endif

#ifdef RADIO_USE_SBUS
    case SBUS:
        ops.init	= SBUS_Init;
        ops.deinit	= SBUS_Deinit;
        ops.detect	= SBUS_Detect;
        ops.update	= SBUS_Update;
        ops.getData	= SBUS_getDataPtr;
        ops.getInputLost = SBUS_getInputLostPtr;
        ops.chCount	= SBUS_CH_NUM;
        break;
#endif

#ifdef RADIO_USE_CRSF
    case CRSF:
        ops.init	= CRSF_Init;
        ops.deinit	= CRSF_Deinit;
        ops.detect	= CRSF_Detect;
        ops.update	= CRSF_Update;
        ops.getData	= CRSF_getDataPtr;
        ops.getInputLost = CRSF_getInputLostPtr;
        ops.chCount	= CRSF_CH_NUM;
        break;
#endif

    case PWM:
    default:
        ops.init	= PWM_Init;
        ops.deinit	= PWM_Deinit;
        ops.detect	= PWM_Detect;
        ops.update	= PWM_Update;
        ops.getData	= PWM_getDataPtr;
        ops.getInputLost = PWM_getInputLostPtr;
        ops.chCount	= PWM_CH_NUM;
        break;
    }
}

/*
 * RADIO_updateChActive
 *  -
 */
static void RADIO_updateChActive ( void )
{
    for ( uint8_t i = 0; i < ops.chCount; i++ ) {
        if ( ops.getInputLost()[i] ) {
            chActiveCount[i] = chOFF;
        } else if ( ops.getData()[i] > RADIO_CH_CENTERMAX ) {
            chActiveCount[i] = chFWD;
        } else if ( ops.getData()[i] < RADIO_CH_CENTERMIN ) {
            chActiveCount[i] = chRVS;
        } else {
            chActiveCount[i] = chOFF;
        }
    }
}

/*
 * RADIO_updateValidChCount
 *  -
 */
static uint8_t RADIO_updateValidChCount ( void )
{
	if ( ops.protocol == PWM ) {
		uint8_t count = 0;
		for ( uint8_t ch = CH1 ; ch < ops.chCount; ch++ ) {
			count += !ops.getInputLost()[ch];
		}
		return count;
	}

	else {
		if ( !*ops.getInputLost() ) {
			return ops.chCount;
		} else {
			return 0;
		}
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
