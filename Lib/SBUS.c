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

#define SBUS_HEADER			0x0F
#define SBUS_FOOTER			0x00
#define SBUS_CH17_MASK		0x01
#define SBUS_CH18_MASK		0x02
#define SBUS_LOSTFRAME_MASK	0x04
#define SBUS_FAILSAFE_MASK	0x08

#define SBUS_THRESHOLD		500
#define SBUS_DROPPED_FRAMES	3
#define SBUS_TIMEOUT		(SBUS_PERIOD * SBUS_DROPPED_FRAMES)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

uint16_t SBUS_Transform (uint16_t);
bool SBUS_HandleUART (void);

/*
 * PRIVATE VARIABLES
 */

uint8_t rxSBUS[2*SBUS_PAYLOAD_LEN] = {0};
SBUS_Data dataSBUS = {0};

/*
 * PUBLIC FUNCTIONS
 */

bool SBUS_Detect(uint32_t baud)
{
	bool retVal = false;

	SBUS_Init(baud);

	uint32_t tick = CORE_GetTick();
	while ((SBUS_TIMEOUT * 2) > CORE_GetTick() - tick)
	{
		if (SBUS_HandleUART())
		{
			retVal = true;
		}
		CORE_Idle();
	}

	SBUS_Deinit();

	return retVal;
}

void SBUS_Init (uint32_t baud)
{
	memset(rxSBUS, 0, sizeof(rxSBUS));

	UART_Init(SBUS_UART, baud, UART_Mode_Inverted);
	UART_ReadFlush(SBUS_UART);
}

void SBUS_Deinit (void)
{
	UART_Deinit(SBUS_UART);
}

void SBUS_Update (void)
{
	// Update Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t prev = 0;

	// Check for New Input Data
	if (SBUS_HandleUART())
	{
		dataSBUS.ch[0]  = SBUS_Transform( (int16_t)( (rxSBUS[1]		  | rxSBUS[2] << 8 ) & 0x07FF) );
		dataSBUS.ch[1]  = SBUS_Transform( (int16_t)( (rxSBUS[2]  >> 3 | rxSBUS[3] << 5 ) & 0x07FF) );
		dataSBUS.ch[2]  = SBUS_Transform( (int16_t)( (rxSBUS[3]  >> 6 | rxSBUS[4] << 2  | rxSBUS[5] << 10 ) & 0x07FF) );
		dataSBUS.ch[3]  = SBUS_Transform( (int16_t)( (rxSBUS[5]  >> 1 | rxSBUS[6] << 7 ) & 0x07FF) );
		dataSBUS.ch[4]  = SBUS_Transform( (int16_t)( (rxSBUS[6]  >> 4 | rxSBUS[7] << 4 ) & 0x07FF) );
		dataSBUS.ch[5]  = SBUS_Transform( (int16_t)( (rxSBUS[7]  >> 7 | rxSBUS[8] << 1 | rxSBUS[9] << 9 ) & 0x07FF) );
		dataSBUS.ch[6]  = SBUS_Transform( (int16_t)( (rxSBUS[9]  >> 2 | rxSBUS[10] << 6 ) & 0x07FF) );
		dataSBUS.ch[7]  = SBUS_Transform( (int16_t)( (rxSBUS[10] >> 5 | rxSBUS[11] << 3 ) & 0x07FF) );

		dataSBUS.ch[8]  = SBUS_Transform( (int16_t)( (rxSBUS[12]		 | rxSBUS[13] << 8 ) & 0x07FF) );
		dataSBUS.ch[9]  = SBUS_Transform( (int16_t)( (rxSBUS[13] >> 3 | rxSBUS[14] << 5 ) & 0x07FF) );
		dataSBUS.ch[10] = SBUS_Transform( (int16_t)( (rxSBUS[14] >> 6 | rxSBUS[15] << 2  | rxSBUS[16] << 10 ) & 0x07FF) );
		dataSBUS.ch[11] = SBUS_Transform( (int16_t)( (rxSBUS[16] >> 1 | rxSBUS[17] << 7 ) & 0x07FF) );
		dataSBUS.ch[12] = SBUS_Transform( (int16_t)( (rxSBUS[17] >> 4 | rxSBUS[18] << 4 ) & 0x07FF) );
		dataSBUS.ch[13] = SBUS_Transform( (int16_t)( (rxSBUS[18] >> 7 | rxSBUS[19] << 1 | rxSBUS[20]) & 0x07FF) );
		dataSBUS.ch[14] = SBUS_Transform( (int16_t)( (rxSBUS[20] >> 2 | rxSBUS[21] << 6 ) & 0x07FF) );
		dataSBUS.ch[15] = SBUS_Transform( (int16_t)( (rxSBUS[21] >> 5 | rxSBUS[22] << 3 ) & 0x07FF) );

		dataSBUS.ch17      = rxSBUS[23] & SBUS_CH17_MASK;
		dataSBUS.ch17      = rxSBUS[23] & SBUS_CH18_MASK;
		dataSBUS.failsafe  = rxSBUS[23] & SBUS_FAILSAFE_MASK;
		dataSBUS.frameLost = rxSBUS[23] & SBUS_LOSTFRAME_MASK;

		prev = now;
	}

	// Check Failsafe

	if (dataSBUS.failsafe)
	{
		dataSBUS.inputLost = true;
	}
	else
	{
		if (SBUS_TIMEOUT <= (now - prev)) {
			dataSBUS.inputLost = true;
			memset(rxSBUS, 0, sizeof(rxSBUS));
		} else {
			dataSBUS.inputLost = false;
		}
	}
}

SBUS_Data* SBUS_GetDataPtr (void)
{
	return &dataSBUS;
}

/*
 * PRIVATE FUNCTIONS
 */

uint16_t SBUS_Transform (uint16_t r)
{
	uint32_t retVal = 0;

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

	if (retVal)
	{
		retVal *= SBUS_MAP_RANGE;
		retVal /= SBUS_RANGE;
		retVal += SBUS_MAP_MIN;
		retVal -= (SBUS_MIN * SBUS_MAP_RANGE / SBUS_RANGE);
	}

	return (uint16_t)retVal;
}


bool SBUS_HandleUART (void)
{
	static uint8_t byte_p = SBUS_FOOTER;
	static bool detected = false;
	bool retVal = false;

	if ( !detected ) // Not detected header
	{
		while (UART_ReadCount(SBUS_UART))
		{
			UART_Read(SBUS_UART, &rxSBUS[SBUS_HEADER_INDEX], SBUS_HEADER_LEN);
			bool b = rxSBUS[SBUS_HEADER_INDEX] == SBUS_HEADER;
			bool b_p = byte_p == SBUS_FOOTER;
			byte_p = rxSBUS[SBUS_HEADER_INDEX];
			if (b && b_p)
			{
				detected = true;
				break;
			}
		}
	}

	if ( detected )  // header has been detected
	{
		if ( UART_ReadCount(SBUS_UART) >= (SBUS_PAYLOAD_LEN - SBUS_HEADER_LEN) )
		{
			UART_Read(SBUS_UART, &rxSBUS[SBUS_DATA_INDEX], (SBUS_PAYLOAD_LEN - SBUS_HEADER_LEN));
			//
			byte_p = rxSBUS[SBUS_FOOTER_INDEX];
			//
			retVal = true;
			// Reset the detected flag
			detected = false;
		}
	}

	return retVal;
}

/*
 * INTERRUPT ROUTINES
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
