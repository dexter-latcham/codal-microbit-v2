#ifndef MICROBIT_CUSTOM_SERVICE_H
#define MICROBIT_CUSTOM_SERVICE_H

#include "MicroBitConfig.h"

#if !CONFIG_ENABLED(DEVICE_BLE)
#else

#include "MicroBitBLEManager.h"
#include "MicroBitBLEService.h"
#include "MicroBitLog.h"
#include "EventModel.h"

#include "MicroBitBLEManager.h"
#include "MicroBitBLEService.h"
#include "MicroBitSerial.h"


class MicroBitLogService : public MicroBitBLEService
{
public:
    MicroBitLogService(BLEDevice &_ble,codal::MicroBitLog &_log);
    MicroBitLog &log;

    void onConnect(const microbit_ble_evt_t *p_ble_evt) override;
    void onDisconnect(const microbit_ble_evt_t *p_ble_evt) override;

protected:
    void listen(bool yes);


    void incrementLogRowCount(MicroBitEvent e);
    void newRowLogged(MicroBitEvent e);
    void logHeaderUpdate(MicroBitEvent e);
    void onDataWritten(const microbit_ble_evt_write_t *params);

    void sendHeader(const uint8_t *header, int length);
    bool sendNextHeader();

    void sendRow(const uint8_t *row, int length);
    bool sendNextRow();
    void onConfirmation( const microbit_ble_evt_hvc_t *params);


    uint8_t SHBufferSize;
    uint8_t* SHBuffer;
    uint8_t SHBufferHead;
    uint8_t SHBufferTail;
    uint8_t SHBytesToSend;


    uint8_t rowBuffSize;
    uint8_t* rowBuff;
    uint8_t rowBuffHead;
    uint8_t rowBuffTail;
    uint8_t rowBuffToSend;

    // bool enableLiveRowTransmission;
    uint16_t rowCount;
    uint16_t headerCount;
    uint16_t requestedHeader;

    float* rowBuffer;
    float* newRowBuffer;

    typedef enum mbbs_cIdx
    {
        mbbs_cIdxROWCOUNT,
        mbbs_cIdxHEADERCOUNT,
        mbbs_cIdxGETHEADER,
        mbbs_cIdxGETROW,
        mbbs_cIdxNEWROWLOGGED,
        mbbs_cIdxCOUNT
    } mbbs_cIdx;

    static const uint16_t serviceUUID;
    static const uint16_t charUUID[ mbbs_cIdxCOUNT];
    MicroBitBLEChar      chars[ mbbs_cIdxCOUNT];

    int              characteristicCount()          { return mbbs_cIdxCOUNT; };
    MicroBitBLEChar *characteristicPtr( int idx)    { return &chars[ idx]; };
};

#endif
#endif
