
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef RADIO_H
#define RADIO_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#include "STM32X.h"

#include "Core.h"
//#include "GPIO.h"
//#include "TIM.h"

#include "PWM.h"
#if defined(RADIO_USE_PPM)
#include "PPM.h"
#endif
#if defined(RADIO_USE_IBUS)
#include "IBUS.h"
#endif
#if defined(RADIO_USE_SBUS)
#include "SBUS.h"
#endif
#if defined(RADIO_USE_CRSF)
#include "CRSF.h"
#endif


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


// SIMPLE MIN MAX MACRO
//#define RADIO_MIN(X, Y)        (((X) < (Y)) ? (X) : (Y))
//#define RADIO_MAX(X, Y)        (((X) > (Y)) ? (X) : (Y))

// SIMPLE BIT-TWIDDLING
//#define RADIO_GETBIT(var, bit) (((var) >> (bit)) & 1)
//#define RADIO_SETBIT(var, bit) (var |= (1U << (bit)))
//#define RADIO_RSTBIT(var, bit) (var &= ~(1U << (bit)))

// CHANNEL COUNTS AND RANGES
#define RADIO_CH_NUM_MAX       16

#define RADIO_CH_MIN           1000
#define RADIO_CH_CENTER        1500
#define RADIO_CH_MAX           2000
#define RADIO_CH_STRETCH       300
#define RADIO_CH_ERROR         50
#define RADIO_CH_DEADBAND      50

#define RADIO_CH_ABSMIN        (RADIO_CH_MIN    - RADIO_CH_STRETCH - RADIO_CH_ERROR)
#define RADIO_CH_ABSMAX        (RADIO_CH_MAX    + RADIO_CH_STRETCH + RADIO_CH_ERROR)
#define RADIO_CH_CENTERMIN     (RADIO_CH_CENTER - RADIO_CH_DEADBAND)
#define RADIO_CH_CENTERMAX     (RADIO_CH_CENTER + RADIO_CH_DEADBAND)

#define RADIO_CH_HALFSCALE     (RADIO_CH_MAX - RADIO_CH_CENTER)
#define RADIO_CH_FULLSCALE     (RADIO_CH_MAX - RADIO_CH_MIN)


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC TYPES      									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


typedef enum {
	PWM,
#if defined(RADIO_USE_PPM)
    PPM,
#endif
#if defined(RADIO_USE_IBUS)
    IBUS,
#endif
#if defined(RADIO_USE_SBUS)
    SBUS,
#endif
#if defined(RADIO_USE_CRSF)
    CRSF
#endif
	RADIO_NUM_PROTOCOL,
} RADIO_protocol;


typedef enum {
    CH1,  CH2,  CH3,  CH4,
    CH5,  CH6,  CH7,  CH8,
    CH9,  CH10, CH11, CH12,
    CH13, CH14, CH15, CH16
} RADIO_chIndex;


typedef enum {
    chOFF,
    chFWD,
    chRVS
} RADIO_chActive;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


RADIO_protocol	RADIO_Init 				( RADIO_protocol );
void 			RADIO_Update 			( void );

uint32_t* 		RADIO_getPtrData 		( void );
bool* 			RADIO_getPtrFault 		( void );
uint8_t 		RADIO_getChCount		( void );
RADIO_chActive* RADIO_getPtrChActive 	( void );
//void 			RADIO_setChZeroPos		( void );

bool 			RADIO_inFaultStateCH   	( RADIO_chIndex );
bool 			RADIO_inFaultStateALL	( void );
bool 			RADIO_inFaultStateANY	( void );


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EXTERN DECLARATIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* RADIO_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
