#include "MicroBitConfig.h"

#if !CONFIG_ENABLED(DEVICE_BLE)
#else

#include "MicroBitLogService.h"
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
    0xfb58,//getheaderN
    0xfb59,//getRowN
    0xfb60//newrowevent
};



MicroBitLogService::MicroBitLogService(BLEDevice &_ble,MicroBitLog &_log):log(_log) {
    rowCount=0;
    headerCount = 0;

    //custom classes to store circular buffers for when a string to send is larger than the bluetooth buffer
    newRowBuffer=new BluetoothSendBuffer(MICROBIT_LOGBT_MAX_BYTES*4,mbbs_cIdxNEWROWLOGGED);
    getHeaderBuffer=new BluetoothSendBuffer(MICROBIT_LOGBT_MAX_BYTES*4,mbbs_cIdxGETHEADER);
    getRowNBuffer=new BluetoothSendBuffer(MICROBIT_LOGBT_MAX_BYTES*4,mbbs_cIdxGETROW);

    RegisterBaseUUID(bs_base_uuid);
    CreateService(serviceUUID);

    CreateCharacteristic(mbbs_cIdxROWCOUNT, charUUID[mbbs_cIdxROWCOUNT],
                         (uint8_t *)&rowCount, sizeof(rowCount), sizeof(rowCount),
                         microbit_propREAD);


    CreateCharacteristic(mbbs_cIdxGETHEADER, charUUID[mbbs_cIdxGETHEADER],
                         getHeaderBuffer->txBuffer,0, MICROBIT_LOGBT_MAX_BYTES,
                         microbit_propWRITE|microbit_propINDICATE);

    CreateCharacteristic(mbbs_cIdxHEADERCOUNT, charUUID[mbbs_cIdxHEADERCOUNT],
                         (uint8_t *)&headerCount, sizeof(headerCount), sizeof(headerCount),
                         microbit_propREAD | microbit_propNOTIFY);

    CreateCharacteristic(mbbs_cIdxGETROW, charUUID[mbbs_cIdxGETROW],
                         getRowNBuffer->txBuffer,0, MICROBIT_LOGBT_MAX_BYTES,
                         microbit_propWRITE|microbit_propINDICATE);

    CreateCharacteristic(mbbs_cIdxNEWROWLOGGED, charUUID[mbbs_cIdxNEWROWLOGGED],
                         newRowBuffer->txBuffer, 0, MICROBIT_LOGBT_MAX_BYTES,
                         microbit_propINDICATE);
    if (getConnected()){
        listen(true);
    }
}

void MicroBitLogService::listen(bool yes) {
    if (EventModel::defaultEventBus) {
        if (yes) {
            rowCount = log.getNumberOfRows();
            headerCount = log.getNumberOfHeaders();
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::updateRowCount,MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::newRowLogged,MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_HEADER, this, &MicroBitLogService::newHeaderEvent);
        }else {
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::updateRowCount);
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::newRowLogged);
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_HEADER, this, &MicroBitLogService::newHeaderEvent);
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
    while (buf->bytesToSend < MICROBIT_LOGBT_MAX_BYTES && txNext != buf->head) {
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
    } else if ( params->handle == valueHandle( mbbs_cIdxGETROW)) {
        _advanceBufferTail(getRowNBuffer);
        _sendNextFromBuffer(getRowNBuffer);
    }
}

/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitLogService::onDataWritten(const microbit_ble_evt_write_t *params)
{
    if (params->handle == valueHandle(mbbs_cIdxNEWROWLOGGED)){
    }else if (params->handle == valueHandle(mbbs_cIdxGETHEADER) && params->len >= sizeof(uint16_t)){
        uint16_t requestedHeader;
        memcpy(&requestedHeader,params->data,sizeof(requestedHeader));
        ManagedString returnedHeader = log.getHeader(requestedHeader);//0 indexed
        if(returnedHeader != ManagedString::EmptyString){
            returnedHeader=returnedHeader+"\n";
            _sendToBuffer(getHeaderBuffer,(uint8_t*)returnedHeader.toCharArray(),returnedHeader.length());
        }
    }else if (params->handle == valueHandle(mbbs_cIdxGETROW) && params->len >= sizeof(uint16_t)){
        uint16_t requestedRow;
        memcpy(&requestedRow,params->data,sizeof(requestedRow));
        ManagedString returnedRow = log.getRow(requestedRow);//0 indexed
        if(returnedRow != ManagedString::EmptyString){
            returnedRow=returnedRow+"\n";
            _sendToBuffer(getRowNBuffer,(uint8_t*)returnedRow.toCharArray(),returnedRow.length());
        }
    }
}


void MicroBitLogService::updateRowCount(MicroBitEvent) {
    if (getConnected()){
        rowCount = log.getNumberOfRows();
        setChrValue(mbbs_cIdxROWCOUNT, (uint8_t*)&rowCount, sizeof(rowCount));
    }
}

void MicroBitLogService::newRowLogged(MicroBitEvent) {
    if (!getConnected() || !indicateChrValueEnabled(newRowBuffer->characteristicId)){
        return;
    }
    ManagedString row = log.getRow(-1);
    if(row != ManagedString::EmptyString){
        row = row+"\n";
        _sendToBuffer(newRowBuffer,(uint8_t*)row.toCharArray(),row.length());
    }
}

void MicroBitLogService::newHeaderEvent(MicroBitEvent) {
    if (getConnected())
    {
        headerCount = log.getNumberOfHeaders();
        notifyChrValue(mbbs_cIdxHEADERCOUNT, (uint8_t*)&headerCount, sizeof(headerCount));
    }
}

#endif

