
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef RADIOCOMMON_H
#define RADIOCOMMON_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#include "STM32X.h"

#include "PWM.h"
#include "CRSF.h"
#include "PPM.h"
#include "IBUS.h"
#include "SBUS.h"


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#define RADIO_MIN(X, Y) 		(((X) < (Y)) ? (X) : (Y))
#define RADIO_MAX(X, Y) 		(((X) > (Y)) ? (X) : (Y))

#define RADIO_GETBIT(var, bit) 	(((var) >> (bit)) & 1)
#define RADIO_SETBIT(var, bit) 	(var |= (1 << (bit)))
#define RADIO_RSTBIT(var, bit)	(var &= (~(1 << (bit))))

#define RADIO_CH_NUM_MAX		(RADIO_MAX(RADIO_MAX(RADIO_MAX(RADIO_MAX(PWM_CH_NUM, CRSF_CH_NUM), PPM_CH_NUM), IBUS_CH_NUM), SBUS_CH_NUM))

#define RADIO_CH_MIN			1000
#define RADIO_CH_CENTER			1500
#define RADIO_CH_MAX			2000
#define RADIO_CH_STRETCH		300
#define RADIO_CH_ERROR			50
#define RADIO_CH_DEADBAND		50

#define RADIO_CH_ABSMIN			(RADIO_CH_MIN - RADIO_CH_STRETCH - RADIO_CH_ERROR)
#define RADIO_CH_ABSMAX			(RADIO_CH_MAX + RADIO_CH_STRETCH + RADIO_CH_ERROR)
#define RADIO_CH_CENTERMIN		(RADIO_CH_CENTER - RADIO_CH_DEADBAND)
#define RADIO_CH_CENTERMAX		(RADIO_CH_CENTER + RADIO_CH_DEADBAND)

#define RADIO_CH_HALFSCALE		(RADIO_CH_MAX - RADIO_CH_CENTER)
#define RADIO_CH_FULLSCALE		(RADIO_CH_MAX - RADIO_CH_MIN)


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC TYPES      									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


typedef enum {
	PPM,
	SBUS,
	IBUS,
	PWM,
	CRSF,
} RADIO_protocol;

typedef enum {
	chOFF,
	chFWD,
	chRVS,
} RADIO_chActive;

typedef enum {
	CH1,  CH2,  CH3,  CH4,
	CH5,  CH6,  CH7,  CH8,
	CH9,  CH10, CH11, CH12,
	CH13, CH14, CH15, CH16,
	CH17, CH18, CH19, CH20,
	CH21, CH22, CH23, CH24,
} RADIO_chIndex;

typedef struct {
	bool			allFault;
	bool			anyFault;
	bool 			chFault[ RADIO_CH_NUM_MAX ];
	uint32_t 		ch[ RADIO_CH_NUM_MAX ];
	RADIO_chActive	chActive[ RADIO_CH_NUM_MAX ];
} RADIO_dataModule;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EXTERN DECLARATIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* RADIOCOMMON_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
