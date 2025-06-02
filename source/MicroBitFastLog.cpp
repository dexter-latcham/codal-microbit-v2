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

MicroBitFastLog::MicroBitFastLog(){
    this->status=0;

    this->columnCount=MICROBIT_FASTLOG_DEFAULT_COLUMNS;
    this->timeStampFormat = TimeStampFormat::Milliseconds;
    this->status |= MICROBIT_FASTLOG_TIMESTAMP_ENABLED;

    this->rowData=NULL;
    this->logger=NULL;
    this->logStartTime=0;
}

//we can only use a fixed number of columns
//it is suggested that this be provided when used
//if not, a default value of 5 is used
MicroBitFastLog::MicroBitFastLog(int columns){
    this->status=0;
    this->status |= MICROBIT_FASTLOG_STATUS_USER_SET_COLS;
    this->columnCount=columns;
    this->timeStampFormat = TimeStampFormat::None;

    this->rowData=NULL;
    this->logger=NULL;
    this->logStartTime=0;
}

MicroBitFastLog::MicroBitFastLog(int columns,int loggerBytes){
    this->status=0;
    if(columns!=-1){
        this->status |= MICROBIT_FASTLOG_STATUS_USER_SET_COLS;
        this->columnCount=columns;
    }else{
        this->columnCount=MICROBIT_FASTLOG_DEFAULT_COLUMNS;
    }
    this->timeStampFormat = TimeStampFormat::None;
    this->rowData=NULL;
    this->logger= new MicroBitCircularBuffer(loggerBytes);
    this->logStartTime=0;
}

void MicroBitFastLog::init(){
    if (status & MICROBIT_FASTLOG_STATUS_INITIALIZED){
        return;
    }

    if(rowData==NULL){
        rowData=(LogColumnEntry*)malloc(sizeof(LogColumnEntry)*columnCount);
    }

    if(rowData==NULL){
        return;
    }

    for(int i=0;i<columnCount;i++){
        new (&rowData[i]) LogColumnEntry;
        rowData[i].type=TYPE_NONE;
        rowData[i].key=ManagedString::EmptyString;
    }

    if(logger==NULL){
        logger = new MicroBitCircularBuffer();
    }

    logStartTime = system_timer_current_time();
    status |= MICROBIT_FASTLOG_STATUS_INITIALIZED;

}


void MicroBitFastLog::setTimeStamp(TimeStampFormat format){
    init();
    if (timeStampFormat == format && timeStampFormat == TimeStampFormat::None){
        return;
    }


    //can only disable timestamp if we have not yet logged a row
    //otherwise it is still recorded but ignored when time to save
    if (!(status & MICROBIT_FASTLOG_TIMESTAMP_ENABLED)){
        if (status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED){
            return;
        }
        status |= MICROBIT_FASTLOG_TIMESTAMP_ENABLED;
    }

    timeStampFormat=format;
    if(format==TimeStampFormat::None){
        if (!(status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED)){
            status &= ~MICROBIT_FASTLOG_TIMESTAMP_ENABLED;
        }
        return;
    }
}

void MicroBitFastLog::beginRow(){
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

void MicroBitFastLog::endRow(){
    if (!(status & MICROBIT_FASTLOG_STATUS_ROW_STARTED))
        return;

    init();


    //if this is the first row, special case
    if (!(status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED)){
        if (!(status & MICROBIT_FASTLOG_STATUS_USER_SET_COLS)){
            //we now know the number of columns a user needs, the number of currently occupied columns
            int count=0;
            while(count < columnCount && rowData[count].type!=TYPE_NONE){
                count+=1;
            }
            //we could free the existing row data and allocate a smaller buffer for row data
            //instead we just ignore the extra cells in row data to reduce malloc calls
            //wasted memory is negligable
            columnCount = count;
            // //if the number needed is less that what has been allocated, we reduce our allocation
            // if(count!=columnCount){
            //     LogColumnEntry* newRowData=(LogColumnEntry*)malloc(sizeof(LogColumnEntry)*count);
            //     for(int j=0;j<count;j++){
            //         new (&newRowData[j]) LogColumnEntry;
            //         newRowData[j].type=rowData[j].type;
            //         newRowData[j].key=rowData[j].key;
            //         newRowData[j].value=rowData[j].value;
            //     }
            //     free(rowData);
            //     rowData=newRowData;
            //     columnCount=count;
            // }
        }
        status |= MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED;
    }

    if(status & MICROBIT_FASTLOG_TIMESTAMP_ENABLED){
        CODAL_TIMESTAMP t = system_timer_current_time();
        CODAL_TIMESTAMP timeDiff = t - logStartTime;
        int msTime = (timeDiff % (CODAL_TIMESTAMP) 1000000000);
        logger->push(msTime);
    }
    for(int i=0;i<columnCount;i++){
        if(rowData[i].type==TYPE_UINT32){
            unsigned int val = rowData[i].value.uint32Val;
            logger->push(val);
        }else if(rowData[i].type==TYPE_INT32){
            int val = rowData[i].value.int32Val;
            logger->push(val);
            // logger->push((int32_t)rowData[i].value.int32Val);
        }else if(rowData[i].type==TYPE_FLOAT){
            logger->push((float)rowData[i].value.floatVal);
        }else{
            logger->push(0);
        }
    }
    status &= ~MICROBIT_FASTLOG_STATUS_ROW_STARTED;
}



void MicroBitFastLog::logData(const char *key, int value) {
    return logData(ManagedString(key), value);
}

void MicroBitFastLog::logData(const char *key, unsigned int value) {
    return logData(ManagedString(key), value);
}
void MicroBitFastLog::logData(const char *key, float value) {
    return logData(ManagedString(key), value);
}

void MicroBitFastLog::logData(const char *key, double value) {
    return logData(ManagedString(key), (float)value);
}

void MicroBitFastLog::logData(ManagedString key, int value) {
    if(value <0){
        int32_t val = (int32_t)value;
        return _storeValue(key,TYPE_INT32,&val);
    }else{
        uint32_t val = (uint32_t)value;
        return _storeValue(key,TYPE_UINT32,&val);
    }
}

void MicroBitFastLog::logData(ManagedString key, unsigned int value) {
    uint32_t val = (uint32_t)value;
    return _storeValue(key,TYPE_UINT32,&val);
}

void MicroBitFastLog::logData(ManagedString key, float value){
    _storeValue(key,TYPE_FLOAT,&value);
}

void MicroBitFastLog::logData(ManagedString key, double value){
    logData(key,(float)value);
}


void MicroBitFastLog::_storeValue(ManagedString key, ValueType type, void* addr){
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
            if(type==TYPE_UINT32){
                rowData[i].value.uint32Val=*(uint32_t*)addr;
            }else if(type == TYPE_INT32){
                rowData[i].value.int32Val=*(int32_t*)addr;
            }else{
                rowData[i].value.floatVal=*(float*)addr;
            }
            return;
        }
    }
    //if we reach this point there is no location in row data to store this item
    //if we havent yet saved any rows to the circular buffer, we can attempt to increase the number of columns
    //only do this if the user hasn't manually set the number of columns to use
    if (!(status & MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED)){
        if (!(status & MICROBIT_FASTLOG_STATUS_USER_SET_COLS)){
            //add 5 new columns as buffer to reduce future malloc calls
            int newColCount = columnCount+5;
            LogColumnEntry* newRowData=(LogColumnEntry*)malloc(sizeof(LogColumnEntry)*newColCount);
            for(int i=0;i<columnCount;i++){
                new (&newRowData[i]) LogColumnEntry;
                newRowData[i].type=rowData[i].type;
                newRowData[i].key=rowData[i].key;
                newRowData[i].value=rowData[i].value;
            }
            for(int i=columnCount;i<newColCount;i++){
                new (&newRowData[i]) LogColumnEntry;
                newRowData[i].type=TYPE_NONE;
                newRowData[i].key=ManagedString::EmptyString;
            }
            free(rowData);
            newRowData[columnCount].key=key;
            newRowData[columnCount].type=type;
            if(type==TYPE_UINT32){
                newRowData[columnCount].value.uint32Val=*(uint32_t*)addr;
            }else if(type == TYPE_INT32){
                newRowData[columnCount].value.int32Val=*(int32_t*)addr;
            }else{
                newRowData[columnCount].value.floatVal=*(float*)addr;
            }
            rowData=newRowData;
            columnCount=newColCount;
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
static ManagedString _bufferRetToString(circBufferElem ret){
    if(ret.type == TYPE_INT32){
        int intVal = (int)ret.value.int32Val;
        return ManagedString(intVal);
    }else if(ret.type == TYPE_UINT32){
        int intVal = (int)ret.value.uint32Val;
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

static ManagedString _timeToString(TimeStampFormat format, CODAL_TIMESTAMP startTimestamp, int timestampOffset){
    CODAL_TIMESTAMP totalTimeInMs = (startTimestamp+(CODAL_TIMESTAMP)timestampOffset) / (CODAL_TIMESTAMP)format;
    int billions = totalTimeInMs / (CODAL_TIMESTAMP) 1000000000;
    int units = totalTimeInMs % (CODAL_TIMESTAMP) 1000000000;
    int fraction = 0;

    if ((int)format > 1) {
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
    if ((int)format > 1){
        s = s + "." + f;
    }
    return s;
}

MicroBitFastLog::~MicroBitFastLog(){
    if(logger!=NULL){
        saveLog();
    }
    delete logger;
    logger = NULL;
    if(rowData!=NULL){
        free(rowData);
    }
    rowData=NULL;
}

static ManagedString _generateTimeFormatString(TimeStampFormat format){
    if(format == TimeStampFormat::Milliseconds){
        return ManagedString("Time (milliseconds)");
    }else if(format == TimeStampFormat::Seconds){
        return ManagedString("Time (seconds)");
    }else if(format == TimeStampFormat::Minutes){
        return ManagedString("Time (minutes)");
    }else if(format == TimeStampFormat::Hours){
        return ManagedString("Time (hours)");
    }else if(format == TimeStampFormat::Days){
        return ManagedString("Time (days)");
    }else if(format == TimeStampFormat::None){
        return ManagedString::EmptyString;
    }
}


void MicroBitFastLog::saveLog(){
    if (!(status & MICROBIT_FASTLOG_STATUS_INITIALIZED)){
        return;
    }
    MicroBitLog log = uBit.log;
    log.setTimeStamp(codal::TimeStampFormat::None);
    log.setSerialMirroring(false);

    int entries = logger->count();
    if(entries==0){
        return;
    }

    int extraCells = entries % columnCount;
    if(status & MICROBIT_FASTLOG_TIMESTAMP_ENABLED){
        extraCells = entries % (columnCount+1);
    }


    circBufferElem elem;
    int timeOffset;

    if(extraCells!=0){
        for(int i=0;i<extraCells;i++){
            elem = logger->pop();
        }
    }

    ManagedString timeStampString = _generateTimeFormatString(timeStampFormat);
    ManagedString timeString;
    while(logger->count()!=0){
        log.beginRow();
        if(status & MICROBIT_FASTLOG_TIMESTAMP_ENABLED){
            elem = logger->pop();
            if(timeStampFormat != TimeStampFormat::None){
                if(elem.type == TYPE_UINT32){
                     timeOffset= (int)elem.value.uint32Val;
                }else if(elem.type == TYPE_INT32){
                     timeOffset= (int)elem.value.int32Val;
                }else if(elem.type == TYPE_FLOAT){
                     timeOffset= (int)elem.value.floatVal;
                }
                timeString = _timeToString(timeStampFormat,logStartTime,timeOffset);
                log.logData(timeStampString,timeString);
            }
        }
        for(int cell=0;cell<columnCount;cell++){
            elem = logger->pop();
            log.logData(rowData[cell].key,_bufferRetToString(elem));
        }
        log.endRow();
    }
}
