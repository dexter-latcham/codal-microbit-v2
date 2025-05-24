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
    if(columns!=-1){
        this->status |= MICROBIT_FASTLOG_STATUS_USER_SET_COLS;
        this->columnCount=columns;
        this->timeStampFormat = TimeStampFormat::None;
        this->timeStampHeading = ManagedString::EmptyString;
    }else{
        this->columnCount=MICROBIT_FASTLOG_DEFAULT_COLUMNS;
        this->timeStampFormat = TimeStampFormat::Milliseconds;
        this->timeStampHeading = ManagedString("Time (milliseconds)");
    }

    this->rowData=NULL;
    this->logger=NULL;
    this->logStartTime=0;
    this->previousLogTime=0;
}

void FastLog::init(){
    if (status & MICROBIT_FASTLOG_STATUS_INITIALIZED){
        if(logger==NULL){
            logger = new CircBuffer(1000);
        }
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
    if (timeStampFormat == format && timeStampFormat == TimeStampFormat::None){
        return;
    }


    //can only disable timestamp if we have not yet logged a row
    //otherwise it is still recorded but ignored when time to save

    timeStampFormat=format;
    if(format==TimeStampFormat::None){
        if (!(status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED)){
            timeStampHeading=ManagedString::EmptyString;
        }
        return;
    }

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

    if (!(status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED)){
        timeStampHeading=timeStampHeadingTmp;
        return;
    }

    for(int i=0;i<columnCount;i++){
        if(rowData[i].key==timeStampHeading){
            rowData[i].key=timeStampHeadingTmp;
            timeStampHeading=timeStampHeadingTmp;
            return;
        }
    }
    timeStampHeading=timeStampHeadingTmp;
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


    if (!(status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED)){
        if (status & MICROBIT_FASTLOG_STATUS_USER_SET_COLS){
            if(timeStampFormat!=TimeStampFormat::None){
                LogColumnEntry* newRowData=(LogColumnEntry*)malloc(sizeof(LogColumnEntry)*columnCount+1);
                for(int j=0;j<columnCount+1;j++){
                    new (&newRowData[j]) LogColumnEntry;
                    newRowData[j].type=TYPE_NONE;
                    newRowData[j].key=rowData[j].key;
                    newRowData[j].value=rowData[j].value;
                }
                free(rowData);
                rowData=newRowData;
                columnCount=columnCount+1;
            }
        }
    }

    if(timeStampHeading!=ManagedString::EmptyString){
        CODAL_TIMESTAMP t = system_timer_current_time();
        if(previousLogTime==0){
            logStartTime=t;
            previousLogTime=t;
            logData(timeStampHeading,0);
        }else{
            CODAL_TIMESTAMP timeDiff = t - previousLogTime;
            previousLogTime=t;
            int msTime = timeDiff % (CODAL_TIMESTAMP) 1000000000;
            logData(timeStampHeading,msTime);
        }
    }

    //if this is the first row, special case
    if (!(status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED)){
        if (!(status & MICROBIT_FASTLOG_STATUS_USER_SET_COLS)){
            //we now know the number of columns a user needs, the number of currently occupied columns
            int count=0;
            while(count < columnCount && rowData[count].type!=TYPE_NONE){
                count+=1;
            }
            //if the number needed is less that what has been allocated, we reduce our allocation
            if(count!=columnCount){
                LogColumnEntry* newRowData=(LogColumnEntry*)malloc(sizeof(LogColumnEntry)*count);
                for(int j=0;j<count;j++){
                    new (&newRowData[j]) LogColumnEntry;
                    newRowData[j].type=rowData[j].type;
                    newRowData[j].key=rowData[j].key;
                    newRowData[j].value=rowData[j].value;
                }
                free(rowData);
                rowData=newRowData;
                columnCount=count;
            }
        }
        status |= MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED;
    }

    for(int i=0;i<columnCount;i++){
        if(rowData[i].type==TYPE_UINT16){
            logger->logVal((uint16_t)rowData[i].value.int16Val);
        }else if(rowData[i].type==TYPE_INT32){
            logger->logVal((int32_t)rowData[i].value.int32Val);
        }else if(rowData[i].type==TYPE_FLOAT){
            logger->logVal((float)rowData[i].value.floatVal);
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
void FastLog::logData(const char *key, uint16_t value) {
    return logData(ManagedString(key), value);
}

void FastLog::logData(const char *key, int32_t value) {
    return logData(ManagedString(key), value);
}
void FastLog::logData(const char *key, float value) {
    return logData(ManagedString(key), value);
}

void FastLog::logData(const char *key, double value) {
    return logData(ManagedString(key), (float)value);
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

void FastLog::logData(ManagedString key, uint16_t value){
    _storeValue(key,TYPE_UINT16,&value);
}
void FastLog::logData(ManagedString key, int32_t value){
    _storeValue(key,TYPE_INT32,&value);
}
void FastLog::logData(ManagedString key, double value){
    logData(key,(float)value);
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
            if(type==TYPE_UINT16){
                rowData[i].value.int16Val=*(uint16_t*)addr;
            }else if(type == TYPE_INT32){
                rowData[i].value.int32Val=*(int32_t*)addr;
            }else{
                rowData[i].value.floatVal=*(float*)addr;
            }
            return;
        }
    }
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


static ManagedString padString(ManagedString s, int digits) {
    ManagedString zero = "0";
    while(s.length() != digits)
        s = zero + s;

    return s;
}

ManagedString FastLog::_timeOffsetToString(int timeOffset){
    CODAL_TIMESTAMP totalTimeInMs = (logStartTime+(CODAL_TIMESTAMP)timeOffset) / (CODAL_TIMESTAMP)timeStampFormat;
    int billions = totalTimeInMs / (CODAL_TIMESTAMP) 1000000000;
    int units = totalTimeInMs % (CODAL_TIMESTAMP) 1000000000;
    int fraction = 0;

    if ((int)timeStampFormat > 1) {
        fraction = units % 100;
        units = units / 100;
        billions = billions / 100;
    }

    ManagedString u(units);
    ManagedString f(fraction);
    ManagedString s;
    f = padString(f, 2);

    if (billions) {
        s = s + billions;
        u = padString(u, 9);
    }

    s = s + u;

    // Add two decimal places for anything other than milliseconds.
    if ((int)timeStampFormat > 1){
        s = s + "." + f;
    }
    return s;
}

FastLog::~FastLog(){
    if(logger!=NULL){
        saveLog(true);
    }
    if(rowData!=NULL){
        free(rowData);
    }
}

void FastLog::saveLog(bool deleteLog){
    if (!(status & MICROBIT_FASTLOG_STATUS_INITIALIZED)){
        return;
    }
    if(logger==NULL){
        return;
    }

    int entries = logger->getElementCount();
    int extraCells = entries % columnCount;
    if(entries==0){
        return;
    }
    int columnIndex=0;

    int timeOffset=0;


    MicroBitLog log = uBit.log;
    // log.clear(true);
    log.setTimeStamp(codal::TimeStampFormat::None);
    log.setSerialMirroring(false);
    log.beginRow();

    if(timeStampHeading!=ManagedString::EmptyString){
        if(timeStampFormat!=TimeStampFormat::None){
            //if timestamp used, make sure this is the first column in the data log
            log.logData(timeStampHeading,0);
        }
    }

    for(int i=0;i<entries-extraCells;i++){
        if(i%columnCount==0){
            columnIndex=0;
            if(i!=0){
                log.endRow();
                log.beginRow();
            }
        }
        if(rowData[columnIndex].key!=ManagedString::EmptyString){
            returnedBufferElem returnedElem = logger->getElem(i+extraCells);
            if(rowData[columnIndex].key==timeStampHeading){
                if(timeStampFormat != TimeStampFormat::None){
                    if(returnedElem.type == TYPE_UINT16){
                         timeOffset+= (int)returnedElem.value.int16Val;
                    }else if(returnedElem.type == TYPE_INT32){
                         timeOffset+= (int)returnedElem.value.int32Val;
                    }else if(returnedElem.type == TYPE_FLOAT){
                         timeOffset+= (int)returnedElem.value.floatVal;
                    }
                    log.logData(rowData[columnIndex].key,_timeOffsetToString(timeOffset));
                }
            }else{
                log.logData(rowData[columnIndex].key,_bufferRetToString(returnedElem));
            }
        }
        columnIndex++;
    }
    log.endRow();
    if(deleteLog){
        delete logger;
        logger=NULL;
    }
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



