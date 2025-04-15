/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "IBUS.h"

#if defined(RADIO_USE_IBUS)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


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
#define IBUS_TIMEOUT_FS		(IBUS_PERIOD * IBUS_DROPPED_FRAMES) // Failsafe timeout. How long radio lost before failsafe is activated
#define IBUS_TIMEOUT_IP		4 // Input timeout. How long after detecting message headers does remaining read timeout.


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


uint32_t	IBUS_Truncate	( uint32_t );
bool 		IBUS_Checksum	( void );
void 		IBUS_HandleUART	( void );


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


uint8_t 	rxIBUS[IBUS_PAYLOAD_LEN] = {0};
bool 		rxHeartbeatIBUS = false;
IBUS_Data	dataIBUS = {0};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
void IBUS_Init ( void )
{
	memset(rxIBUS, 0, sizeof(rxIBUS));
	rxHeartbeatIBUS = false;
	dataIBUS.inputLost = true;

	UART_Init(IBUS_UART, IBUS_BAUD, UART_Mode_Default);
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
void IBUS_Deinit ( void )
{
	UART_Deinit(IBUS_UART);
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
bool IBUS_Detect ( void )
{
	IBUS_Init();

	uint32_t tick = CORE_GetTick();
	while ((IBUS_TIMEOUT_FS * 2) > CORE_GetTick() - tick)
	{
		IBUS_Update();
		CORE_Idle();
	}

	if ( dataIBUS.inputLost )
	{
		IBUS_Deinit();
	}

	return !dataIBUS.inputLost;
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
void IBUS_Update ( void )
{
	// Update Rx Data
	IBUS_HandleUART();

	// Update Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t tick = 0;

	// Check for New Input Data
	if (rxHeartbeatIBUS)
	{
		// Assign Input to data Struct
		uint8_t ch = 0;
		for (uint8_t i = IBUS_DATA_INDEX; i < (IBUS_PAYLOAD_LEN - IBUS_CHECKSUM_LEN); i += 2)
		{
			uint32_t trunc = (int32_t)(rxIBUS[i] | rxIBUS[i+1] << 8);
			dataIBUS.ch[ch] = IBUS_Truncate(trunc);
			ch += 1;
		}
		// Reset Flags
		rxHeartbeatIBUS = false;
		dataIBUS.inputLost = false;
		tick = now;
	}

	// Check for Input Failsafe
	if (!dataIBUS.inputLost && IBUS_TIMEOUT_FS <= (now - tick)) { // If not receiving data and inputLost flag not set
		dataIBUS.inputLost = true;
		memset(rxIBUS, 0, sizeof(rxIBUS));
	}
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
IBUS_Data* IBUS_getDataPtr ( void )
{
	return &dataIBUS;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
uint32_t IBUS_Truncate ( uint32_t r )
{
	uint32_t retVal = 0;

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


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
bool IBUS_Checksum ( void )
{
	bool retVal = false;

	uint32_t cs = (int32_t)(rxIBUS[IBUS_CHECKSUM_INDEX] | (int32_t)rxIBUS[IBUS_CHECKSUM_INDEX + 1] << 8);
	uint32_t check = IBUS_CHECKSUM_START;
	check -= IBUS_HEADER1;
	check -= IBUS_HEADER2;
	for (uint8_t i = IBUS_DATA_INDEX; i < IBUS_CHECKSUM_INDEX; i++)
	{
		check -= rxIBUS[i];
	}

	if (cs == check)
	{
		retVal = true;
	}

	return retVal;
}


/*
 * TEXT
 *
 * INPUTS:
 * OUTPUTS:
 */
void IBUS_HandleUART ( void )
{
	// Init Loop Variables
	uint32_t now = CORE_GetTick();
	static uint32_t timeout = 0;
	static bool detH1 = false;
	static bool detH2 = false;

	// Check for Start of transmission (Header1)
	if ( !detH1 )
	{
		// Process All Available Bytes in Buffer Until Header1 Detected
		while ( UART_ReadCount(IBUS_UART) >= IBUS_HEADER1_LEN)
		{
			// Read in Next Byte
			UART_Read(IBUS_UART, &rxIBUS[IBUS_HEADER1_INDEX], IBUS_HEADER1_LEN);
			// Check if the Byte is the Message Header1
			if (rxIBUS[IBUS_HEADER1_INDEX] == IBUS_HEADER1) {
				detH1 = true;
				timeout = now + IBUS_TIMEOUT_IP;
				break;
			}
		}
	}

	// Header1 Detected, Check for Header2
	if ( detH1 && !detH2)
	{
		// Only Proceed When Byte in Buffer
		if ( UART_ReadCount(IBUS_UART) >= IBUS_HEADER2_LEN )
		{
			// Read in Next Byte
			UART_Read(IBUS_UART, &rxIBUS[IBUS_HEADER2_INDEX], IBUS_HEADER2_LEN);
			// Check if the Byte is the Message Header1
			if (rxIBUS[IBUS_HEADER2_INDEX] == IBUS_HEADER2) {
				detH2 = true;
				timeout = now + IBUS_TIMEOUT_IP;
			} else if (rxIBUS[IBUS_HEADER2_INDEX] == IBUS_HEADER1) { // Case for 2 sequential 0x20 bytes
				// Do nothing. Next loop will re-check for Header2
			} else {
				detH1 = false;
			}
		}

		// Check for a timeout
		if ( now > timeout )
		{
			detH1 = false;
			detH2 = false;
		}
	}

	// Both Headers Detected, Read Remaining Transmission
	if ( detH1 && detH2 )
	{
		// Only Proceed When Full Message is Ready
		if ( UART_ReadCount(IBUS_UART) >= (IBUS_PAYLOAD_LEN - IBUS_HEADER1_LEN - IBUS_HEADER2_LEN) )
		{
			// Read in Remaining Message
			UART_Read(IBUS_UART, &rxIBUS[IBUS_DATA_INDEX], (IBUS_PAYLOAD_LEN - IBUS_HEADER1_LEN - IBUS_HEADER2_LEN));
			// Verify the Checksum
			if (IBUS_Checksum()) {
				rxHeartbeatIBUS = true;
			}
			// Reset detect for next read weather or not CS is correct
			detH1 = false;
			detH2 = false;
		}

		// Check for a timeout
		if ( now > timeout)
		{
			detH1 = false;
			detH2 = false;
		}
	}
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EVENT HANDLERS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* INTERRUPT ROUTINES									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
