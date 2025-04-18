
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "CRSF.h"

#if defined(RADIO_USE_CRSF)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE DEFINITIONS                               */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


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
#define CRSF_TIMEOUT_IP              4

// UART port (adjust as needed)
#define CRSF_UART                    UART1

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


typedef struct {
    uint16_t ch[16];
    bool failsafe;
    bool frameLost;
    bool inputLost;
} CRSF_Data;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE PROTOTYPES                                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


uint16_t CRSF_Transform(uint16_t raw);
static uint8_t CRSF_CalcChecksum(uint8_t *data, uint8_t len);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE VARIABLES                                 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


static CRSF_Data dataCRSF = {0};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS                                  */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * CRSF_Init
 *
 * Initialize the UART and CRSF variables.
 */
void CRSF_Init(uint32_t baud)
{
    memset(&dataCRSF, 0, sizeof(dataCRSF));
    dataCRSF.inputLost = true;
    UART_Init(CRSF_UART, baud, UART_Mode_Normal);
    UART_ReadFlush(CRSF_UART);
}


/*
 * CRSF_Deinit
 *
 * Shut down the CRSF module.
 */
void CRSF_Deinit(void)
{
    UART_Deinit(CRSF_UART);
}


/*
 * CRSF_Detect
 *
 * Try to detect valid CRSF frames within a timeout period.
 */
bool CRSF_Detect(uint32_t baud)
{
    CRSF_Init(baud);
    uint32_t tick = CORE_GetTick();
    while ((CRSF_TIMEOUT_IP * 10) > (CORE_GetTick() - tick))
    {
        CRSF_Update(); // Process any available data.
        CORE_Idle();
    }
    if (dataCRSF.inputLost)
    {
        CRSF_Deinit();
    }
    return !dataCRSF.inputLost;
}


/*
 * CRSF_Update
 *
 * Efficiently processes UART input by continuously reading available bytes,
 * building the packet, validating it, and then decoding the channels.
 *
 * This implementation eliminates the need for separate heartbeat flags.
 */
void CRSF_Update(void)
{
    static uint8_t packetBuffer[CRSF_RC_PACKET_LEN] = {0};
    static uint8_t idx = 0;              // Current index in packetBuffer.
    static uint32_t packetTick = 0;      // Time when the first byte was received.
    uint32_t now = CORE_GetTick();

    // Process available bytes from the UART.
    while (UART_ReadCount(CRSF_UART) > 0)
    {
        uint8_t byte;
        UART_Read(CRSF_UART, &byte, 1);

        // If we are not in the middle of assembling a packet, wait for the header.
        if (idx == 0)
        {
            if (byte == CRSF_HEADER)
            {
                packetBuffer[0] = byte;
                idx = 1;
                packetTick = now;
            }
            // If byte is not a header, ignore it.
        }
        else  // Already started packet assembly.
        {
            packetBuffer[idx++] = byte;

            // Once we have at least the header and length, check if the packet is complete.
            if (idx == 2)
            {
                // Optionally, you could validate the expected length here.
            }
            if (idx >= 2)
            {
                // Expected packet length is defined by the length byte received.
                uint8_t expectedLength = CRSF_HEADER_LEN + CRSF_LENGTH_LEN + packetBuffer[CRSF_LENGTH_INDEX] + CRSF_CHECKSUM_LEN;
                if (idx >= expectedLength)
                {
                    // Verify checksum (all bytes except the last one).
                    if (CRSF_CalcChecksum(packetBuffer, expectedLength - 1) == packetBuffer[expectedLength - 1])
                    {
                        // Packet is valid; decode if it is an RC channels packet.
                        if (packetBuffer[CRSF_PAYLOAD_INDEX] == CRSF_RC_CHANNELS_TYPE)
                        {
                            // Decode channel data; decoding indices assume payload data starts at index 3.
                            dataCRSF.ch[0]  = CRSF_Transform((uint16_t)(((packetBuffer[3]        | packetBuffer[4] << 8)) & 0x07FF));
                            dataCRSF.ch[1]  = CRSF_Transform((uint16_t)(((packetBuffer[4] >> 3   | packetBuffer[5] << 5)) & 0x07FF));
                            dataCRSF.ch[2]  = CRSF_Transform((uint16_t)(((packetBuffer[5] >> 6   | packetBuffer[6] << 2  | packetBuffer[7] << 10)) & 0x07FF));
                            dataCRSF.ch[3]  = CRSF_Transform((uint16_t)(((packetBuffer[7] >> 1   | packetBuffer[8] << 7)) & 0x07FF));
                            dataCRSF.ch[4]  = CRSF_Transform((uint16_t)(((packetBuffer[8] >> 4   | packetBuffer[9] << 4)) & 0x07FF));
                            dataCRSF.ch[5]  = CRSF_Transform((uint16_t)(((packetBuffer[9] >> 7   | packetBuffer[10] << 1 | packetBuffer[11] << 9)) & 0x07FF));
                            dataCRSF.ch[6]  = CRSF_Transform((uint16_t)(((packetBuffer[11] >> 2  | packetBuffer[12] << 6)) & 0x07FF));
                            dataCRSF.ch[7]  = CRSF_Transform((uint16_t)(((packetBuffer[12] >> 5  | packetBuffer[13] << 3)) & 0x07FF));

                            dataCRSF.ch[8]  = CRSF_Transform((uint16_t)(((packetBuffer[14]        | packetBuffer[15] << 8)) & 0x07FF));
                            dataCRSF.ch[9]  = CRSF_Transform((uint16_t)(((packetBuffer[15] >> 3   | packetBuffer[16] << 5)) & 0x07FF));
                            dataCRSF.ch[10] = CRSF_Transform((uint16_t)(((packetBuffer[16] >> 6   | packetBuffer[17] << 2 | packetBuffer[18] << 10)) & 0x07FF));
                            dataCRSF.ch[11] = CRSF_Transform((uint16_t)(((packetBuffer[18] >> 1   | packetBuffer[19] << 7)) & 0x07FF));
                            dataCRSF.ch[12] = CRSF_Transform((uint16_t)(((packetBuffer[19] >> 4   | packetBuffer[20] << 4)) & 0x07FF));
                            dataCRSF.ch[13] = CRSF_Transform((uint16_t)(((packetBuffer[20] >> 7   | packetBuffer[21] << 1 | packetBuffer[22])) & 0x07FF));
                            dataCRSF.ch[14] = CRSF_Transform((uint16_t)(((packetBuffer[22] >> 2   | packetBuffer[23] << 6)) & 0x07FF));
                            dataCRSF.ch[15] = CRSF_Transform((uint16_t)(((packetBuffer[23] >> 5   | packetBuffer[24] << 3)) & 0x07FF));
                        }
                        // Mark data as valid.
                        dataCRSF.inputLost = false;
                    }
                    // Packet complete (whether valid or not), so reset the assembly state.
                    idx = 0;
                }
            }

            // If too much time has passed while building a packet, reset.
            if ((now - packetTick) > CRSF_TIMEOUT_IP)
            {
                idx = 0;
            }
        }
    }

    // Failsafe: If no valid packet has been decoded for a while, mark input as lost.
    static uint32_t lastValidTick = 0;
    if (!dataCRSF.inputLost)
    {
        lastValidTick = now;
    }
    if ((now - lastValidTick) >= (CRSF_TIMEOUT_IP * 10))
    {
        dataCRSF.inputLost = true;
    }
}


/*
 * CRSF_getDataPtr
 *
 * Returns a pointer to the latest CRSF data.
 */
CRSF_Data* CRSF_getDataPtr(void)
{
    return &dataCRSF;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PRIVATE FUNCTIONS                                 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


/*
 * CRSF_Transform
 *
 * Transforms the raw 11-bit channel data using calibration parameters.
 */
uint16_t CRSF_Transform(uint16_t raw)
{
    uint32_t retVal;
    if (raw < (CRSF_MIN - CRSF_THRESHOLD))
        retVal = 0;
    else if (raw < CRSF_MIN)
        retVal = CRSF_MIN;
    else if (raw <= CRSF_MAX)
        retVal = raw;
    else if (raw < (CRSF_MAX + CRSF_THRESHOLD))
        retVal = CRSF_MAX;
    else
        retVal = 0;

    if (retVal)
    {
        retVal *= CRSF_MAP_RANGE;
        retVal /= CRSF_RANGE;
        retVal += CRSF_MAP_MIN;
        retVal -= (CRSF_MIN * CRSF_MAP_RANGE / CRSF_RANGE);
    }
    return (uint16_t)retVal;
}


/*
 * CRSF_CalcChecksum
 *
 * Calculates a simple XOR checksum for a buffer of data.
 */
static uint8_t CRSF_CalcChecksum(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

#endif
