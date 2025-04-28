
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "CRSF.h"

#ifdef RADIO_USE_CRSF

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                               */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef RADIO_USE_CRSF
#ifndef CRSF_UART
#error "error: CRSF_UART needs to be defined (e.g UART_1)"
#endif
#ifndef UART1_PINS
#error "error: UART1_PINS needs to be defined (e.g GPIO_PIN_7)"
#endif
#ifndef UART1_AF
#error "error: UART1_AF needs to be defined (e.g GPIO_AF0_USART1)"
#endif
#endif

#define CRSF_TICKS_TO_US(x)  ((x - 992) * 5 / 8 + 1500)
#define CRSF_US_TO_TICKS(x)  ((x - 1500) * 8 / 5 + 992)

#define CRSF_BAUD				420000
#define CRSF_PERIOD_MS			4
#define CRSF_TIMEOUT_PACKET_MS  1
#define CRSF_TIMEOUT_RADIO_MS	(CRSF_PERIOD_MS * 10)

#define CRSF_LEN_SYNC           1
#define CRSF_LEN_LENGTH         1
#define CRSF_LEN_CRC8       	1
#define CRSF_LEN_PACKET_MIN     3
#define CRSF_LEN_PACKET_MAX     64

#define CRSF_INDEX_SYNC         0
#define CRSF_INDEX_LENGTH       1
#define CRSF_INDEX_PAYLOAD      2

#define CRSF_SYNC          		0xC8

// Channel transformation constants (calibration values)
#define CRSF_MIN             	172
#define CRSF_MIN_1000           191
#define CRSF_CENTER				992
#define CRSF_MAX_2000           1792
#define CRSF_MAX                1811
#define CRSF_THRESHOLD          10
#define CRSF_MAP_RANGE          1000
#define CRSF_RANGE              (CRSF_MAX - CRSF_MIN)
#define CRSF_MAP_MIN            0

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static bool 	CRSF_DecodePayload	( void );
static uint8_t 	CRSF_CalcCRC8		( const uint8_t *, uint8_t );
static uint32_t	CRSF_Transform		( uint16_t );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t  rx[CRSF_LEN_PACKET_MAX]		= {0};
static uint32_t idx							= CRSF_INDEX_SYNC;
static uint32_t	packetStart					= 0;
static uint32_t	lastValidPacket 			= 0;
static uint8_t 	payloadLen 					= 0;

static uint32_t	data[CRSF_CH_NUM]			= {0};
static bool 	inputLost 					= true;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * CRSF_Init
 *  -
 */
void CRSF_Init ( void )
{
    memset( rx,   0, sizeof(rx) );
    idx 			= CRSF_INDEX_SYNC;
    packetStart		= 0;
   	lastValidPacket = 0;
    payloadLen 		= 0;

    memset( data, 0, sizeof(data) );
    inputLost = true;


    UART_Init(		CRSF_UART, CRSF_BAUD, UART_Mode_Default );
    UART_ReadFlush( CRSF_UART );
}

/*
 * CRSF_Deinit
 *  -
 */
void CRSF_Deinit ( void )
{
    UART_Deinit( CRSF_UART );
}

/*
 * CRSF_Detect
 *  -
 */
bool CRSF_Detect ( void )
{
    CRSF_Init();
    uint32_t start = CORE_GetTick();

    while ( (CORE_GetTick() - start) < CRSF_TIMEOUT_RADIO_MS )
    {
        CRSF_Update();
        CORE_Idle();
    }

    if ( inputLost )
        CRSF_Deinit();

    return inputLost;
}

/*
 * CRSF_Update
 *  -
 */
void CRSF_Update ( void )
{
    uint8_t byte;

    while ( UART_ReadCount(CRSF_UART) )
    {
        UART_Read(CRSF_UART, &byte, 1);

        // LOOKING FOR PROTOCOL SYNC BYTE
        if ( idx == CRSF_INDEX_SYNC ) { // want this check after length check so can cascade into it on a failed length check
			// Confirm data is CRSF sync byte
			if ( byte == CRSF_SYNC ) {
				rx[CRSF_INDEX_SYNC]	= byte;
				idx = CRSF_INDEX_LENGTH;
				packetStart = CORE_GetTick();
			}
		}

        // LOOK FOR VALID PROTOCOL LENGTH BYTE
        else if ( idx == CRSF_INDEX_LENGTH ) {
        	// Confirm valid packet length
        	if ( byte >= CRSF_LEN_PACKET_MIN && byte <= (CRSF_LEN_PACKET_MAX - CRSF_LEN_SYNC - CRSF_LEN_CRC8) ) {
            	rx[CRSF_INDEX_LENGTH] = byte;
            	payloadLen = CRSF_LEN_SYNC + CRSF_LEN_LENGTH + rx[CRSF_INDEX_LENGTH];
            	idx = CRSF_INDEX_PAYLOAD;
            // Failed check
        	} else {
        		idx = CRSF_INDEX_SYNC;
        	}
        }

        // WAIT TO READ IN FULL PAYLOAD
        else {
        	rx[idx] = byte;

        	// Check for complete payload Rx
        	if ( idx >= (payloadLen-1) )
        	{
        		uint8_t crc = CRSF_CalcCRC8( &rx[CRSF_INDEX_PAYLOAD], rx[CRSF_INDEX_LENGTH]-1 );
        		// Calculate checksum to verify Payload
         		if ( crc  == rx[payloadLen-1] ) {
         			if ( CRSF_DecodePayload() ) {
            			inputLost = false;
            			lastValidPacket = CORE_GetTick();
         			}
        		}
        		idx = CRSF_INDEX_SYNC;
        	}
        	else { idx++; }
        }
    }


	uint32_t now = CORE_GetTick();

	// TIMEOUT MID PAYLOAD - ABORT TRANSMISSION
	if ( idx != CRSF_INDEX_SYNC && (now - packetStart) >= CRSF_TIMEOUT_PACKET_MS ) {
		idx = CRSF_INDEX_SYNC;
	}
	// TIMEOUT WITH RADIO
	if ( !inputLost && (now - lastValidPacket) >= CRSF_TIMEOUT_RADIO_MS ) {
		inputLost = true;
		idx = CRSF_INDEX_SYNC;
	}
}

/*
 * CRSF_getDataPtr
 *  -
 */
uint32_t* CRSF_getDataPtr ( void )
{
    return data;
}

/*
 * CRSF_getInputLost
 *  -
 */
bool* CRSF_getInputLostPtr ( void )
{
    return &inputLost;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static bool CRSF_DecodePayload( void )
{
	switch ( rx[CRSF_INDEX_PAYLOAD] ) {

	case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
		// decode 16×11-bit channels from payload
		for ( int i = 0; i < CRSF_CH_NUM; i++ ) {
			uint32_t bitIndex = i * 11;
			uint32_t byteIndex = CRSF_INDEX_PAYLOAD + (bitIndex >> 3); // divide by 8 and offset
			uint32_t bitOffset = byteIndex & 7; // mod 8
			// Extract the 16 channels
			uint32_t raw = ( rx[byteIndex] >> bitOffset ) | ( rx[byteIndex + 1] << (8 - bitOffset) );
	        // If our 11-bit slice spans three bytes, grab the third byte too
			if ( bitOffset > 5 ) {
				raw |= (uint32_t)rx[byteIndex + 2] << (16 - bitOffset) ;
			}
			// Mask to 11 bits, transform, and store
			data[i] = CRSF_Transform(raw & 0x07FF);
		}
		break;

	case CRSF_FRAMETYPE_GPS:
		break;
	case CRSF_FRAMETYPE_GPS_TIME:
		break;
	case CRSF_FRAMETYPE_GPS_EXT:
		break;
	case CRSF_FRAMETYPE_VARIO:
		break;
	case CRSF_FRAMETYPE_BATTERY_SENSOR:
		break;
	case CRSF_FRAMETYPE_BARO_ALTITUDE:
		break;
	case CRSF_FRAMETYPE_AIRSPEED:
		break;
	case CRSF_FRAMETYPE_HEARTBEAT:
		break;
	case CRSF_FRAMETYPE_RPM:
		break;
	case CRSF_FRAMETYPE_TEMP:
		break;
	case CRSF_FRAMETYPE_VTX_TELEMETRY:
		break;
	case CRSF_FRAMETYPE_LINK_STATISTICS:
		break;
	case CRSF_FRAMETYPE_SUBSET_RC_CHANNELS_PACKED:
		break;
	case CRSF_FRAMETYPE_LINK_RX_ID:
		break;
	case CRSF_FRAMETYPE_LINK_TX_ID:
		break;
	case CRSF_FRAMETYPE_ATTITUDE:
		break;
	case CRSF_FRAMETYPE_MAVLINK_FC:
		break;
	case CRSF_FRAMETYPE_FLIGHT_MODE:
		break;
	case CRSF_FRAMETYPE_ESP_NOW_MSG:
		break;

	default:
		return false;
	}

	return true;
}


/*
 * CRSF_Transform
 *  -
 */
static uint32_t CRSF_Transform ( uint16_t raw )
{
    uint32_t pulse;

    if 		( raw < (CRSF_MIN - CRSF_THRESHOLD) )	{ pulse = 0; }
    else if ( raw <  CRSF_MIN_1000)                 { pulse = CRSF_MIN; }
    else if ( raw <= CRSF_MAX_2000)              	{ pulse = raw; }
    else if ( raw < (CRSF_MAX + CRSF_THRESHOLD) )	{ pulse = CRSF_MAX; }
    else                                        	{ pulse = 0; }

    if ( pulse ) {
    	pulse  = pulse * CRSF_MAP_RANGE / CRSF_RANGE;
    	pulse += CRSF_MAP_MIN;
    	pulse -= (CRSF_MIN * CRSF_MAP_RANGE / CRSF_RANGE);
    }
    return pulse;
}


/*
 * CRSF_CalcCRC8
 *  - CRC-8/D5 — initial 0, poly 0xD5, reflected = false
 */
static uint8_t CRSF_CalcCRC8 ( const uint8_t *data, uint8_t len )
{
    uint8_t crc = 0;

    for ( uint8_t i = 0; i < len; i++ )
    {
        crc ^= data[i];

        for ( uint8_t b = 0; b < 8; b++ )
        {
            if ( crc & 0x80) {
            	crc = (crc << 1) ^ 0xD5;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
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
