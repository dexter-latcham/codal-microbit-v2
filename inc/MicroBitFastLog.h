#ifndef MICROBIT_FASTLOG_H
#define MICROBIT_FASTLOG_H


#define MICROBIT_FASTLOG_DEFAULT_COLUMNS 3

#define MICROBIT_FASTLOG_STATUS_INITIALIZED     0x0001
#define MICROBIT_FASTLOG_STATUS_ROW_STARTED     0x0002

#include "stdint.h"
#include "MicroBitCircularBuffer.h"
#include "MicroBitLog.h"
#include "MicroBitLog.h"
#include "ManagedString.h"
#include "stdint.h"


namespace codal
{

enum ValueType {
    TYPE_NONE=0,
    TYPE_UINT16 = 1,
    TYPE_INT32 =2,
    TYPE_FLOAT=3
};

class LogColumnEntry
{
    public:
    ManagedString key;
    ValueType type;
    union value{
        float floatVal;
        int32_t int32Val;
        uint16_t int16Val;
    }value;
};

class FastLog{
    uint32_t status;
    int columnCount;
    struct LogColumnEntry* rowData;
    CircBuffer *logger;
    TimeStampFormat                 timeStampFormat;
    ManagedString timeStampHeading;

    public:
    FastLog(int comumns=MICROBIT_FASTLOG_DEFAULT_COLUMNS);

    void beginRow();
    void endRow();



    void logData(const char *key, int value);
    void logData(ManagedString key, int value);

    void logData(const char *key, uint16_t value);
    void logData(const char *key, int32_t value);
    void logData(const char *key, float value);
    void logData(ManagedString key, float value);
    void logData(ManagedString key, uint16_t value);
    void logData(ManagedString key, int32_t value);
    void saveLog();

    void setTimeStamp(TimeStampFormat format);
    ManagedString getHeaders();

    ManagedString getHeaders(int index);

    ManagedString getRow(int row);
    uint16_t getNumberOfRows();

    uint16_t getNumberOfHeaders();
private:
    void init();
    void _storeValue(ManagedString key, ValueType type, void* addr);
};


}
#endif
