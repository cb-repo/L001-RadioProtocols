
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
#define CRSF_TIMEOUT_PACKET_MS  2
#define CRSF_TIMEOUT_RADIO_MS	100//(CRSF_PERIOD_MS * 10)

#define CRSF_LEN_SYNC           1
#define CRSF_LEN_LENGTH         1
#define CRSF_LEN_TYPE       	1
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
#define CRSF_RANGE              (CRSF_MAX - CRSF_MIN)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                        */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                   */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static CRSF_frameType_e CRSF_Decode					( void );
static inline void	CRSF_DecodeFrame_ChannelsRC	( void );
static inline void 	CRSF_DecodeFrame_LinkStats 	( void );
static uint8_t 		CRSF_CalcCRC8				( const uint8_t *, uint8_t );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint32_t idx							= CRSF_INDEX_SYNC;
static uint32_t	packetStart					= 0;
static uint32_t	lastValidPacket 			= 0;
static uint8_t 	payloadLen 					= 0;

static uint8_t  rx[CRSF_LEN_PACKET_MAX]		= {0};
static uint32_t	data[CRSF_CH_NUM]			= {0};
static uint32_t	raw[CRSF_CH_NUM]			= {0};

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

    if ( inputLost ) {
        CRSF_Deinit();
    }

    return !inputLost;
}

/*
 * CRSF_Update
 *  -
 */
void CRSF_Update ( void )
{
	static uint32_t countGood =0;
	static uint32_t countBad =0;
	static uint32_t countBadCRC =0;
	static uint32_t countBadIL =0;
	static uint32_t countValidPacket =0;
	static uint32_t countotherPAcket =0;
    uint8_t byte;

    while ( UART_ReadCount(CRSF_UART) )
    {
        UART_Read(CRSF_UART, &byte, 1);

        // LOOKING FOR PROTOCOL SYNC BYTE
        if ( idx == CRSF_INDEX_SYNC ) { // want this check after length check so can cascade into it on a failed length check
			// Confirm data is CRSF sync byte
			if ( byte == CRSF_SYNC ) {
    		    memset( rx, 0, sizeof(rx) );
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
            	countGood++;
            // Failed check
        	} else {
        		idx = CRSF_INDEX_SYNC;
        		countBad++;
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
         			CRSF_frameType_e packet = CRSF_Decode();
         			if ( packet == CRSF_FRAMETYPE_RC_CHANNELS ) {
            			inputLost = false;
            			lastValidPacket = CORE_GetTick();
            			countValidPacket++;
//            		    memset( rx, 0, sizeof(rx) );
         			} else {
         				countotherPAcket++;
         			}
        		}
         		else {
         			countBadCRC ++;
         		}
        		idx = CRSF_INDEX_SYNC;
        	}
        	else { idx++; }
        }
    }


	uint32_t now = CORE_GetTick();

	if ( countGood >1000 ||
	 countBad >1000 ||
	 countBadCRC >1000 ||
	 countBadIL >1000 ||
	 countValidPacket >1000 ||
	 countotherPAcket >1000 )
	{
//		inputLost = true;
	}

	// TIMEOUT MID PAYLOAD - ABORT TRANSMISSION
	if ( idx != CRSF_INDEX_SYNC && (now - packetStart) >= CRSF_TIMEOUT_PACKET_MS ) {
		idx = CRSF_INDEX_SYNC;
	}
	// TIMEOUT WITH RADIO
	if ( !inputLost && (now - lastValidPacket) >= CRSF_TIMEOUT_RADIO_MS ) {
		inputLost = true;
		countBadIL++;
		idx = CRSF_INDEX_SYNC;
	}
}

/*
 * CRSF_getDataPtr
 *  -
 */
uint32_t* CRSF_getData ( void )
{
    return data;
}

/*
 * CRSF_getInputLost
 *  -
 */
bool* CRSF_getInputLost ( void )
{
    return &inputLost;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static CRSF_frameType_e CRSF_Decode ( void )
{
	switch ( rx[CRSF_INDEX_PAYLOAD] ) {

	case CRSF_FRAMETYPE_RC_CHANNELS:
		CRSF_DecodeFrame_ChannelsRC();
		return CRSF_FRAMETYPE_RC_CHANNELS;

	case CRSF_FRAMETYPE_LINK_STATISTICS:
		CRSF_DecodeFrame_LinkStats();
		return CRSF_FRAMETYPE_LINK_STATISTICS;

	case CRSF_FRAMETYPE_GPS:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_ATTITUDE:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_VARIO:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_BATTERY_SENSOR:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_BARO_ALTITUDE:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_OPENTX_SYNC:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_FLIGHT_MODE:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_PING_DEVICES:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_DEVICE_INFO:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_REQUEST_SETTINGS:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_COMMAND:
		return CRSF_unknown;
	case CRSF_FRAMETYPE_RADIO:
		return CRSF_unknown;
//	case CRSF_FRAMETYPE_GPS_TIME:
//		return false;
//	case CRSF_FRAMETYPE_GPS_EXT:
//		return false;
//	case CRSF_FRAMETYPE_AIRSPEED:
//		return false;
//	case CRSF_FRAMETYPE_HEARTBEAT:
//		return false;
//	case CRSF_FRAMETYPE_RPM:
//		return false;
//	case CRSF_FRAMETYPE_TEMP:
//		return false;
//	case CRSF_FRAMETYPE_VTX_TELEMETRY:
//		return false;
//	case CRSF_FRAMETYPE_SUBSET_RC_CHANNELS_PACKED:
//		return false;
//	case CRSF_FRAMETYPE_LINK_RX_ID:
//		return false;
//	case CRSF_FRAMETYPE_LINK_TX_ID:
//		return false;
//	case CRSF_FRAMETYPE_MAVLINK_FC:
//		return false;
//	case CRSF_FRAMETYPE_ESP_NOW_MSG:
//		return false;
	default:
		return CRSF_unknown;
	}

	return CRSF_unknown;
}

/*
 *
 */
static inline void CRSF_DecodeFrame_ChannelsRC ( void )
{
	// decode 16×11-bit channels from payload
	for ( int i = 0; i < CRSF_CH_NUM; i++ ) {
		uint32_t bitIndex = i * 11;
		uint32_t byteIndex = (CRSF_INDEX_PAYLOAD + CRSF_LEN_TYPE) + (bitIndex >> 3); // divide by 8 and offset
		uint32_t bitOffset = bitIndex & 7; // mod 8
		// Extract the 16 channels
		raw[i] = ( (rx[byteIndex] >> bitOffset) | (rx[byteIndex + 1] << (8 - bitOffset)) );
		// If our 11-bit slice spans three bytes, grab the third byte too
		if ( bitOffset > 5 ) {
			raw[i] |= (uint32_t)rx[byteIndex + 2] << (16 - bitOffset) ;
		}
		// Mask to 11 bits, transform, and store
		raw[i] &= 0x07FF;
		uint32_t convert = raw[i];
		// Bound data
	    if 		( convert < (CRSF_MIN - CRSF_THRESHOLD) )	{ convert = 0; }
	    else if ( convert <  CRSF_MIN_1000)                 { convert = CRSF_MIN_1000; }
	    else if ( convert <= CRSF_MAX_2000)              	{ /* do nothing*/ }
	    else if ( convert <= (CRSF_MAX + CRSF_THRESHOLD) )	{ convert = CRSF_MAX_2000; }
	    else                                       			{ convert = 0; }
		// transform
		if ( convert ) {
			convert -= CRSF_MIN_1000;
			convert *= 1000;//RADIO_CH_FULLSCALE;
			convert /= (CRSF_MAX_2000 - CRSF_MIN_1000);
			convert += 1000;//RADIO_CH_MIN;
		}
	    // store
		data[i] = convert;
	}
}


static inline void CRSF_DecodeFrame_LinkStats ( void )
{

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
