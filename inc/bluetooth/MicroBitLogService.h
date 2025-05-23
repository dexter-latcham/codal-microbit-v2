#ifndef MICROBIT_CUSTOM_SERVICE_H
#define MICROBIT_CUSTOM_SERVICE_H

#include "MicroBitConfig.h"

#if !CONFIG_ENABLED(DEVICE_BLE)
#else

#include "MicroBitBLEManager.h"
#include "MicroBitBLEService.h"
#include "MicroBitLog.h"
#include "EventModel.h"

#include "string.h"
#include "stdint.h"

#define MICROBIT_LOGBT_MAX_BYTES 20
class BluetoothSendBuffer {
public:
    uint8_t *buffer;
    uint8_t* txBuffer;

    int bufferSize;
    int head;
    int tail;
    int bytesToSend;
    int characteristicId;

    BluetoothSendBuffer(int size,int charId){
        characteristicId=charId;
        bufferSize=size;
        int allocSize = bufferSize + MICROBIT_LOGBT_MAX_BYTES;
        buffer = (uint8_t *)malloc(allocSize);
        memclr(buffer, allocSize);
        txBuffer=buffer+bufferSize;
        head = 0;
        tail=0;
        bytesToSend = 0;
    }
};

class MicroBitLogService : public MicroBitBLEService
{
public:
    MicroBitLogService(BLEDevice &_ble,codal::MicroBitLog &_log);
    MicroBitLog &log;

    void onConnect(const microbit_ble_evt_t *p_ble_evt) override;
    void onDisconnect(const microbit_ble_evt_t *p_ble_evt) override;

protected:
    void listen(bool yes);


    void updateRowCount(MicroBitEvent e);
    void newRowLogged(MicroBitEvent e);
    void newHeaderEvent(MicroBitEvent e);
    void onDataWritten(const microbit_ble_evt_write_t *params);

    void onConfirmation( const microbit_ble_evt_hvc_t *params);


    BluetoothSendBuffer *newRowBuffer;
    BluetoothSendBuffer *getHeaderBuffer;
    BluetoothSendBuffer *getRowNBuffer;

    bool enableLiveRowTransmission;
    
    uint16_t rowCount;
    uint16_t headerCount;

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

    void _advanceBufferTail(BluetoothSendBuffer* buf);
    void _sendToBuffer(BluetoothSendBuffer* buf, const uint8_t* data, int len);
    void _sendNextFromBuffer(BluetoothSendBuffer* buf);
};

#endif
#endif
