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

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define RADIO_NUM_CHANNELS	MAX( MAX(PWM_NUM_CHANNELS, PPM_NUM_CHANNELS), MAX(IBUS_NUM_CHANNELS, SBUS_NUM_CHANNELS) )

#define RADIO_MIN		1000
#define RADIO_CENTER	1500
#define RADIO_MAX		2000

/*
 * PUBLIC TYPES
 */

typedef enum {
	PPM,
	SBUS,
	IBUS,
	PWM,
} RADIO_Protocols;

//typedef union {
//	PWM_Properties pwm;
//	PPM_Properties ppm;
//	IBUS_Properties ibus;
//	SBUS_Properties sbus;
//} RADIO_ProtocolProperties;

typedef union {
	PWM_Data* ptrDataPWM;
	PPM_Data* ptrDataPPM;
	IBUS_Data* ptrDataIBUS;
	SBUS_Data* ptrDataSBUS;
} RADIO_ptrProtocolData;

typedef struct {
	GPIO_t * GPIO_PWM[PWM_NUM_CHANNELS];
	uint8_t Pin_PWM[PWM_NUM_CHANNELS];
	UART_t * UART;
	uint32_t Baud_SBUS;
	GPIO_t * GPIO_UART;
	uint8_t Pin_UART;
	TIM_t * Timer;
	uint32_t TimerFreq;
	uint32_t TimerReload;
	RADIO_Protocols Protocol;
	RADIO_ptrProtocolData prtData;
} RADIO_Properties;

typedef struct {
	bool failsafe;
	uint8_t ch_num;
	int16_t ch[RADIO_NUM_CHANNELS];
} RADIO_Data;

/*
 * PUBLIC FUNCTIONS
 */

RADIO_Properties RADIO_Detect (RADIO_Properties);
void RADIO_Init (RADIO_Properties);
void RADIO_Update (void);
RADIO_Data* RADIO_GetDataPtr (void);

/*
 * EXTERN DECLARATIONS
 */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* RADIO_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */