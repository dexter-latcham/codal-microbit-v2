#ifndef MICROBIT_FASTLOG_H
#define MICROBIT_FASTLOG_H


#define MICROBIT_FASTLOG_DEFAULT_COLUMNS 3

#define MICROBIT_FASTLOG_STATUS_INITIALIZED     0x0001
#define MICROBIT_FASTLOG_STATUS_ROW_STARTED     0x0002

#define MICROBIT_FASTLOG_EVT_LOG_FULL           1
#define MICROBIT_FASTLOG_EVT_NEW_ROW           2
#define MICROBIT_FASTLOG_EVT_HEADERS_CHANGED           3

#include "stdint.h"
#include "MicroBitCircularBuffer.h"

#ifndef runningLocal
#include "ManagedString.h"
#include "stdint.h"
namespace codal
{
#endif
class LogColumnEntry
{
    public:
    ManagedString key;
    uint8_t type;
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


    public:
    FastLog();

    void beginRow();
    void endRow();

    void logData(ManagedString key, float value);
    void logData(ManagedString key, uint16_t value);
    void logData(ManagedString key, int32_t value);
    void saveLog();

    ManagedString getHeaders();

    ManagedString getHeaders(int index);

    ManagedString getRow(int row);
    uint16_t getNumberOfRows();

    uint16_t getNumberOfHeaders();
private:
        void init();
};


#ifndef runningLocal
}
#endif
#endif
