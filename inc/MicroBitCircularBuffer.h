#ifndef MICROBIT_CIRC_BUFFER_H
#define MICROBIT_CIRC_BUFFER_H


#define MICROBIT_FASTLOG_STATUS_INITIALIZED     0x0001
#define MICROBIT_FASTLOG_STATUS_ROW_STARTED     0x0002

// #define runningLocal 

#include "stdint.h"

#ifndef runningLocal
#include "ManagedString.h"
#include "stdint.h"
namespace codal
{
#endif

typedef struct {
    uint8_t* start;
    uint8_t* last;
    uint8_t* next;
    int bytes;
}TypeMeta;

class CircBuffer {

private:
    bool full;
    int maxByteSize;
    uint32_t status;          // 0 = uninitialized, 1 = initialized
    uint8_t* dataStart;         // Pointer to raw data block
    uint8_t* dataEnd;
    TypeMeta* int16Meta;
    TypeMeta* int32Meta;
    TypeMeta* floatMeta;
public:
    CircBuffer(int size);

    /**
     * Initializes the data buffer.
     */
    void init();

    /**
     * Logs a new row of data. Overwrites oldest row if buffer is full.
     */
    void logVal(float value);

    void logVal(uint16_t value);

    void logVal(int32_t value);

    void logData(TypeMeta ** meta,uint8_t* data);



    typedef void (*PrintFunc)(const void*);
    void _printSection(TypeMeta* meta,PrintFunc func);



    uint8_t* _getElemIndex(TypeMeta* meta,int index);
    void _printElemIndex(TypeMeta* meta,int index,PrintFunc func);

    uint8_t* _getNextWriteLoc(TypeMeta* meta);

    void _clearRegion(TypeMeta** metaPtr, uint8_t* from, uint8_t* to);


    int _countSection(TypeMeta*meta);
    int getRowCount();
    void print();

    void printElem(int index);

    ManagedString getElem(int index);
};

#ifndef runningLocal
}
#endif
#endif
