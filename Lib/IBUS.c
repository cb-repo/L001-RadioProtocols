/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "IBUS.h"

/*
 * PRIVATE DEFINITIONS
 */

#define IBUS_HEADER1_LEN	1
#define IBUS_HEADER2_LEN	1
#define IBUS_DATA_LEN		2
#define IBUS_CHECKSUM_LEN	2
#define IBUS_PAYLOAD_LEN	(IBUS_HEADER1_LEN + IBUS_HEADER2_LEN + (IBUS_DATA_LEN * IBUS_NUM_CHANNELS) + IBUS_CHECKSUM_LEN)

#define IBUS_HEADER1_INDEX	0
#define IBUS_HEADER2_INDEX	(IBUS_HEADER1_INDEX + IBUS_HEADER1_LEN)
#define IBUS_DATA_INDEX		(IBUS_HEADER2_INDEX + IBUS_HEADER2_LEN)
#define IBUS_CHECKSUM_INDEX	(IBUS_DATA_INDEX + (IBUS_DATA_LEN * IBUS_NUM_CHANNELS))

#define IBUS_HEADER1		0x20
#define IBUS_HEADER2		0x40
#define IBUS_CHECKSUM_START	0xFFFF

#define IBUS_JITTER_ARRAY	3		// Given 7ms payload period, ~21ms input lag
#define IBUS_THRESHOLD		500
#define IBUS_DROPPED_FRAMES	3
#define IBUS_TIMEOUT		(IBUS_PERIOD * IBUS_DROPPED_FRAMES)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

uint16_t IBUS_Truncate (uint16_t);
bool IBUS_Checksum(uint8_t);
void IBUS_HandleUART (void);

/*
 * PRIVATE VARIABLES
 */

uint8_t rxIBUS[IBUS_JITTER_ARRAY][IBUS_PAYLOAD_LEN] = {0};
bool rxHeartbeatIBUS = false;
IBUS_Data dataIBUS = {0};

/*
 * PUBLIC FUNCTIONS
 */

bool IBUS_Detect(void)
{
	IBUS_Init();

	uint32_t tick = CORE_GetTick();
	while ((IBUS_TIMEOUT * 2) > CORE_GetTick() - tick)
	{
		IBUS_HandleUART();
		CORE_Idle();
	}

	IBUS_Deinit();

	return rxHeartbeatIBUS;
}

void IBUS_Init (void)
{
	memset(rxIBUS, 0, sizeof(rxIBUS));

	UART_Init(IBUS_UART, IBUS_BAUD, UART_Mode_Default);
}

void IBUS_Deinit (void)
{
	UART_Deinit(IBUS_UART);
}

void IBUS_Update (void)
{
	// Update Rx Data
	IBUS_HandleUART();

	// Update Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t prev = 0;

	// Check for New Input Data
	if (rxHeartbeatIBUS)
	{
		// Average and Assign Input to data Struct
		for (uint8_t j = IBUS_DATA_INDEX; j < (IBUS_PAYLOAD_LEN - IBUS_CHECKSUM_LEN); j += 2)
		{
			uint8_t avg = 0;
			uint32_t ch = 0;
			for (uint8_t i = 0; i < IBUS_JITTER_ARRAY; i++)
			{
				uint16_t trunc = (int16_t)(rxIBUS[i][j] | rxIBUS[i][j+1] << 8);
				trunc = IBUS_Truncate(trunc);
				if (trunc != 0)
				{
					ch += trunc;
					avg += 1;
				}
			}
			ch /= avg;
			dataIBUS.ch[j] = ch;
		}
		rxHeartbeatIBUS = false;
		prev = now;
	}

	// Check for Input Failsafe
	if (IBUS_TIMEOUT <= (now - prev)) {
		dataIBUS.inputLost = true;
		memset(rxIBUS, 0, sizeof(rxIBUS));
	} else {
		dataIBUS.inputLost = false;
	}
}

IBUS_Data* IBUS_GetDataPtr (void)
{
	return &dataIBUS;
}

/*
 * PRIVATE FUNCTIONS
 */

uint16_t IBUS_Truncate (uint16_t r)
{
	uint16_t retVal = 0;

	if (r == 0) {
		retVal = 0;
	} else if (r < (IBUS_MIN - IBUS_THRESHOLD)) {
		retVal = 0;
	} else if (r < IBUS_MIN) {
		retVal = IBUS_MIN;
	} else if (r <= IBUS_MAX) {
		retVal = r;
	} else if (r < (IBUS_MAX + IBUS_THRESHOLD))	{
		retVal = IBUS_MAX;
	} else {
		retVal = 0;
	}

	return retVal;
}

bool IBUS_Checksum(uint8_t jitter)
{
	bool retVal = false;

	uint16_t cs = (int16_t)(rxIBUS[jitter][IBUS_CHECKSUM_INDEX] | rxIBUS[jitter][IBUS_CHECKSUM_INDEX + 1] << 8);
	uint16_t check = IBUS_CHECKSUM_START;
	for (uint8_t i = 0 ; i < (IBUS_PAYLOAD_LEN - IBUS_CHECKSUM_LEN); i++)
	{
		check -= rxIBUS[jitter][i];
	}

	if (cs == check) { retVal = true; }

	return retVal;
}

void IBUS_HandleUART (void)
{
	static uint8_t jitter = 0;			// Jitter-Smoothing Array Index

	while (UART_ReadCount(IBUS_UART) >= IBUS_PAYLOAD_LEN)
	{
		UART_Read(IBUS_UART, &rxIBUS[jitter][IBUS_HEADER1_INDEX], IBUS_HEADER1_LEN);
		if (rxIBUS[jitter][IBUS_HEADER1_INDEX] == IBUS_HEADER1)
		{
			UART_Read(IBUS_UART, &rxIBUS[jitter][IBUS_HEADER2_INDEX], IBUS_HEADER2_LEN);
			if (rxIBUS[jitter][IBUS_HEADER2_INDEX] == IBUS_HEADER2)
			{
				// Read Channel Data
				UART_Read(IBUS_UART, &rxIBUS[jitter][IBUS_DATA_INDEX], (IBUS_PAYLOAD_LEN - IBUS_HEADER1_LEN - IBUS_HEADER2_LEN));
				// Verify the Checksum
				if (IBUS_Checksum(jitter)) {
					// Kick Heartbeat
					rxHeartbeatIBUS = true;
					//Increment Jitter Array Index
					jitter += 1;
					if (jitter >= IBUS_JITTER_ARRAY) { jitter = 0; }
				}
			}
		}
	}
}

/*
 * INTERRUPT ROUTINES
 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
