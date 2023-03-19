
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef RADIO_H
#define RADIO_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#include "STM32X.h"
#include "PWM.h"
#include "PPM.h"
#include "SBUS.h"
#include "IBUS.h"


/*
 * PUBLIC DEFINITIONS
 */


#define RADIO_MIN(X, Y) 		(((X) < (Y)) ? (X) : (Y))
#define RADIO_MAX(X, Y) 		(((X) > (Y)) ? (X) : (Y))

#define RADIO_GETBIT(var, bit) 	(((var) >> (bit)) & 1)
#define RADIO_SETBIT(var, bit) 	(var |= (1 << (bit)))
#define RADIO_RSTBIT(var, bit)	(var &= (~(1 << (bit))))

#define RADIO_NUM_CHANNELS		(RADIO_MAX( RADIO_MAX(PWM_NUM_CHANNELS, PPM_NUM_CHANNELS), RADIO_MAX(IBUS_NUM_CHANNELS, SBUS_NUM_CHANNELS) ))

#define RADIO_CH_MIN			1000
#define RADIO_CH_CENTER			1500
#define RADIO_CH_MAX			2000

#define RADIO_CH_HALFSCALE		(RADIO_CH_MAX - RADIO_CH_CENTER)
#define RADIO_CH_FULLSCALE		(RADIO_CH_MAX - RADIO_CH_MIN)


/*
 * PUBLIC TYPES
 */


typedef enum {
	PPM,
	SBUS,
	IBUS,
	PWM,
} RADIO_Protocols;

typedef enum {
	chActive_False,
	chActive_True,
	chActive_TrueRev,
} RADIO_ChannelActiveFlags;

typedef struct {
	bool 						inputLost;
	uint8_t 					ch_num;
	uint32_t 					ch[RADIO_NUM_CHANNELS];
	bool						chZeroSet;
	uint32_t					chZero[RADIO_NUM_CHANNELS];
	RADIO_ChannelActiveFlags	chActive[RADIO_NUM_CHANNELS];
} RADIO_Data;

typedef struct {
	uint32_t 		Baud_SBUS;
	RADIO_Protocols	Protocol;
} RADIO_Properties;


/*
 * PUBLIC FUNCTIONS
 */


bool		RADIO_DetInit					( RADIO_Properties * );
void 		RADIO_Init 						( RADIO_Properties * );
void 		RADIO_Update 					( void );
RADIO_Data*	RADIO_GetDataPtr				( void );
void		RADIO_SetChannelZeroPosition	( void );


/*
 * EXTERN DECLARATIONS
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* RADIO_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
