#ifndef MICROBIT_FASTLOG_H
#define MICROBIT_FASTLOG_H


#define MICROBIT_FASTLOG_DEFAULT_COLUMNS 3

#define MICROBIT_FASTLOG_STATUS_INITIALIZED     0x0001
#define MICROBIT_FASTLOG_STATUS_ROW_STARTED     0x0002
#define MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED     0x0004
#define MICROBIT_FASTLOG_STATUS_USER_SET_COLS     0x0008
#define MICROBIT_FASTLOG_TIMESTAMP_ENABLED     0x0010

#include "stdint.h"
#include "MicroBitCircularBuffer.h"
#include "MicroBitLog.h"
#include "MicroBitLog.h"
#include "ManagedString.h"
#include "stdint.h"


namespace codal
{

class LogColumnEntry
{
    public:
    ManagedString key;
    ValueType type;
    union value{
        float floatVal;
        int32_t int32Val;
        uint32_t uint32Val;
    }value;
};

class MicroBitFastLog{
    uint32_t status;
    int columnCount;
    struct LogColumnEntry* rowData;
    MicroBitCircularBuffer *logger;
    TimeStampFormat                 timeStampFormat;
    CODAL_TIMESTAMP logStartTime;

    public:


    MicroBitFastLog(int columns,int loggerBytes);
    MicroBitFastLog(int columns);
    MicroBitFastLog();

    ~MicroBitFastLog();

    void beginRow();
    void endRow();




    void logData(const char *key, unsigned int value);
    void logData(ManagedString key, unsigned int value);

    void logData(const char *key, int value);
    void logData(ManagedString key, int value);

    void logData(const char *key, float value);
    void logData(ManagedString key, float value);

    void logData(const char *key, double value);
    void logData(ManagedString key, double value);

    void saveLog();

    void setTimeStamp(TimeStampFormat format);

private:
    void init();
    void _storeValue(ManagedString key, ValueType type, void* addr);
};


}
#endif

    // ManagedString getHeaders();

    // ManagedString getHeaders(int index);

    // ManagedString getRow(int row);
    // uint16_t getNumberOfRows();

    // uint16_t getNumberOfHeaders();
//

    // void logData(const char *key, double value);
    // void logData(ManagedString key, double value);

    // void logData(const char *key, uint16_t value);
    // void logData(ManagedString key, uint16_t value);

    // void logData(const char *key, int32_t value);
    // void logData(ManagedString key, int32_t value);

    // void logData(const char *key, float value);
    // void logData(ManagedString key, float value);

