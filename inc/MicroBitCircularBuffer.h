#ifndef MICROBIT_CIRC_BUFFER_H
#define MICROBIT_CIRC_BUFFER_H


#define MICROBIT_FASTLOG_STATUS_INITIALIZED     0x0001
#define MICROBIT_FASTLOG_STATUS_ROW_STARTED     0x0002


#include "stdint.h"
#include "ManagedString.h"
namespace codal
{

enum ValueType {
    TYPE_NONE=0,
    TYPE_UINT16 = 1,
    TYPE_INT32 =2,
    TYPE_FLOAT=3
};

typedef struct returnedBufferElem{
    ValueType type;
    union value{
        float floatVal;
        int32_t int32Val;
        uint16_t int16Val;
    }value;
}returnedBufferElem;

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


    void logVal(int value);

    void logVal(uint16_t value);

    void logVal(int32_t value);

    void logVal(float value);

    int getElementCount();
    returnedBufferElem getElem(int index);
    //
    // ManagedString getElem(int index);

private:
    uint8_t* _getNextWriteLoc(TypeMeta* meta);
    void _clearRegion(TypeMeta** metaPtr, uint8_t* from, uint8_t* to);
    void _logData(TypeMeta ** meta,uint8_t* data);

    int _countSection(TypeMeta*meta);

    // uint8_t* _getElemIndex(TypeMeta* meta,int index);
};

}
#endif
