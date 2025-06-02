#ifndef MICROBIT_CIRC_BUFFER_H
#define MICROBIT_CIRC_BUFFER_H


#define MICROBIT_CIRCULAR_BUFFER_STATUS_INITIALIZED     0x0001
#define MICROBIT_CIRCULAR_BUFFER_STATUS_ROW_STARTED     0x0002

#define MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT 6
#define MICROBIT_CIRCULAR_BUFFER_MAX_DATA_BUFFERS 40
#define MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION 500

#include "stdint.h"
#include "ManagedString.h"
namespace codal
{

enum ValueType {
    TYPE_NONE=-1,
    TYPE_UINT8 =0,
    TYPE_UINT16 =1,
    TYPE_INT16 =2,
    TYPE_UINT32 =3,
    TYPE_INT32 =4,
    TYPE_FLOAT=5
};


typedef struct circBufferElem{
    ValueType type;
    union value{
        float floatVal;
        int32_t int32Val;
        uint32_t uint32Val;
    }value;
}circBufferElem;

typedef struct {
    int start;
    int last;
    int next;
    int bytes;
}TypeMeta;

class MicroBitCircularBuffer {

private:
    bool full;
    uint32_t status;
    uint8_t*dataBuffers[MICROBIT_CIRCULAR_BUFFER_MAX_DATA_BUFFERS];
    int allocatedBuffers;
    int currentByteMax;
    int maxBufferSize;
    TypeMeta* typeMetas[MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT];
    ValueType globalType;

public:

    MicroBitCircularBuffer();
    MicroBitCircularBuffer(int size);
    ~MicroBitCircularBuffer();

    /**
     * Initializes the data buffer.
     */
    void init();


    void push(double value);



    void push(int value);
    void push(unsigned int val);

    void push(uint8_t value);
    void push(uint16_t value);
    void push(int16_t value);
    void pushu32(uint32_t value);
    void pushi32(int32_t value);

    void push(float value);


    int count();

    /**
    * get at an index
    */
    circBufferElem get(int index);

    /**
    * get and remove oldest element
    */
    circBufferElem pop();

private:


    circBufferElem _getElem(ValueType type, int index);
    void _clearRegion(ValueType type, int from, int to);
    void _logData(ValueType type,uint8_t* data);

    bool _insertElement(int index, uint8_t* data,int bytes);

    // uint8_t* _getElemIndex(TypeMeta* meta,int index);
};
}
#endif
