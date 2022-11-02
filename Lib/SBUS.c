/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "SBUS.h"
#include "Core.h"

/*
 * PRIVATE DEFINITIONS
 */

#define EMPTY				0
#define SBUS_HEADER_LEN		1
#define SBUS_PAYLOAD_LEN	25

#define SBUS_HEADER_INDEX	0
#define SBUS_DATA_INDEX		1

#define SBUS_HEADER			0xF0
#define SBUS_CH17_MASK		0x01
#define SBUS_CH18_MASK		0x02
#define SBUS_LOSTFRAME_MASK	0x04
#define SBUS_FAILSAFE_MASK	0x08

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

void SBUS_Decode (void);

/*
 * PRIVATE VARIABLES
 */

UART_t * uart;
uint32_t baud;
UART_Mode_t mode = UART_Mode_Inverted;

uint8_t rx[SBUS_PAYLOAD_LEN] = {0};
SBUS_Data data = {0};

/*
 * PUBLIC FUNCTIONS
 */

void SBUS_Init (UART_t * u, uint32_t b)
{
	uart = u;
	baud = b;
	UART_Init(uart, baud, mode);
}

void SBUS_Deinit (void)
{
	UART_Deinit(uart);
}

void SBUS_Update (void)
{
	if (UART_ReadCount(uart) >= SBUS_PAYLOAD_LEN)
	{
		UART_Read(uart, &rx[SBUS_HEADER_INDEX], SBUS_HEADER_LEN);
		if (rx[SBUS_HEADER_INDEX] == SBUS_HEADER)
		{
			UART_Read(uart, &rx[SBUS_DATA_INDEX], (SBUS_PAYLOAD_LEN - SBUS_HEADER_LEN));
			SBUS_Decode();
		}
	}
}

SBUS_Data* SBUS_GetDataPtr (void)
{
	return &data;
}

/*
 * PRIVATE FUNCTIONS
 */

void SBUS_Decode (void)
{
	data.ch[0] =	(int16_t)( (rx[1] 		| rx[2] << 8					) & 0x07FF);
	data.ch[1] =	(int16_t)( (rx[2] >> 3 	| rx[3] << 5					) & 0x07FF);
	data.ch[2] =	(int16_t)( (rx[3] >> 6 	| rx[4] << 2  	| rx[5] << 10 	) & 0x07FF);
	data.ch[3] =	(int16_t)( (rx[5] >> 1 	| rx[6] << 7					) & 0x07FF);
	data.ch[4] = 	(int16_t)( (rx[6] >> 4 	| rx[7] << 4					) & 0x07FF);
	data.ch[5] = 	(int16_t)( (rx[7] >> 7 	| rx[8] << 1 	| rx[9] << 9	) & 0x07FF);
	data.ch[6] =	(int16_t)( (rx[9] >> 2 	| rx[10] << 6					) & 0x07FF);
	data.ch[7] =	(int16_t)( (rx[10] >> 5	| rx[11] << 3					) & 0x07FF);

	data.ch[8] = 	(int16_t)( (rx[12] 		| rx[13] << 8					) & 0x07FF);
	data.ch[9] =	(int16_t)( (rx[13] >> 3	| rx[14] << 5					) & 0x07FF);
	data.ch[10] =	(int16_t)( (rx[14] >> 6	| rx[15] << 2 	| rx[16] << 10	) & 0x07FF);
	data.ch[11] =	(int16_t)( (rx[16] >> 1	| rx[17] << 7					) & 0x07FF);
	data.ch[12] =	(int16_t)( (rx[17] >> 4	| rx[18] << 4					) & 0x07FF);
	data.ch[13] =	(int16_t)( (rx[18] >> 7	| rx[19] << 1	| rx[20] << 9	) & 0x07FF);
	data.ch[14] =	(int16_t)( (rx[20] >> 2	| rx[21] << 6					) & 0x07FF);
	data.ch[15] =	(int16_t)( (rx[21] >> 5	| rx[22] << 3					) & 0x07FF);

	data.ch17 		= rx[23] & SBUS_CH17_MASK;
	data.ch18 		= rx[23] & SBUS_CH18_MASK;
	data.lost_frame	= rx[23] & SBUS_LOSTFRAME_MASK;
	data.failsafe 	= rx[23] & SBUS_FAILSAFE_MASK;
}

/*
 * INTERRUPT ROUTINES
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
