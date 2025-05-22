#include "MicroBitConfig.h"

#if !CONFIG_ENABLED(DEVICE_BLE)
#else

#include "CustomBtService.h"
#include "MicroBitLog.h"
#include "ExternalEvents.h"
#include "MicroBitFiber.h"
#include "ErrorNo.h"
#include "NotifyEvents.h"

using namespace codal;

const uint16_t MicroBitLogService::serviceUUID               = 0x0754;
const uint16_t MicroBitLogService::charUUID[ mbbs_cIdxCOUNT] = { 
    0xca4c,//rowcount
    0xfb25,//headercount
    0xfb58,//getheaders
    0xfb57,//getRowN
    0xfb59//newrowlogged
};



#define MICROBIT_FASTLOG_MAX_ROW_ELEMS 20

MicroBitLogService::MicroBitLogService(BLEDevice &_ble,MicroBitLog &_log):log(_log) {

    rowCount=0;
    headerCount = 0;

    rowBuffer=(float*)malloc(sizeof(float)*MICROBIT_FASTLOG_MAX_ROW_ELEMS);


    newRowBuffer=new BluetoothSendBuffer(MICROBIT_FASTBT_MAX_BYTES*4,mbbs_cIdxNEWROWLOGGED);
    getHeaderBuffer=new BluetoothSendBuffer(MICROBIT_FASTBT_MAX_BYTES*4,mbbs_cIdxGETHEADER);


    RegisterBaseUUID(bs_base_uuid);
    CreateService(serviceUUID);

    CreateCharacteristic(mbbs_cIdxROWCOUNT, charUUID[mbbs_cIdxROWCOUNT],
                         (uint8_t *)&rowCount, sizeof(rowCount), sizeof(rowCount),
                         microbit_propREAD);


    CreateCharacteristic(mbbs_cIdxGETHEADER, charUUID[mbbs_cIdxGETHEADER],
                         getHeaderBuffer->txBuffer,0, MICROBIT_FASTBT_MAX_BYTES,
                         microbit_propWRITE|microbit_propINDICATE);

    CreateCharacteristic(mbbs_cIdxHEADERCOUNT, charUUID[mbbs_cIdxHEADERCOUNT],
                         (uint8_t *)&headerCount, sizeof(headerCount), sizeof(headerCount),
                         microbit_propREAD | microbit_propNOTIFY);

    CreateCharacteristic(mbbs_cIdxGETROW, charUUID[mbbs_cIdxGETROW],
                         (uint8_t *)rowBuffer, 0, sizeof(float)*MICROBIT_FASTLOG_MAX_ROW_ELEMS,
                         microbit_propREAD | microbit_propNOTIFY|microbit_propWRITE);

    CreateCharacteristic(mbbs_cIdxNEWROWLOGGED, charUUID[mbbs_cIdxNEWROWLOGGED],
                         newRowBuffer->txBuffer, 0, MICROBIT_FASTBT_MAX_BYTES,
                         microbit_propINDICATE);
    if (getConnected()){
        listen(true);
    }
}

void MicroBitLogService::listen(bool yes) {
    if (EventModel::defaultEventBus) {
        if (yes) {
            rowCount=12;
            // headerCount=0;
            // rowCount = log.getNumberOfRows();
            headerCount = log.getNumberOfHeaders();
            // EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_FASTLOG_EVT_NEW_ROW, this, &MicroBitLogService::incrementLogRowCount,MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_HEADER, this, &MicroBitLogService::logHeaderUpdate);
            // if(enableLiveRowTransmission){
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::newRowLogged,MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
            // }
        }else {
            // EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_FASTLOG_EVT_NEW_ROW, this, &MicroBitLogService::incrementLogRowCount);
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::newRowLogged);
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_HEADER, this, &MicroBitLogService::logHeaderUpdate);
        }
    }
}

void MicroBitLogService::onConnect(const microbit_ble_evt_t *p_ble_evt) {
    listen(true);
}

void MicroBitLogService::onDisconnect(const microbit_ble_evt_t *p_ble_evt) {
    listen(false);
}



void MicroBitLogService::_sendNextFromBuffer(BluetoothSendBuffer* buf) {
    if (buf->bytesToSend != 0 || buf->tail == buf->head){
        return;
    }

    if (!getConnected() || !indicateChrValueEnabled(buf->characteristicId)){
        return;
    }

    int txNext = buf->tail;
    while (buf->bytesToSend < MICROBIT_FASTBT_MAX_BYTES && txNext != buf->head) {
        buf->txBuffer[buf->bytesToSend++] = buf->buffer[txNext];
        txNext = (txNext + 1) % buf->bufferSize;
    }

    indicateChrValue(buf->characteristicId, buf->txBuffer, buf->bytesToSend);
}

void MicroBitLogService::_sendToBuffer(BluetoothSendBuffer* buf, const uint8_t* data, int len) {
    if (len < 1){
        return;
    }
    if(!getConnected() || !indicateChrValueEnabled(buf->characteristicId)){
        return;
    }

    int written = 0;
    while (getConnected() && indicateChrValueEnabled(buf->characteristicId)) {
        while (written < len) {
            int nextHead = (buf->head + 1) % buf->bufferSize;
            if (nextHead == buf->tail){
                break;
            }
            buf->buffer[buf->head] = data[written++];
            buf->head = nextHead;
        }
        _sendNextFromBuffer(buf);
        break;
    }
}
void MicroBitLogService::_advanceBufferTail(BluetoothSendBuffer* buf) {
    buf->tail = (buf->tail + buf->bytesToSend) % buf->bufferSize;
    buf->bytesToSend = 0;
}
void MicroBitLogService::onConfirmation( const microbit_ble_evt_hvc_t *params) {
    if ( params->handle == valueHandle( mbbs_cIdxGETHEADER)) {
        _advanceBufferTail(getHeaderBuffer);
        _sendNextFromBuffer(getHeaderBuffer);
    } else if ( params->handle == valueHandle( mbbs_cIdxNEWROWLOGGED)) {
        _advanceBufferTail(newRowBuffer);
        _sendNextFromBuffer(newRowBuffer);
    }
}

/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitLogService::onDataWritten(const microbit_ble_evt_write_t *params)
{
    if (params->handle == valueHandle(mbbs_cIdxNEWROWLOGGED)){
        uint16_t sentData;
        memcpy(&sentData,params->data,sizeof(sentData));
        if(sentData!=0){
            // enableLiveRowTransmission=true;
            // EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_FASTLOG_EVT_NEW_ROW, this, &MicroBitLogService::newRowLogged,MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
        }
    }else if (params->handle == valueHandle(mbbs_cIdxGETHEADER) && params->len >= sizeof(uint16_t)){
        uint16_t requestedHeader;
        memcpy(&requestedHeader,params->data,sizeof(requestedHeader));
        ManagedString returnedHeader = log.getHeader(requestedHeader);//0 indexed
        if(returnedHeader != ManagedString::EmptyString){
            returnedHeader=returnedHeader+"\n";
            _sendToBuffer(getHeaderBuffer,(uint8_t*)returnedHeader.toCharArray(),returnedHeader.length());
        }
    }else if (params->handle == valueHandle(mbbs_cIdxGETROW) && params->len >= sizeof(uint16_t)){
        // uint16_t requestedRow;
        // memcpy(&requestedRow,params->data,sizeof(requestedRow));
        // int getRowReturnVal = log.getRowFloats(requestedRow,rowBuffer,MICROBIT_LOGBT_MAX_SIZE);
        // if(getRowReturnVal>0){
        //     if(getRowReturnVal<MICROBIT_LOGBT_MAX_SIZE){
        //         rowBuffer[getRowReturnVal]=0x7fc00000;
        //     }
        //     notifyChrValue(mbbs_cIdxGETROW, (uint8_t*)rowBuffer, sizeof(float)*(getRowReturnVal+1));
        // }
    }
}


void MicroBitLogService::incrementLogRowCount(MicroBitEvent) {
    if (getConnected()){
        rowCount = log.getNumberOfRows();
        setChrValue(mbbs_cIdxROWCOUNT, (uint8_t*)&rowCount, sizeof(rowCount));
    }
}

void MicroBitLogService::newRowLogged(MicroBitEvent) {
    if (getConnected()){
        ManagedString row = log.getRow(-1);
        if(row != ManagedString::EmptyString){
            row = row+"\n";
            _sendToBuffer(newRowBuffer,(uint8_t*)row.toCharArray(),row.length());
        }
    }
}

void MicroBitLogService::logHeaderUpdate(MicroBitEvent) {
    if (getConnected())
    {
        headerCount = log.getNumberOfHeaders();
        notifyChrValue(mbbs_cIdxHEADERCOUNT, (uint8_t*)&headerCount, sizeof(headerCount));
    }
}

#endif

