#include "MicroBitCircularBuffer.h"
#include "stdint.h"
using namespace codal;
MicroBitCircularBuffer::MicroBitCircularBuffer() {
    this->maxBufferSize=-1;
    this->status = 0;
    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_MAX_DATA_BUFFERS;i++){
        this->dataBuffers[i]=NULL;
    }
    this->globalType=TYPE_NONE;
    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT;i++){
        typeMetas[i]=NULL;
    }
}

MicroBitCircularBuffer::MicroBitCircularBuffer(int size) {
    this->maxBufferSize=size;
    this->status = 0;
    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_MAX_DATA_BUFFERS;i++){
        this->dataBuffers[i]=NULL;
    }
    this->globalType=TYPE_NONE;
    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT;i++){
        typeMetas[i]=NULL;
    }
}

MicroBitCircularBuffer::~MicroBitCircularBuffer(){
    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT;i++){
        if(typeMetas[i]!=NULL){
            free(typeMetas[i]);
            typeMetas[i]=NULL;
        }
    }

    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_MAX_DATA_BUFFERS;i++){
        if(dataBuffers[i]!=NULL){
            free(dataBuffers[i]);
            dataBuffers[i]=NULL;
        }
    }
}


void MicroBitCircularBuffer::init() {
    if (status & MICROBIT_CIRCULAR_BUFFER_STATUS_INITIALIZED){
        return;
    }
    int toAlloc=MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
    if(maxBufferSize!=-1){
        if(maxBufferSize<toAlloc){
            toAlloc=maxBufferSize;
        }
    }
    dataBuffers[0] = (uint8_t*)malloc(toAlloc);
    if (dataBuffers[0] == NULL) {
        return;
    }
    allocatedBuffers=1;
    currentByteMax=toAlloc;
    status |= MICROBIT_CIRCULAR_BUFFER_STATUS_INITIALIZED;
}

TypeMeta* _createTypeRegion(int typeSize){
    TypeMeta* ret =(TypeMeta*)malloc(sizeof(TypeMeta));
    ret->start=-1;
    ret->bytes=typeSize;
    return ret;
}

void MicroBitCircularBuffer::push(uint8_t val) {
    if(globalType>TYPE_UINT8){
        uint16_t valNew = (uint16_t)val;
        return push(valNew);
    }
    if(globalType<TYPE_UINT8){
        typeMetas[TYPE_UINT8]=_createTypeRegion(sizeof(uint8_t));
        globalType=TYPE_UINT8;
    }
    _logData(TYPE_UINT8,(uint8_t*)&val);
}


void MicroBitCircularBuffer::push(unsigned int val) {
    if(val <= 255){
        uint8_t newVal = (uint8_t) val;
        return push(newVal);
    }else if(val <= 65535){
        uint16_t newVal = (uint16_t) val;
        return push(newVal);
    }else{
        uint32_t newVal = (uint32_t) val;
        return pushu32(newVal);
    }
}
void MicroBitCircularBuffer::push(int val) {
    if(val <0){
        if(val < -32768){
            int32_t newVal = (int32_t) val;
            return pushi32(newVal);
        }else{
            int16_t newVal = (int16_t) val;
            return push(newVal);
        }
    }else{
        if(val <= 255){
            uint8_t newVal = (uint8_t) val;
            return push(newVal);
        }else if(val <= 65535){
            uint16_t newVal = (uint16_t) val;
            return push(newVal);
        }else{
            uint32_t newVal = (uint32_t) val;
            return pushu32(newVal);
        }
    }
}

void MicroBitCircularBuffer::push(uint16_t val) {
    if(globalType>TYPE_UINT16){
        if(val>=32767){
            uint32_t valNew = (uint32_t)val;
            return pushu32(valNew);
        }else{
            int16_t valNew = (int16_t)val;
            return push(valNew);
        }
    }
    if(globalType<TYPE_UINT16){
        typeMetas[TYPE_UINT16]=_createTypeRegion(sizeof(uint16_t));
        globalType=TYPE_UINT16;
    }
    _logData(TYPE_UINT16,(uint8_t*)&val);
}

void MicroBitCircularBuffer::push(int16_t val) {
    if(globalType>TYPE_INT16){
        if(val>=0){
            uint32_t valNew = (uint32_t)val;
            return pushu32(valNew);
        }else{
            int32_t valNew = (int32_t)val;
            return pushi32(valNew);
        }
    }
    if(globalType<TYPE_INT16){
        typeMetas[TYPE_INT16]=_createTypeRegion(sizeof(int16_t));
        globalType=TYPE_INT16;
    }
    _logData(TYPE_INT16,(uint8_t*)&val);
}

void MicroBitCircularBuffer::pushu32(uint32_t val) {
    if(globalType>TYPE_UINT32){
        if(val >=2147483647){
            float valNew = (float)val;
            return push(valNew);
        }else{
            int32_t valNew = (int32_t)val;
            return pushi32(valNew);
        }
    }
    if(globalType<TYPE_UINT32){
        typeMetas[TYPE_UINT32]=_createTypeRegion(sizeof(uint32_t));
        globalType=TYPE_UINT32;
    }
    _logData(TYPE_UINT32,(uint8_t*)&val);
}
void MicroBitCircularBuffer::pushi32(int32_t val) {
    if(globalType>TYPE_INT32){
        float valNew = (float)val;
        return push(valNew);
    }
    if(globalType<TYPE_INT32){
        typeMetas[TYPE_INT32]=_createTypeRegion(sizeof(int32_t));
        globalType=TYPE_INT32;
    }
    _logData(TYPE_INT32,(uint8_t*)&val);
}


void MicroBitCircularBuffer::push(double val) {
    push((float)val);
}

void MicroBitCircularBuffer::push(float val) {
    if(globalType<TYPE_FLOAT){
        typeMetas[TYPE_FLOAT]=_createTypeRegion(sizeof(float));
        globalType=TYPE_FLOAT;
    }
    _logData(TYPE_FLOAT,(uint8_t*)&val);
}

circBufferElem MicroBitCircularBuffer::pop(){
    int type=0;
    for(type=0;type<MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT;type++){
        if(typeMetas[type]!=NULL){
            circBufferElem ret = _getElem((ValueType)type, typeMetas[type]->start);
            if(typeMetas[type]->start == typeMetas[type]->last){
                free(typeMetas[type]);
                typeMetas[type]=NULL;

            }else{
                int potStart = typeMetas[type]->start+typeMetas[type]->bytes;
                if((potStart+typeMetas[type]->bytes)>currentByteMax){
                    potStart=0;
                }
                if(potStart < typeMetas[type]->last){
                    if((potStart+typeMetas[type]->bytes)> typeMetas[type]->last){
                        free(typeMetas[type]);
                        typeMetas[type]=NULL;
                        return ret;
                    }
                }
                typeMetas[type]->start=potStart;
            }
            return ret;
        }
    }
    circBufferElem retElem ;
    retElem.type=TYPE_NONE;
    return retElem;
}


circBufferElem MicroBitCircularBuffer::get(int index){
    int count =0;
    int current = 0;
    TypeMeta *meta = typeMetas[0];
    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT;i++){
        meta = typeMetas[i];
        if(meta == NULL){
            continue;
        }
        current = meta->start;
        if(meta->last < meta->start){
            while(current+meta->bytes<=currentByteMax){
                if(index==count){
                    return _getElem((ValueType)i, current);
                }
                current+=meta->bytes;
                count+=1;
            }
            current=0;
        }
        while(current<=meta->last){
            if(index==count){
                return _getElem((ValueType)i, current);
            }
            count+=1;
            current+=meta->bytes;
        }
    }
    circBufferElem ret;
    ret.type= TYPE_NONE;
    return ret;
}


int MicroBitCircularBuffer::count(){
    if (!(status & MICROBIT_CIRCULAR_BUFFER_STATUS_INITIALIZED)){
        return 0;
    }

    TypeMeta* meta = typeMetas[0];
    int count=0;
    for(int i=0;i<MICROBIT_CIRCULAR_BUFFER_TYPE_COUNT;i++){
        meta = typeMetas[i];
        if(typeMetas[i]!=NULL){
            int current = meta->start;
            if(meta->last < meta->start){
                while(current+meta->bytes<=currentByteMax){
                    count+=1;
                    current+=meta->bytes;
                }
                current=0;
            }
            while(current<=meta->last){
                count+=1;
                current+=meta->bytes;
            }
        }
    }
    return count;
}




circBufferElem MicroBitCircularBuffer::_getElem(ValueType type, int index){
    uint8_t temp[4];
    int bytesToCopy=typeMetas[type]->bytes;
    for(int i=0;i<bytesToCopy;i++){
        int curI=index+i;
        int bufferToUse = curI / MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
        int indexInBuffer = curI % MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
        temp[i]=dataBuffers[bufferToUse][indexInBuffer];
    }
    circBufferElem ret;
    ret.type= TYPE_NONE;
    if(type==TYPE_UINT8){
        ret.type=TYPE_UINT32;
        ret.value.uint32Val=*(uint8_t*)temp;
    }else if(type==TYPE_UINT16){
        ret.type=TYPE_UINT32;
        ret.value.uint32Val=*(uint16_t*)temp;
    }else if(type==TYPE_UINT32){
        ret.type=TYPE_UINT32;
        ret.value.uint32Val=*(uint32_t*)temp;
    }else if(type==TYPE_INT16){
        ret.type=TYPE_INT32;
        ret.value.int32Val=*(int16_t*)temp;
    }else if(type==TYPE_INT32){
        ret.type=TYPE_INT32;
        ret.value.uint32Val=*(int32_t*)temp;
    }else if(type==TYPE_FLOAT){
        ret.type=TYPE_FLOAT;
        ret.value.floatVal=*(float*)temp;
    }
    return ret;
}

bool MicroBitCircularBuffer::_insertElement(int index, uint8_t* data,int bytes){
    if(maxBufferSize!=currentByteMax){
        if((index+8) > currentByteMax){
            if(allocatedBuffers!=MICROBIT_CIRCULAR_BUFFER_MAX_DATA_BUFFERS){
                int toAlloc=MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
                if(maxBufferSize!=-1){
                    if((maxBufferSize-currentByteMax)<toAlloc){
                        toAlloc=maxBufferSize-currentByteMax;
                    }
                }
                if(toAlloc>0){
                    dataBuffers[allocatedBuffers]=(uint8_t*)malloc(toAlloc);
                    if(dataBuffers[allocatedBuffers]!=NULL){
                        currentByteMax+=toAlloc;
                        allocatedBuffers+=1;
                        if(allocatedBuffers==MICROBIT_CIRCULAR_BUFFER_MAX_DATA_BUFFERS){
                            maxBufferSize=currentByteMax;
                        }
                    }else{
                        maxBufferSize=currentByteMax;
                    }
                }
            }
        }
    }
    int bufferToUse = index / MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
    int indexInBuffer = index % MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
    int spaceInBuffer = MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION-indexInBuffer;
    if(spaceInBuffer<bytes){
        memcpy(dataBuffers[bufferToUse]+indexInBuffer,data,spaceInBuffer);
        index+=spaceInBuffer;

        bufferToUse = index / MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
        indexInBuffer = index % MICROBIT_CIRCULAR_BUFFER_MAX_SINGLE_ALLOCATION;
        memcpy(dataBuffers[bufferToUse]+indexInBuffer,data+spaceInBuffer,bytes-spaceInBuffer);
    }else{
        memcpy(dataBuffers[bufferToUse]+indexInBuffer,data,bytes);
    }
    return true;
}

/**
 * Internal function used to add a number of bytes provided to the head of the buffer
 * Assumes that the value provided matches the type of the buffer region metadata provided
 * @param metaPtr pointer to the metadata for a region in the circular buffer to use
 * @param datra pointer to the data to log
 */
void MicroBitCircularBuffer::_logData(ValueType type,uint8_t* data){
    init();
    if(typeMetas[type]->start==-1){
        typeMetas[type]->next=0;
        typeMetas[type]->start=0;
        typeMetas[type]->last=0;
        for(int i=type-1;i>=0;i--){
            if(typeMetas[i]!=NULL){
                typeMetas[type]->next=typeMetas[i]->next;
                typeMetas[type]->last=typeMetas[i]->last+typeMetas[i]->bytes-typeMetas[type]->bytes;
                break;
            }
        }
        if((typeMetas[type]->next+typeMetas[type]->bytes)>currentByteMax){
            typeMetas[type]->next=0;
        }
        typeMetas[type]->start=typeMetas[type]->next;
    }else{
        if(typeMetas[type]->next<=typeMetas[type]->start){
            if((typeMetas[type]->next+typeMetas[type]->bytes)>typeMetas[type]->start){
                if(typeMetas[type]->start==typeMetas[type]->last){
                    typeMetas[type]->start=typeMetas[type]->next;
                    typeMetas[type]->last=typeMetas[type]->next;
                }else{
                    typeMetas[type]->start=typeMetas[type]->start+typeMetas[type]->bytes;
                    if((typeMetas[type]->start+typeMetas[type]->bytes)>currentByteMax){
                        typeMetas[type]->start=0;
                    }
                }
            }
        }
    }
    for(int i=type-1;i>=0;i--){
        if(typeMetas[i]!=NULL){
            _clearRegion((ValueType)i,typeMetas[type]->last+typeMetas[type]->bytes,typeMetas[type]->next+typeMetas[type]->bytes);
        }
    }

    _insertElement(typeMetas[type]->next,data,typeMetas[type]->bytes);
    typeMetas[type]->last=typeMetas[type]->next;
    typeMetas[type]->next= typeMetas[type]->next+typeMetas[type]->bytes;

    if((typeMetas[type]->next+typeMetas[type]->bytes)>currentByteMax){
        typeMetas[type]->next=0;
    }
}

void MicroBitCircularBuffer::_clearRegion(ValueType type, int from, int to){
    if(typeMetas[type]==NULL){
        return;
    }

    TypeMeta* meta = typeMetas[type];
    if(to==from){
        free(typeMetas[type]);
        typeMetas[type]=NULL;
        return;
    }

    if(to<from){
        if(from<currentByteMax){
            _clearRegion(type,from,currentByteMax);
        }
        if(to!=0){
            _clearRegion(type,0,to);
        }
        return;
    }

    if(meta->start<=meta->last){
        if((meta->last+meta->bytes)<=from){
            return;
        }
        while(meta->start<to){
            meta->start=meta->start+meta->bytes;
            if(meta->start>meta->last){
                free(typeMetas[type]);
                typeMetas[type]=NULL;
                return;
            }
        }
        return;
    }else{
        if(((meta->last+meta->bytes) <= from)&&(meta->start >=to)){
            return;
        }
        while(meta->start<to){
            meta->start=meta->start+meta->bytes;
            if((meta->start+meta->bytes)>currentByteMax){
                meta->start=0;
                return _clearRegion(type,from,to);
            }
        }
        if((meta->last+meta->bytes)<=from){
            return;
        }
        meta->start=0;
        return _clearRegion(type,from,to);
    }

}
