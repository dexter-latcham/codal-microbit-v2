#ifndef MICROBIT_FASTLOG_H
#define MICROBIT_FASTLOG_H


#define MICROBIT_FASTLOG_DEFAULT_COLUMNS 20

#define MICROBIT_FASTLOG_STATUS_INITIALIZED     0x0001
#define MICROBIT_FASTLOG_STATUS_ROW_STARTED     0x0002
#define MICROBIT_FASTLOG_STATUS_FIRST_ROW_LOGGED     0x0004
#define MICROBIT_FASTLOG_STATUS_USER_SET_COLS     0x0008
#define MICROBIT_FASTLOG_CIRCULAR_BUFFER_CLEARED     0x0010

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
    CODAL_TIMESTAMP logStartTime;
    CODAL_TIMESTAMP previousLogTime;

    public:
    FastLog(int columns=-1);

    ~FastLog();

    void beginRow();
    void endRow();



    void logData(const char *key, int value);
    void logData(ManagedString key, int value);

    void logData(const char *key, uint16_t value);
    void logData(const char *key, int32_t value);
    void logData(const char *key, float value);
    void logData(const char *key, double value);
    void logData(ManagedString key, float value);
    void logData(ManagedString key, double value);
    void logData(ManagedString key, uint16_t value);
    void logData(ManagedString key, int32_t value);
    void saveLog(bool deleteLog=true);

    void setTimeStamp(TimeStampFormat format);
    ManagedString getHeaders();

    ManagedString getHeaders(int index);

    ManagedString getRow(int row);
    uint16_t getNumberOfRows();

    uint16_t getNumberOfHeaders();
private:
    void init();
    void _storeValue(ManagedString key, ValueType type, void* addr);

    ManagedString _timeOffsetToString(int timeOffset);
    ManagedString _bufferRetToString(returnedBufferElem ret);
};


}
#endif
