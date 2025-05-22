#include "MicroBitFastLog.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"

#ifndef runningLocal
#include "MicroBit.h"
#include "MicroBitLog.h"
#include "ManagedString.h"
#include <new>
extern MicroBit uBit;

using namespace codal;
#endif
FastLog::FastLog(){
    this->status=0;
    this->columnCount=MICROBIT_FASTLOG_DEFAULT_COLUMNS;
    this->rowData=NULL;
}

void FastLog::init(){
    if (status & MICROBIT_FASTLOG_STATUS_INITIALIZED){
        return;
    }

    columnCount=MICROBIT_FASTLOG_DEFAULT_COLUMNS;
    rowData=(LogColumnEntry*)malloc(sizeof(LogColumnEntry)*columnCount);

    if(rowData==NULL){
    }

    for(int i=0;i<columnCount;i++){
        new (&rowData[i]) LogColumnEntry;
        rowData[i].type=0;
        rowData[i].key=ManagedString::EmptyString;
    }

    logger = new CircBuffer(1000);

    status |= MICROBIT_FASTLOG_STATUS_INITIALIZED;

}

void FastLog::beginRow(){
    init();

    if (status & MICROBIT_FASTLOG_STATUS_ROW_STARTED){
        endRow();
    }



    for(int i=0;i<columnCount;i++){
        rowData[i].type=0;
    }

    //for each do thing
    status |= MICROBIT_FASTLOG_STATUS_ROW_STARTED;
}

void FastLog::endRow(){
    if (!(status & MICROBIT_FASTLOG_STATUS_ROW_STARTED))
        return;

    init();
    for(int i=0;i<columnCount;i++){
        if(rowData[i].type!=0){
            if(rowData[i].type==1){
                logger->logVal((uint16_t)rowData[i].value.int16Val);
            }else if(rowData[i].type==2){
                logger->logVal((int32_t)rowData[i].value.int32Val);
            }else if(rowData[i].type==3){
                logger->logVal((float)rowData[i].value.floatVal);
            }
        }else{
            logger->logVal((uint16_t)0);
        }
    }

    // Event(MICROBIT_ID_LOG, MICROBIT_FASTLOG_EVT_NEW_ROW);
    status &= ~MICROBIT_FASTLOG_STATUS_ROW_STARTED;
}

void FastLog::logData(ManagedString key, float value){
    init();

    if (!(status & MICROBIT_FASTLOG_STATUS_ROW_STARTED))
        beginRow();

    for(int i=0;i<columnCount;i++){
        if(rowData[i].key==ManagedString::EmptyString){
            rowData[i].key=key;
            rowData[i].type=3;
            rowData[i].value.floatVal=value;
            // Event(MICROBIT_ID_LOG,MICROBIT_FASTLOG_EVT_HEADERS_CHANGED);
            return;
        }
        if(rowData[i].key==key){
            rowData[i].type=3;
            rowData[i].value.floatVal=value;
            return;
        }
    }
}

void FastLog::logData(ManagedString key, uint16_t value){
    init();

    if (!(status & MICROBIT_FASTLOG_STATUS_ROW_STARTED))
        beginRow();

    for(int i=0;i<columnCount;i++){
        if(rowData[i].key==ManagedString::EmptyString){
            rowData[i].key=key;
            rowData[i].type=1;
            rowData[i].value.int16Val=value;
            // Event(MICROBIT_ID_LOG,MICROBIT_FASTLOG_EVT_HEADERS_CHANGED);
            return;
        }
        if(rowData[i].key==key){
            rowData[i].type=1;
            rowData[i].value.int16Val=value;
            return;
        }
    }
}

void FastLog::logData(ManagedString key, int32_t value){
    init();

    if (!(status & MICROBIT_FASTLOG_STATUS_ROW_STARTED))
        beginRow();

    for(int i=0;i<columnCount;i++){
        if(rowData[i].key==ManagedString::EmptyString){
            rowData[i].key=key;
            rowData[i].type=2;
            rowData[i].value.int32Val=value;
            // Event(MICROBIT_ID_LOG,MICROBIT_FASTLOG_EVT_HEADERS_CHANGED);
            return;
        }
        if(rowData[i].key==key){
            rowData[i].type=2;
            rowData[i].value.int32Val=value;
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
    int entries = logger->getRowCount();
    int extraCells = entries%columnCount;
    return (entries - extraCells)/columnCount;
}

ManagedString FastLog::getRow(int row){
    if(row<0){
        int start = row*columnCount;
        int end = start+columnCount;
        ManagedString ret = logger->getElem(start);
        for(int i=start+1;i<end;i++){
            ret=ret+","+logger->getElem(i);
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

#ifdef runningLocal
void FastLog::saveLog(){
    int entries = logger.getRowCount();
    int extraCells = entries%columnCount;
    for(int i=0;i<entries-extraCells;i++){
        if(i%columnCount==0){
            if(i!=0){
                printf("end row\n");
            }
            printf("start row\n");
        }
        logger.printElem(i+extraCells);
    }
    printf("end row\n");
}
#else
void FastLog::saveLog(){
    init();
    MicroBitLog log = uBit.log;
    log.clear(true);
    log.setTimeStamp(codal::TimeStampFormat::None);
    log.setSerialMirroring(false);

    int entries = logger->getRowCount();
    int extraCells = entries%columnCount;
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
            ManagedString elem = logger->getElem(i+extraCells);
            log.logData(rowData[columnIndex].key,elem);
        }
        columnIndex++;
    }
    log.endRow();
}

#endif
