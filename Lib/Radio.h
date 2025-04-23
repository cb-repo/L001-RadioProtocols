
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef RADIO_H
#define RADIO_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"

#include "Core.h"
#include "PWM.h"
#ifdef RADIO_USE_PPM
#include "PPM.h"
#endif
#ifdef RADIO_USE_IBUS
#include "IBUS.h"
#endif
#ifdef RADIO_USE_SBUS
#include "SBUS.h"
#endif
#ifdef RADIO_USE_CRSF
#include "CRSF.h"
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define RADIO_MIN(X, Y)    	(((X) < (Y)) ? (X) : (Y))
#define RADIO_MAX(X, Y)		(((X) > (Y)) ? (X) : (Y))

#define RADIO_CH_NUM_BASE   PWM_CH_NUM
#ifdef RADIO_USE_PPM
  #define RADIO_CH_NUM_PPM  RADIO_MAX(PPM_CH_NUM, RADIO_CH_NUM_BASE)
#else
  #define RADIO_CH_NUM_PPM  RADIO_CH_NUM_BASE
#endif
#ifdef RADIO_USE_IBUS
  #define RADIO_CH_NUM_IBUS RADIO_MAX(IBUS_CH_NUM, RADIO_CH_NUM_PPM)
#else
  #define RADIO_CH_NUM_IBUS RADIO_CH_NUM_PPM
#endif
#ifdef RADIO_USE_SBUS
  #define RADIO_CH_NUM_SBUS RADIO_MAX(SBUS_CH_NUM, RADIO_CH_NUM_IBUS)
#else
  #define RADIO_CH_NUM_SBUS RADIO_CH_NUM_IBUS
#endif
#ifdef RADIO_USE_CRSF
  #define RADIO_CH_NUM_MAX  RADIO_MAX(CRSF_CH_NUM, RADIO_CH_NUM_SBUS)
#else
  #define RADIO_CH_NUM_MAX  RADIO_CH_NUM_SBUS
#endif

#define RADIO_CH_MIN        1000
#define RADIO_CH_CENTER     1500
#define RADIO_CH_MAX        2000

#define RADIO_CH_STRETCH    300
#define RADIO_CH_ERROR      50
#define RADIO_CH_DEADBAND   50

#define RADIO_CH_ABSMIN     (RADIO_CH_MIN    - RADIO_CH_STRETCH - RADIO_CH_ERROR)
#define RADIO_CH_ABSMAX     (RADIO_CH_MAX    + RADIO_CH_STRETCH + RADIO_CH_ERROR)
#define RADIO_CH_CENTERMIN  (RADIO_CH_CENTER - RADIO_CH_DEADBAND)
#define RADIO_CH_CENTERMAX	(RADIO_CH_CENTER + RADIO_CH_DEADBAND)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC TYPES      									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	PWM,
#ifdef RADIO_USE_PPM
    PPM,
#endif
#ifdef RADIO_USE_IBUS
    IBUS,
#endif
#ifdef RADIO_USE_SBUS
    SBUS,
#endif
#ifdef RADIO_USE_CRSF
    CRSF,
#endif
	RADIO_NUM_PROTOCOL,
} RADIO_protocol_t;

typedef enum {
    CH1,  CH2,  CH3,  CH4,
    CH5,  CH6,  CH7,  CH8,
    CH9,  CH10, CH11, CH12,
    CH13, CH14, CH15, CH16
} RADIO_chIndex_t;

typedef enum {
    chOFF,
    chFWD,
    chRVS
} RADIO_chActive_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

RADIO_protocol_t	RADIO_Init 				( RADIO_protocol_t );
void 				RADIO_Update 			( void );

uint32_t* 			RADIO_getData 			( void );
bool* 				RADIO_getInputLost 		( void );
uint8_t 			RADIO_getChCount		( void );
RADIO_chActive_t* 	RADIO_getChActiveCount 	( void );
uint8_t 			RADIO_getChValidCount 	( void );

bool 				RADIO_inFaultStateCH   	( RADIO_chIndex_t );
bool 				RADIO_inFaultStateALL	( void );
bool 				RADIO_inFaultStateANY	( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EXTERN DECLARATIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* RADIO_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
