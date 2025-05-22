
#include "CodalConfig.h"
#include "MicroBitFastLog.h"
#include "MicroBitLog.h"

#include "ManagedString.h"

using namespace codal;

#define MAX_ALLOC_SIZE 300

MicroBitFastLog::MicroBitFastLog(int numberColumns) {
    this->columnCount = numberColumns;
    this->status = 0;
    this->data = NULL;
    this->head = 0;
    this->count = 0;
    this->maxRows = 0;
}

void MicroBitFastLog::init() {
    if (status != 0) {
        return;
    }

    maxRows = MAX_ALLOC_SIZE / (sizeof(float) * columnCount);
    if (maxRows == 0) {
        return;
    }

    data = (float*)malloc(maxRows * columnCount * sizeof(float));
    if (data == NULL) {
        return;
    }

    head = 0;
    count = 0;
    status = 1;
}

void MicroBitFastLog::logRow(float* dataArray) {
    if (status == 0) {
        init();
    }

    int index = (head % maxRows) * columnCount;
    memcpy(&data[index], dataArray, columnCount * sizeof(float));
    head = (head + 1) % maxRows;

    if (count < maxRows) {
        count++;
    }
}

void MicroBitFastLog::getRow(float* returnArray, int rowNumber) {
    if (status == 0 || data == NULL || rowNumber < 0 || rowNumber >= count) {
        return;
    }

    int startRow = (head + maxRows - count) % maxRows;
    int rowIndex = (startRow + rowNumber) % maxRows;
    int index = rowIndex * columnCount;

    memcpy(returnArray, &data[index], columnCount * sizeof(float));
}

int MicroBitFastLog::getRowCount() {
    return count;
}

void MicroBitFastLog::writeToDisk(MicroBitLog &log){
    log.clear(true);
    log.setTimeStamp(codal::TimeStampFormat::None);
    log.setSerialMirroring(true);

    float* currentRow  = (float*)malloc(sizeof(float)*columnCount);
    ManagedString headings[5]={
        ManagedString("foa"),
        ManagedString("bab"),
        ManagedString("bac"),
        ManagedString("bad"),
        ManagedString("bae")
    };
    for(int i=0;i<count;i++){
        getRow(currentRow,i);
        log.beginRow();
        for(int j=0;j<columnCount;j++){
            log.logData(headings[j],currentRow[j]);
        }
        log.endRow();
    }
    free(currentRow);
}
