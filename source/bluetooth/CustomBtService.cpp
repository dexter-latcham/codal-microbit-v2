#include "MicroBitConfig.h"

#if !CONFIG_ENABLED(DEVICE_BLE)
#else

#include "CustomBtService.h"

using namespace codal;

const uint16_t MicroBitLogService::serviceUUID               = 0x0754;
const uint16_t MicroBitLogService::charUUID[ mbbs_cIdxCOUNT] = { 
    0xca4c,//rowcount
    0xfb25,//headercount
    0xfb58,//getheaders
    0xfb57,//getRowN
    0xfb59//newrowlogged
    };


#include "MicroBitLog.h"

MicroBitLogService::MicroBitLogService(BLEDevice &_ble,MicroBitLog &_log):log(_log) {
    headers[0]='\0';
    rowCount=0;
    headerCount = 0;
    rowBuffer=(float*)malloc(sizeof(float)*MICROBIT_LOGBT_MAX_SIZE);
    newRowBuffer=(float*)malloc(sizeof(float)*MICROBIT_LOGBT_MAX_SIZE);

    RegisterBaseUUID(bs_base_uuid);
    CreateService(serviceUUID);

    CreateCharacteristic(mbbs_cIdxROWCOUNT, charUUID[mbbs_cIdxROWCOUNT],
                         (uint8_t *)&rowCount, sizeof(rowCount), sizeof(rowCount),
                         microbit_propREAD);


    CreateCharacteristic(mbbs_cIdxGETHEADER, charUUID[mbbs_cIdxGETHEADER],
                         (uint8_t *)&headers, sizeof(headers), sizeof(headers),
                         microbit_propREAD | microbit_propWRITE|microbit_propNOTIFY);

    CreateCharacteristic(mbbs_cIdxHEADERCOUNT, charUUID[mbbs_cIdxHEADERCOUNT],
                         (uint8_t *)&headerCount, sizeof(headerCount), sizeof(headerCount),
                         microbit_propREAD | microbit_propNOTIFY);

    CreateCharacteristic(mbbs_cIdxGETROW, charUUID[mbbs_cIdxGETROW],
                         (uint8_t *)rowBuffer, 0, sizeof(float)*MICROBIT_LOGBT_MAX_SIZE,
                         microbit_propREAD | microbit_propNOTIFY);

    CreateCharacteristic(mbbs_cIdxNEWROWLOGGED, charUUID[mbbs_cIdxNEWROWLOGGED],
                         (uint8_t *)newRowBuffer, 0, sizeof(float)*MICROBIT_LOGBT_MAX_SIZE,
                         microbit_propREAD | microbit_propNOTIFY);
    if (getConnected())
        listen(true);
}

void MicroBitLogService::listen(bool yes) {
    if (EventModel::defaultEventBus)
    {
        if (yes)
        {
            rowCount = log.getNumberOfRows(0);
            headerCount = log.getNumberOfHeaders();
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::incrementLogRowCount,MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::newRowLogged,MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
            EventModel::defaultEventBus->listen(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_HEADERS_CHANGED, this, &MicroBitLogService::logHeaderUpdate);
        }
        else
        {
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::incrementLogRowCount);
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_NEW_ROW, this, &MicroBitLogService::newRowLogged);
            EventModel::defaultEventBus->ignore(MICROBIT_ID_LOG, MICROBIT_LOG_EVT_HEADERS_CHANGED, this, &MicroBitLogService::logHeaderUpdate);
        }
    }
}

void MicroBitLogService::onConnect(const microbit_ble_evt_t *p_ble_evt) {
    listen(true);
}

void MicroBitLogService::onDisconnect(const microbit_ble_evt_t *p_ble_evt) {
    listen(false);
}


/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitLogService::onDataWritten(const microbit_ble_evt_write_t *params)
{
    if (params->handle == valueHandle(mbbs_cIdxGETHEADER) && params->len >= sizeof(uint16_t)){
        uint16_t requestedHeader;
        memcpy(&requestedHeader,params->data,sizeof(requestedHeader));
        ManagedString headersNew = log.getHeaders();
        int start=0;
        int end=0;
        int currentHeaderNumber=0;
        while(currentHeaderNumber<requestedHeader&& start < headersNew.length()){
            if(headersNew.charAt(start)==','){
                currentHeaderNumber++;

            }
            start++;
        }
        int length=0;
        while((start+length)<headersNew.length() && headersNew.charAt(start+length)!=','){
            headers[length]=headersNew.charAt(start+length);
            length++;
        }
        if(length < MICROBIT_LOGBT_MAX_HEADER_LENGTH){
            headers[length]='\0';
        }
        notifyChrValue(mbbs_cIdxGETHEADER, (uint8_t*)&headers, length);
    }
}


void MicroBitLogService::incrementLogRowCount(MicroBitEvent) {
    if (getConnected()){
        rowCount = log.getNumberOfRows(0);
        setChrValue(mbbs_cIdxROWCOUNT, (uint8_t*)&rowCount, sizeof(rowCount));
    }
}

void MicroBitLogService::newRowLogged(MicroBitEvent) {
    if (getConnected()){
        int getRowReturnVal = log.getLastRowFloats(newRowBuffer,MICROBIT_LOGBT_MAX_SIZE);
        if(getRowReturnVal>0){
            notifyChrValue(mbbs_cIdxNEWROWLOGGED, (uint8_t*)newRowBuffer, sizeof(float)*getRowReturnVal);
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

