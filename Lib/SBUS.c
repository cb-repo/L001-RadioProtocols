/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "SBUS.h"

/*
 * PRIVATE DEFINITIONS
 */

#define EMPTY				0

#define SBUS_HEADER_LEN		1
#define SBUS_DATA_LEN		22
#define SBUS_AUX_LEN		1
#define SBUS_FOOTER_LEN		1
#define SBUS_PAYLOAD_LEN	(SBUS_HEADER_LEN + SBUS_DATA_LEN + SBUS_AUX_LEN + SBUS_FOOTER_LEN)

#define SBUS_HEADER_INDEX	0
#define SBUS_DATA_INDEX		(SBUS_HEADER_INDEX + SBUS_HEADER_LEN)
#define SBUS_AUX_INDEX		(SBUS_DATA_INDEX + SBUS_DATA_LEN)
#define SBUS_FOOTER_INDEX	(SBUS_AUX_INDEX + SBUS_AUX_LEN)

#define SBUS_HEADER			0xF0
#define SBUS_FOOTER			0x00
#define SBUS_CH17_MASK		0x01
#define SBUS_CH18_MASK		0x02
#define SBUS_LOSTFRAME_MASK	0x04
#define SBUS_FAILSAFE_MASK	0x08

#define SBUS_JITTER_ARRAY	3		// Given analogue 14ms payload period, ~42ms input lag
#define SBUS_THRESHOLD		500
#define SBUS_DROPPED_FRAMES	3
#define SBUS_TIMEOUT		(SBUS_PERIOD * SBUS_DROPPED_FRAMES)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

uint16_t SBUS_Truncate (uint16_t);
void SBUS_JitterAverage(void);
void SBUS_HandleUART (void);

/*
 * PRIVATE VARIABLES
 */

uint8_t rx[SBUS_JITTER_ARRAY][SBUS_PAYLOAD_LEN] = {0};
bool rxHeartbeat = false;
SBUS_Properties sbus = {0};
SBUS_Data data = {0};

/*
 * PUBLIC FUNCTIONS
 */

bool SBUS_Detect(SBUS_Properties s)
{
	SBUS_Init(s);

	uint32_t tick = CORE_GetTick();
	while ((SBUS_TIMEOUT * 2) > CORE_GetTick() - tick)
	{
		SBUS_HandleUART();
		CORE_Idle();
	}

	SBUS_Deinit();

	return rxHeartbeat;
}

void SBUS_Init (SBUS_Properties s)
{
	memset(rx, 0, sizeof(rx));

	sbus = s;
	UART_Init(sbus.UART, sbus.Baud, UART_Mode_Inverted);
}

void SBUS_Deinit (void)
{
	UART_Deinit(sbus.UART);
}

void SBUS_Update (void)
{
	// Update Rx Data
	SBUS_HandleUART();

	// Update Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t prev = 0;

	// Check for New Input Data
	if (rxHeartbeat)
	{
		SBUS_JitterAverage();
		rxHeartbeat = false;
		prev = now;
	}

	// Check for Input Failsafe
	if (SBUS_TIMEOUT <= (now - prev)) {
		data.failsafe = true;
		memset(rx, 0, sizeof(rx));
	} else {
		bool failsafe = false;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++) {
			if ((rx[i][23] & SBUS_FAILSAFE_MASK)) {
				failsafe = true;
				break;
			}
		}
		if (failsafe) { data.failsafe = true;}
		else { data.failsafe = false; }
	}

	for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
	{
		if ((rx[i][23] & SBUS_LOSTFRAME_MASK))
		{
			data.lost_frame = true;
			break;
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

uint16_t SBUS_Truncate (uint16_t r)
{
	uint16_t retVal = 0;

	if (r == 0) {
		retVal = 0;
	} else if (r < (SBUS_MIN - SBUS_THRESHOLD)) {
		retVal = 0;
	} else if (r < SBUS_MIN) {
		retVal = SBUS_MIN;
	} else if (r <= SBUS_MAX) {
		retVal = r;
	} else if (r < (SBUS_MAX + SBUS_THRESHOLD))	{
		retVal = SBUS_MAX;
	} else {
		retVal = 0;
	}

	return retVal;
}

void SBUS_JitterAverage(void)
{
	for (uint8_t l = 0; l < 2; l++)
	{
		uint8_t avg = 0;
		uint32_t ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][1 + (l*11)] | rx[i][2 + (l*11)] << 8 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[0 + (l*8)] = ch;

		avg = 0;
		ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][2 + (l*11)] >> 3 | rx[i][3 + (l*11)] << 5 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[1 + (l*8)] = ch;

		avg = 0;
		ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][3 + (l*11)] >> 6 | rx[i][4 + (l*11)] << 2  | rx[i][5 + (l*11)] << 10 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[2 + (l*8)] = ch;

		avg = 0;
		ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][5 + (l*11)] >> 1 | rx[i][6 + (l*11)] << 7 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[3 + (l*8)] = ch;

		avg = 0;
		ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][6 + (l*11)] >> 4 | rx[i][7 + (l*11)] << 4 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[4 + (l*8)] = ch;

		avg = 0;
		ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][7 + (l*11)] >> 7 | rx[i][8 + (l*11)] << 1 | rx[i][9 + (l*11)] << 9 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[5 + (l*8)] = ch;

		avg = 0;
		ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][9 + (l*11)] >> 2 | rx[i][10 + (l*11)] << 6 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[6 + (l*8)] = ch;

		avg = 0;
		ch = 0;
		for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
		{
			uint16_t trunc = (int16_t)( (rx[i][10 + (l*11)] >> 5 | rx[i][11 + (l*11)] << 3 ) & 0x07FF);
			trunc = SBUS_Truncate(trunc);
			if (trunc != 0)
			{
				ch += trunc;
				avg += 1;
			}
		}
		ch /= avg;
		data.ch[7 + (l*8)] = ch;
	}

	uint8_t ch17 = 0;
	uint8_t ch18 = 0;
	for (uint8_t i = 0; i < SBUS_JITTER_ARRAY; i++)
	{
		ch17 += rx[i][23] & SBUS_CH17_MASK;
		ch18 += rx[i][23] & SBUS_CH18_MASK;
	}
	if ( ch17 > (SBUS_JITTER_ARRAY / 2) ) { data.ch17 = true; }
	if ( ch18 > (SBUS_JITTER_ARRAY / 2) ) { data.ch18 = true; }
}


void SBUS_HandleUART (void)
{
	static uint8_t jitter = 0;			// Jitter-Smoothing Array Index

	while (UART_ReadCount(sbus.UART) >= SBUS_PAYLOAD_LEN)
	{
		UART_Read(sbus.UART, &rx[jitter][SBUS_HEADER_INDEX], SBUS_HEADER_LEN);
		if (rx[jitter][SBUS_HEADER_INDEX] == SBUS_HEADER)
		{
			// Read and Decode Channel Data
			UART_Read(sbus.UART, &rx[jitter][SBUS_DATA_INDEX], (SBUS_PAYLOAD_LEN - SBUS_HEADER_LEN));
			// Verify the Message Validity
			if (rx[jitter][SBUS_FOOTER_INDEX] == SBUS_FOOTER) {
				// Kick Heartbeat
				rxHeartbeat = true;
				//Increment Jitter Array Index
				jitter += 1;
				if (jitter >= SBUS_JITTER_ARRAY) { jitter = 0; }
			}
		}
	}
}

/*
 * INTERRUPT ROUTINES
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
