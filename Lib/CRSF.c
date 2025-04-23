
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
#ifndef UART1_GPIO
#error "error: UART1_GPIO needs to be defined (e.g GPIOB)"
#endif
#ifndef UART1_PINS
#error "error: UART1_PINS needs to be defined (e.g GPIO_PIN_7)"
#endif
#ifndef UART1_AF
#error "error: UART1_AF needs to be defined (e.g GPIO_AF0_USART1)"
#endif
#endif

#define CRSF_BAUD	420000

#define CRSF_HEADER_LEN              1
#define CRSF_LENGTH_LEN              1
#define CRSF_CHECKSUM_LEN            1

// For RC channels packet: Header (1) + Length (1) + Payload (23 bytes) + Checksum (1) = 26 bytes.
#define CRSF_RC_PACKET_PAYLOAD_LEN   23
#define CRSF_RC_PACKET_LEN           (CRSF_HEADER_LEN + CRSF_LENGTH_LEN + CRSF_RC_PACKET_PAYLOAD_LEN + CRSF_CHECKSUM_LEN)

#define CRSF_HEADER_INDEX            0
#define CRSF_LENGTH_INDEX            1
#define CRSF_PAYLOAD_INDEX           2

#define CRSF_HEADER                  0xC8
#define CRSF_RC_CHANNELS_TYPE        0x16

// Timeouts in milliseconds (adjust as needed)
#define CRSF_TIMEOUT_MS              4

// Channel transformation constants (calibration values)
#define CRSF_MIN                     172
#define CRSF_MAX                     1811
#define CRSF_THRESHOLD               50
#define CRSF_MAP_RANGE               1000
#define CRSF_RANGE                   (CRSF_MAX - CRSF_MIN)
#define CRSF_MAP_MIN                 0

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE TYPES                                     */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t 	CRSF_CalcCRC8	( const uint8_t *, uint8_t );
static uint32_t	CRSF_Transform	( uint16_t );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t  packetBuffer[CRSF_RC_PACKET_LEN];
static uint8_t  idx;
static uint32_t	packetTick;
static uint32_t	lastValidTick;

static uint32_t	data[CRSF_CH_NUM]	= {0};
static bool 	inputLost 			= {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * CRSF_Init
 *  -
 */
void CRSF_Init ( void )
{
    memset( &data, 0, sizeof(data) );
    inputLost = true;

    idx           = 0;
    packetTick    = 0;
    lastValidTick = 0;
    memset( packetBuffer, 0, sizeof(packetBuffer) );

    UART_Init(		CRSF_UART, CRSF_BAUD, UART_Mode_Default );
    UART_ReadFlush(	CRSF_UART );
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

    while ( (CORE_GetTick() - start) < (CRSF_TIMEOUT_MS * 10) )
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
    uint32_t now;

    // Process all bytes waiting in the UART FIFO
    while ( UART_ReadCount(CRSF_UART) > 0 )
    {
        UART_Read(CRSF_UART, &byte, 1);
        now = CORE_GetTick();

        // 1) Start of packet?
        if ( idx == 0 ) {
            if ( byte == CRSF_HEADER ) {
                packetBuffer[0]	= byte;
                idx           	= 1;
                packetTick    	= now;
            }
            continue;
        }

        // 2) Continue assembling
        packetBuffer[idx++] = byte;

        // 2a) on idx==2 we now have length — validate it
        if ( idx == 2 ) {
            if ( packetBuffer[CRSF_LENGTH_INDEX] > CRSF_RC_PACKET_PAYLOAD_LEN ) {
                idx = 0;  // bogus length ⇒ resync
                continue;
            }
        }

        // 2b) Once we've read header+length+payload+CRC, process
        {
            uint8_t expectedLen = CRSF_HEADER_LEN + CRSF_LENGTH_LEN + packetBuffer[CRSF_LENGTH_INDEX] + CRSF_CHECKSUM_LEN;

            if ( idx >= expectedLen ) {
                // CRC8 over Type+Payload
                if ( packetBuffer[CRSF_PAYLOAD_INDEX] == CRSF_RC_CHANNELS_TYPE && CRSF_CalcCRC8( &packetBuffer[CRSF_PAYLOAD_INDEX], packetBuffer[CRSF_LENGTH_INDEX] ) == packetBuffer[expectedLen-1] ) {
                    // decode 16×11-bit channels from payload
                    for ( int i = 0; i < 16; i++ ) {
                        uint32_t bitOff = i * 11;
                        uint32_t byteOff = CRSF_PAYLOAD_INDEX + (bitOff >> 3);
                        uint32_t bitIn  = bitOff & 7;

                        uint32_t raw = ( packetBuffer[byteOff] >> bitIn ) | ( packetBuffer[byteOff+1] << (8 - bitIn) );

                        if ( bitIn > 5 )
                            raw |= ( packetBuffer[byteOff+2] << (16 - bitIn) );

                        data[i] = CRSF_Transform(raw & 0x07FF);
                    }

                    // clear status flags (no failsafe bits in this packet)
                    inputLost  		= false;
                    lastValidTick	= now;
                }
                // done with this packet
                idx = 0;
            }
        }

        // 3) timeout while assembling?
        if ( (now - packetTick) > CRSF_TIMEOUT_MS )
        {
            idx = 0;
        }
    }

    // 4) overall input-lost timeout
    now = CORE_GetTick();
    if ( (now - lastValidTick) >= (CRSF_TIMEOUT_MS * 10) )
    {
        inputLost = true;
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


/*
 * CRSF_Transform
 *  -
 */
static uint32_t CRSF_Transform ( uint16_t raw )
{
    uint32_t v;

    if 		( raw < (CRSF_MIN - CRSF_THRESHOLD) )	{ v = 0; }
    else if ( raw < CRSF_MIN)                    	{ v = CRSF_MIN; }
    else if ( raw <= CRSF_MAX)                  	{ v = raw; }
    else if ( raw < (CRSF_MAX + CRSF_THRESHOLD) )	{ v = CRSF_MAX; }
    else                                        	{ v = 0; }

    if ( v ) {
    	v  = v * CRSF_MAP_RANGE / CRSF_RANGE;
    	v += CRSF_MAP_MIN;
    	v -= (CRSF_MIN * CRSF_MAP_RANGE / CRSF_RANGE);
    }
    return v;
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
