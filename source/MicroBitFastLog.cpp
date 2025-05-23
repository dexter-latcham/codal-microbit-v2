#include "MicroBitFastLog.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"

#include "MicroBit.h"
#include "ManagedString.h"

#include <new>
extern MicroBit uBit;

using namespace codal;

//we can only use a fixed number of columns
//it is suggested that this be provided when used
//if not, a default value of 5 is used
FastLog::FastLog(int columns){
    this->status=0;
    this->columnCount=columns;
    this->rowData=NULL;
    this->timeStampFormat = TimeStampFormat::None;
    this->timeStampHeading = ManagedString::EmptyString;
    this->logger=NULL;
}

void FastLog::init(){
    if (status & MICROBIT_FASTLOG_STATUS_INITIALIZED){
        return;
    }

    rowData=(LogColumnEntry*)malloc(sizeof(LogColumnEntry)*columnCount);
    if(rowData==NULL){
        //todo handling
    }

    for(int i=0;i<columnCount;i++){
        new (&rowData[i]) LogColumnEntry;
        rowData[i].type=TYPE_NONE;
        rowData[i].key=ManagedString::EmptyString;
    }

    logger = new CircBuffer(1000);

    status |= MICROBIT_FASTLOG_STATUS_INITIALIZED;

}


void FastLog::setTimeStamp(TimeStampFormat format){
    init();
    if (timeStampFormat == format && timeStampFormat == TimeStampFormat::None)
        return;

    //currently we only support ms
    format = TimeStampFormat::Milliseconds;

    ManagedString units;
    switch (format) {
    case TimeStampFormat::None:
        break;
    case TimeStampFormat::Milliseconds:
        units = "milliseconds";
        break;

    case TimeStampFormat::Seconds:
        units = "seconds";
        break;

    case TimeStampFormat::Minutes:
        units = "minutes";
        break;

    case TimeStampFormat::Hours:
        units = "hours";
        break;

    case TimeStampFormat::Days:
        units = "days";
        break;
    }
    ManagedString timeStampHeadingTmp = "Time (" + units + ")";

    for(int i=0;i<columnCount;i++){
        if(rowData[i].key==ManagedString::EmptyString){
            rowData[i].key=timeStampHeadingTmp;
            timeStampHeading=timeStampHeadingTmp;
            return;
        }
    }
}

void FastLog::beginRow(){
    init();

    if (status & MICROBIT_FASTLOG_STATUS_ROW_STARTED){
        endRow();
    }

    for(int i=0;i<columnCount;i++){
        rowData[i].type=TYPE_NONE;
    }

    //for each do thing
    status |= MICROBIT_FASTLOG_STATUS_ROW_STARTED;
}

void FastLog::endRow(){
    if (!(status & MICROBIT_FASTLOG_STATUS_ROW_STARTED))
        return;

    init();
    if(timeStampHeading!=ManagedString::EmptyString){
        CODAL_TIMESTAMP t = system_timer_current_time();
        int units = t % (CODAL_TIMESTAMP) 1000000000;
        logData(timeStampHeading,units);
    }

    for(int i=0;i<columnCount;i++){
        if(rowData[i].type!=TYPE_NONE){
            if(rowData[i].type==TYPE_UINT16){
                logger->logVal((uint16_t)rowData[i].value.int16Val);
            }else if(rowData[i].type==TYPE_INT32){
                logger->logVal((int32_t)rowData[i].value.int32Val);
            }else if(rowData[i].type==TYPE_FLOAT){
                logger->logVal((float)rowData[i].value.floatVal);
            }
        }else{
            logger->logVal((uint16_t)0);
        }
    }
    // Event(MICROBIT_ID_LOG, MICROBIT_FASTLOG_EVT_NEW_ROW);
    status &= ~MICROBIT_FASTLOG_STATUS_ROW_STARTED;
}



void FastLog::logData(const char *key, int value) {
    return logData(ManagedString(key), value);
}

void FastLog::logData(ManagedString key, int value) {
    if(value >=0 && value < 65535){
        return logData(key, (uint16_t) value);
    }else if(value >= INT32_MIN && value <= INT32_MAX){
        return logData(key, (int32_t) value);
    }else{
        return logData(key, (float) value);
    }
}

void FastLog::logData(const char *key, uint16_t value) {
    return logData(ManagedString(key), value);
}

void FastLog::logData(const char *key, int32_t value) {
    return logData(ManagedString(key), value);
}
void FastLog::logData(const char *key, float value) {
    return logData(ManagedString(key), value);
}
void FastLog::logData(ManagedString key, uint16_t value){
    _storeValue(key,TYPE_UINT16,&value);
}
void FastLog::logData(ManagedString key, int32_t value){
    _storeValue(key,TYPE_INT32,&value);
}
void FastLog::logData(ManagedString key, float value){
    _storeValue(key,TYPE_FLOAT,&value);
}


void FastLog::_storeValue(ManagedString key, ValueType type, void* addr){
    init();
    if (!(status & MICROBIT_FASTLOG_STATUS_ROW_STARTED)){
        beginRow();
    }

    for(int i=0;i<columnCount;i++){
        if(rowData[i].key==ManagedString::EmptyString){
            rowData[i].key=key;
        }
        if(rowData[i].key==key){
            rowData[i].type=type;
            switch(type){
                case TYPE_UINT16:
                    rowData[i].value.int16Val=*(uint16_t*)addr;
                    break;
                case TYPE_INT32:
                    rowData[i].value.int32Val=*(int32_t*)addr;
                    break;
                case TYPE_FLOAT:
                    rowData[i].value.floatVal=*(float*)addr;
                    break;
            }
            return;
        }
    }
}


uint16_t FastLog::getNumberOfHeaders(){
    init();
    uint16_t count=0;
    for(int i=0;i<columnCount;i++){
        if(rowData[i].key!=ManagedString::EmptyString){
            count++;
        }
    }
    return count;
}

uint16_t FastLog::getNumberOfRows(){
    init();
    int entries = logger->getElementCount();
    int extraCells = entries % columnCount;
    return (entries - extraCells) / columnCount;
}



static ManagedString floatToStr(float num){
    int integerPart = (int)num;
    int fractionPart = (int)((num-integerPart)*100000.0f);

    ManagedString ret(integerPart);
    ManagedString zero = "0";
    if(fractionPart!=0){
        ManagedString fracPart(fractionPart);
        while(fracPart.length()<5){
            fracPart=zero+fracPart;
        }
        ret=ret+"."+fracPart;
    }
    return ret;
}

ManagedString FastLog::_bufferRetToString(returnedBufferElem ret){
    if(ret.type == TYPE_UINT16){
        int intVal = (int)ret.value.int16Val;
        return ManagedString(intVal);
    }else if(ret.type == TYPE_INT32){
        int intVal = (int)ret.value.int32Val;
        return ManagedString(intVal);
    }else if(ret.type == TYPE_FLOAT){
        ManagedString floatStr=floatToStr(ret.value.floatVal);
        return ManagedString(floatStr);
    }
    return ManagedString::EmptyString;
}

ManagedString FastLog::getRow(int row){
    if(row<0){
        int start = row*columnCount;
        int end = start+columnCount;
        returnedBufferElem returnedElem = logger->getElem(start);
        ManagedString ret = _bufferRetToString(returnedElem);
        for(int i=start+1;i<end;i++){
            returnedElem = logger->getElem(i);
            ret=ret+","+_bufferRetToString(returnedElem);
        }
        return ret;
    }
    return ManagedString::EmptyString;
}

ManagedString FastLog::getHeaders(int index){
    init();
    if(index>=columnCount){
        return ManagedString::EmptyString;
    }
    return rowData[index].key;
}

ManagedString FastLog::getHeaders(){
    init();
    if(rowData[0].key==ManagedString::EmptyString){
        return ManagedString::EmptyString;
    }

    ManagedString result;
    result = result+rowData[0].key;
    for(uint32_t i=1;i<columnCount;i++){
        if(rowData[i].key!=ManagedString::EmptyString){
            result = result+","+rowData[i].key;
        }
    }
    return result;
}

void FastLog::saveLog(){
    init();
    MicroBitLog log = uBit.log;

    // log.clear(true);
    log.setTimeStamp(codal::TimeStampFormat::None);
    log.setSerialMirroring(false);

    if(timeStampHeading!=ManagedString::EmptyString){
        //if timestamp used, make sure this is the first column in the data log
        log.logData(timeStampHeading,ManagedString::EmptyString);
    }

    int entries = logger->getElementCount();
    int extraCells = entries % columnCount;
    int columnIndex=0;
    for(int i=0;i<entries-extraCells;i++){
        if(i%columnCount==0){
            columnIndex=0;
            if(i!=0){
                log.endRow();
            }
            log.beginRow();
        }
        if(rowData[columnIndex].key!=ManagedString::EmptyString){
            returnedBufferElem returnedElem = logger->getElem(i+extraCells);
            ManagedString elemString = _bufferRetToString(returnedElem);
            log.logData(rowData[columnIndex].key,elemString);
        }
        columnIndex++;
    }
    log.endRow();
}

