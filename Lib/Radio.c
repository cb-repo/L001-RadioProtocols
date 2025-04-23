
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
    void   		( *init )		( void );
    void   		( *deinit )		( void );
    bool   		( *detect )		( void );
    void   		( *update )		( void );
    uint32_t*	( *getData )	( void );
    bool*		( *getFault )	( void );
    uint8_t		chCount;
} RADIO_ops;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void	RADIO_setProtocol 		( RADIO_protocol );
static bool	RADIO_tryProtocol 		( RADIO_protocol );
static void RADIO_updateChActive	( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static bool 			initialised = false;

static RADIO_ops    	ops;
static RADIO_protocol	protocol;
static RADIO_chActive	activeFlags[ RADIO_CH_NUM_MAX ];

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
RADIO_protocol RADIO_Init ( RADIO_protocol initial )
{
	//
	initialised = true;

	// INITIALISE PROVIDED PROTOCOL
	if ( RADIO_tryProtocol( initial ) ) 		{ return protocol; }

	// TRY ALL PROTOCOLS IF FIRST ATTEMPT FAILS
    for ( RADIO_protocol p = 0; p < RADIO_NUM_PROTOCOL; p++ ) {
        if (p == initial) { continue; }
        if ( RADIO_tryProtocol(p) ) {
        	return protocol;
        }
    }

	// IF STILL NO DICE, FALL BACK TO INITAL

    RADIO_setProtocol(initial);
    ops.init();
    return protocol;
}

/*
 *
 */
void RADIO_Deinit ( void )
{
	initialised = false;
    ops.deinit();
}

/*
 * RADIO_Update
 *   - Poll this in your main loop (every ~1ms) to refresh channel data & fault flags.
 */
void RADIO_Update ( void )
{
    ops.update();
    RADIO_updateChActive();
}

/*
 * RADIO_getDataPtr
 *   - After RADIO_Update(), use this to read ch[], inputLost, etc.
 */
uint32_t* RADIO_getPtrData ( void )
{
    return ops.getData();
}

/*
 *
 */
bool* RADIO_getPtrFault ( void )
{
    return ops.getFault();
}

/*
 *
 */
uint8_t RADIO_getChCount ( void )
{
    return ops.chCount;
}

/*
 *
 */
RADIO_chActive* RADIO_getPtrChActive ( void )
{
    return activeFlags;
}

///*
// * RADIO_setChannelZeroPosition
// *   - Once you have a steady input, call this to record current sticks as “zero.”
// */
//void RADIO_setChZeroPos ( void )
//{
//    if ( !data.anyFault )
//    {
//        for (uint8_t i = 0; i < data.ch_num; i++) {
//            data.chZero[i] = data.ch[i];
//        }
//        data.chZeroSet = true;
//    }
//}

/*
 * RADIO_inFaultState
 *   - True if there’s currently no valid radio input.
 */
bool RADIO_inFaultStateCH ( RADIO_chIndex c )
{
	if ( !initialised ) {
		return true;
	}

	return ops.getFault()[c];
}

/*
 *
 */
bool RADIO_inFaultStateALL ( void )
{
	if ( !initialised ) {
		return true;
	}

    for ( uint8_t i = 0; i < ops.chCount; i++ ) {
    	bool fault = ops.getFault()[i];
        if ( !fault ) {
        	return false;
        }
    }

    return true;
}

/*
 *
 */
bool RADIO_inFaultStateANY ( void )
{
	if ( !initialised ) {
		return true;
	}

    for ( uint8_t i = 0; i < ops.chCount; i++ ) {
        if ( ops.getFault()[i] ) {
        	return true;
        }
    }

    return false;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * RADIO_setProtocol
 *  - Fill in 'radio' ops & ch_num
 */
static void RADIO_setProtocol ( RADIO_protocol p )
{
	//
    protocol = p;

    //
    switch (p)
    {

#if defined(RADIO_USE_PPM)
    case PPM:
        ops.init     = PPM_Init;
        ops.deinit   = PPM_Deinit;
        ops.update   = PPM_Update;
        ops.getData  = PPM_getPtrData;
        ops.getFault = PPM_getPtrFault;    /* returns bool* to chFault[] */
        ops.chCount  = PPM_CH_NUM;
        break;
#endif

#if defined(RADIO_USE_IBUS)
    case IBUS:
        ops.init     = IBUS_Init;
        ops.deinit   = IBUS_Deinit;
        ops.update   = IBUS_Update;
        ops.getData  = IBUS_getPtrData;
        ops.getFault = IBUS_getPtrFault;
        ops.chCount  = IBUS_CH_NUM;
        break;
#endif

#if defined(RADIO_USE_SBUS)
    case SBUS:
        ops.init     = SBUS_Init;
        ops.deinit   = SBUS_Deinit;
        ops.update   = SBUS_Update;
        ops.getData  = SBUS_getPtrData;
        ops.getFault = SBUS_getPtrFault;
        ops.chCount  = SBUS_CH_NUM;
        break;
#endif

#if defined(RADIO_USE_CRSF)
    case CRSF:
        ops.init     = CRSF_Init;
        ops.deinit   = CRSF_Deinit;
        ops.update   = CRSF_Update;
        ops.getData  = CRSF_getPtrData;
        ops.getFault = CRSF_getPtrFault;
        ops.chCount  = CRSF_CH_NUM;
        break;
#endif

    case PWM:
    default:
        ops.init     = PWM_Init;
        ops.deinit   = PWM_Deinit;
        ops.update   = PWM_Update;
        ops.getData  = PWM_getPtrData;
        ops.getFault = PWM_getPtrFault;
        ops.chCount  = PWM_CH_NUM;
        break;
    }
}

/*
 * RADIO_tryProtocol
 *  - init, wait for signal, deinit if fail
 */
static bool RADIO_tryProtocol(RADIO_protocol p)
{
    RADIO_setProtocol(p);
    ops.init();

    uint32_t start = CORE_GetTick();
    while ((CORE_GetTick() - start) < RADIO_DETECT_TIMEOUT_MS) {
        ops.update();
        bool anyGood = false;
        bool *faults = ops.getFault();
        for (uint8_t i = 0; i < ops.chCount; i++) {
            if (!faults[i]) { anyGood = true; break; }
        }
        if (anyGood) return true;
    }
    ops.deinit();
    return false;
}

/*
 *
 */
static void RADIO_updateChActive ( void )
{
    for (uint8_t i = 0; i < ops.chCount; i++)
    {
        if ( ops.getFault()[i] ) {
            activeFlags[i] = chOFF;
        } else if ( ops.getData()[i] > RADIO_CH_CENTERMAX ) {
            activeFlags[i] = chFWD;
        } else if ( ops.getData()[i] < RADIO_CH_CENTERMIN ) {
            activeFlags[i] = chRVS;
        } else {
            activeFlags[i] = chOFF;
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

