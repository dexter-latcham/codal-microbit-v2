#ifndef MICROBIT_CIRC_BUFFER_H
#define MICROBIT_CIRC_BUFFER_H


#define MICROBIT_FASTLOG_STATUS_INITIALIZED     0x0001
#define MICROBIT_FASTLOG_STATUS_ROW_STARTED     0x0002


#include "stdint.h"
#include "ManagedString.h"
namespace codal
{

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
    ManagedString getElem(int index);

private:
    uint8_t* _getNextWriteLoc(TypeMeta* meta);
    void _clearRegion(TypeMeta** metaPtr, uint8_t* from, uint8_t* to);
    void _logData(TypeMeta ** meta,uint8_t* data);

    int _countSection(TypeMeta*meta);

    uint8_t* _getElemIndex(TypeMeta* meta,int index);
};

}
#endif
