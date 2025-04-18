
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef SBUS_H
#define SBUS_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#include "STM32X.h"

#include "Core.h"
#include "UART.h"
#include "GPIO.h"
#include "US.h"


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#define SBUS_CH_NUM				16		// x16 standard channels

#define SBUS_BAUD				100000
#define SBUS_BAUD_FAST			200000

#define SBUS_PERIOD_ANALOGUE	14
#define SBUS_PERIOD_FAST		7
#define	SBUS_PERIOD				SBUS_PERIOD_ANALOGUE

#define SBUS_MIN				172
#define SBUS_CENTER				991
#define SBUS_MAX				1811
#define SBUS_RANGE				(SBUS_MAX - SBUS_MIN)

#define SBUS_MAP_MIN			1000
#define SBUS_MAP_MAX			2000
#define SBUS_MAP_RANGE			(SBUS_MAP_MAX - SBUS_MAP_MIN)


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC TYPES      									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


typedef struct {
	bool inputLost;
	bool frameLost;
	bool failsafe;
	bool ch17;
	bool ch18;
	uint32_t ch[SBUS_CH_NUM];
} SBUS_Data;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


void 		SBUS_Init 		( uint32_t );
void 		SBUS_Deinit 	( void );
bool 		SBUS_Detect 	( void );
void 		SBUS_Update 	( void );

SBUS_Data*	SBUS_getDataPtr	( void );


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EXTERN DECLARATIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* SBUS_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
