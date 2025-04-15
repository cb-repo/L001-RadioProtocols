
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef RADIO_H
#define RADIO_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#include "STM32X.h"

#include "RadioCommon.h"

#if defined(RADIO_USE_PWM)
#include "PWM.h"
#endif
#if defined(RADIO_USE_CRSF)
#include "CRSF.h"
#endif
#if defined(RADIO_USE_PPM)
#include "PPM.h"
#endif
#if defined(RADIO_USE_IBUS)
#include "IBUS.h"
#endif
#if defined(RADIO_USE_SBUS)
#include "SBUS.h"
#endif


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC TYPES      									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


typedef struct {
	bool 			inputLost;
	bool			allFault;
	bool			anyFault;
	bool 			chFault[ RADIO_CH_NUM_MAX ];
	uint32_t 		ch[ RADIO_CH_NUM_MAX ];
	uint8_t 		ch_num;
	RADIO_chActive	chActive[ RADIO_CH_NUM_MAX ];
	bool			chZeroSet;
	uint32_t		chZero[RADIO_CH_NUM_MAX];
} RADIO_data;

typedef struct {
	uint32_t 		baudSBUS;
	RADIO_protocol	protocol;
} RADIO_config;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


uint8_t 	RADIO_Init 						( RADIO_config * );
bool		RADIO_Detect					( RADIO_config * );
void 		RADIO_Update 					( void );

RADIO_data*	RADIO_getDataPtr				( void );
void		RADIO_setChannelZeroPosition	( void );
bool 		RADIO_inFaultState 				( void );


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EXTERN DECLARATIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* RADIO_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
