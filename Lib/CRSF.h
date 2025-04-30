
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef CRSF_H
#define CRSF_H
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "STM32X.h"

#include "UART.h"
#include "Core.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC DEFINITIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define CRSF_CH_NUM	16

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC TYPES      									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum
{
	CRSF_unknown = 0x00,
    CRSF_FRAMETYPE_GPS = 0x02,
//    CRSF_FRAMETYPE_GPS_TIME = 0x03,
//    CRSF_FRAMETYPE_GPS_EXT = 0x06,
    CRSF_FRAMETYPE_VARIO = 0x07,
    CRSF_FRAMETYPE_BATTERY_SENSOR = 0x08,
    CRSF_FRAMETYPE_BARO_ALTITUDE = 0x09,
    CRSF_FRAMETYPE_OPENTX_SYNC = 0x10,
//    CRSF_FRAMETYPE_AIRSPEED = 0x0A,
//    CRSF_FRAMETYPE_HEARTBEAT = 0x0B,
//    CRSF_FRAMETYPE_RPM = 0x0C,
//    CRSF_FRAMETYPE_TEMP = 0x0D,
//    CRSF_FRAMETYPE_VTX_TELEMETRY = 0x10,
    CRSF_FRAMETYPE_LINK_STATISTICS = 0x14,
    CRSF_FRAMETYPE_RC_CHANNELS = 0x16,
//    CRSF_FRAMETYPE_SUBSET_RC_CHANNELS_PACKED = 0x17,
//    CRSF_FRAMETYPE_LINK_RX_ID = 0x1C,
//    CRSF_FRAMETYPE_LINK_TX_ID = 0x1D,
    CRSF_FRAMETYPE_ATTITUDE = 0x1E,
//    CRSF_FRAMETYPE_MAVLINK_FC = 0x1F,
    CRSF_FRAMETYPE_FLIGHT_MODE = 0x21,
//    CRSF_FRAMETYPE_ESP_NOW_MSG = 0x22,
    CRSF_FRAMETYPE_PING_DEVICES = 0x28,
    CRSF_FRAMETYPE_DEVICE_INFO = 0x29,
    CRSF_FRAMETYPE_REQUEST_SETTINGS = 0x2A,
    CRSF_FRAMETYPE_COMMAND = 0x32,
    CRSF_FRAMETYPE_RADIO = 0x3A,
} CRSF_frameType_e;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* PUBLIC FUNCTIONS										*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void 		CRSF_Init 			( void );
void 		CRSF_Deinit 		( void );
bool 		CRSF_Detect 		( void );
void 		CRSF_Update 		( void );

uint32_t*	CRSF_getData		( void );
bool* 		CRSF_getInputLost	( void );

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* EXTERN DECLARATIONS									*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif /* CRSF_H */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
