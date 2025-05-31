#include "MicroBitCircularBuffer.h"
#include "stdint.h"
using namespace codal;

MicroBitCircularBuffer::MicroBitCircularBuffer(int size) {
    if(size <= 8){
        //bad size, too small
        size=10;
    }
    this->maxByteSize=size;
    this->status = 0;
    this->dataStart = NULL; //void* holding data
    this->globalType=TYPE_NONE;
    for(int i=0;i<MICROBIT_FASTLOG_TYPE_COUNT;i++){
        typeMetas[i]=NULL;
    }
}

MicroBitCircularBuffer::~MicroBitCircularBuffer(){
    for(int i=0;i<MICROBIT_FASTLOG_TYPE_COUNT;i++){
        if(typeMetas[i]!=NULL){
            free(typeMetas[i]);
            typeMetas[i]=NULL;
        }
    }
    if(dataStart!=NULL){
        free(dataStart);
        dataStart=NULL;
    }
}

void MicroBitCircularBuffer::init() {
    if (status & MICROBIT_FASTLOG_STATUS_INITIALIZED){
        return;
    }
    dataStart = (uint8_t*)malloc(maxByteSize);
    if (dataStart == NULL) {
        return;
    }
    dataEnd = dataStart+maxByteSize;
    status |= MICROBIT_FASTLOG_STATUS_INITIALIZED;
}




void MicroBitCircularBuffer::push(uint8_t val) {
    if(globalType>TYPE_UINT8){
        uint16_t valNew = (uint16_t)val;
        return push(valNew);
    }
    if(globalType<TYPE_UINT8){
        typeMetas[TYPE_UINT8]=(TypeMeta*)malloc(sizeof(TypeMeta));
        typeMetas[TYPE_UINT8]->start=NULL;
        typeMetas[TYPE_UINT8]->bytes=sizeof(uint8_t);
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
        return push(newVal);
    }
}
void MicroBitCircularBuffer::push(int val) {
    if(val <0){
        if(val < -32768){
            int32_t newVal = (int32_t) val;
            return push(newVal);
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
            return push(newVal);
        }
    }
}

void MicroBitCircularBuffer::push(uint16_t val) {
    if(globalType>TYPE_UINT16){
        if(val>=32767){
            uint32_t valNew = (uint32_t)val;
            return push(valNew);
        }else{
            int16_t valNew = (int16_t)val;
            return push(valNew);
        }
    }
    if(globalType<TYPE_UINT16){
        typeMetas[TYPE_UINT16]=(TypeMeta*)malloc(sizeof(TypeMeta));
        typeMetas[TYPE_UINT16]->start=NULL;
        typeMetas[TYPE_UINT16]->bytes=sizeof(uint16_t);
        globalType=TYPE_UINT16;
    }
    _logData(TYPE_UINT16,(uint8_t*)&val);
}

void MicroBitCircularBuffer::push(int16_t val) {
    if(globalType>TYPE_INT16){
        if(val>=0){
            uint32_t valNew = (uint32_t)val;
            return push(valNew);
        }else{
            int32_t valNew = (int32_t)val;
            return push(valNew);
        }
    }
    if(globalType<TYPE_INT16){
        typeMetas[TYPE_INT16]=(TypeMeta*)malloc(sizeof(TypeMeta));
        typeMetas[TYPE_INT16]->start=NULL;
        typeMetas[TYPE_INT16]->bytes=sizeof(int16_t);
        globalType=TYPE_INT16;
    }
    _logData(TYPE_INT16,(uint8_t*)&val);
}

void MicroBitCircularBuffer::push(uint32_t val) {
    if(globalType>TYPE_UINT32){
        if(val >=2147483647){
            float valNew = (float)val;
            return push(valNew);
        }else{
            int32_t valNew = (int32_t)val;
            return push(valNew);
        }
    }
    if(globalType<TYPE_UINT32){
        typeMetas[TYPE_UINT32]=(TypeMeta*)malloc(sizeof(TypeMeta));
        typeMetas[TYPE_UINT32]->start=NULL;
        typeMetas[TYPE_UINT32]->bytes=sizeof(uint32_t);
        globalType=TYPE_UINT32;
    }
    _logData(TYPE_UINT32,(uint8_t*)&val);
}
void MicroBitCircularBuffer::push(int32_t val) {
    if(globalType>TYPE_INT32){
        float valNew = (float)val;
        return push(valNew);
    }
    if(globalType<TYPE_INT32){
        typeMetas[TYPE_INT32]=(TypeMeta*)malloc(sizeof(TypeMeta));
        typeMetas[TYPE_INT32]->start=NULL;
        typeMetas[TYPE_INT32]->bytes=sizeof(int32_t);
        globalType=TYPE_INT32;
    }
    _logData(TYPE_INT32,(uint8_t*)&val);
}


void MicroBitCircularBuffer::push(double val) {
    push((float)val);
}

void MicroBitCircularBuffer::push(float val) {
    if(globalType<TYPE_FLOAT){
        typeMetas[TYPE_FLOAT]=(TypeMeta*)malloc(sizeof(TypeMeta));
        typeMetas[TYPE_FLOAT]->start=NULL;
        typeMetas[TYPE_FLOAT]->bytes=sizeof(float);
        globalType=TYPE_FLOAT;
    }
    _logData(TYPE_FLOAT,(uint8_t*)&val);
}

circBufferElem _generateRet(ValueType type, uint8_t* valPtr){
    circBufferElem ret = {.type=TYPE_NONE};

    if(type==TYPE_UINT8){
        ret.type=TYPE_UINT32;
        ret.value.uint32Val=*(uint8_t*)valPtr;
    }else if(type==TYPE_UINT16){
        ret.type=TYPE_UINT32;
        ret.value.uint32Val=*(uint16_t*)valPtr;
    }else if(type==TYPE_UINT32){
        ret.type=TYPE_UINT32;
        ret.value.uint32Val=*(uint32_t*)valPtr;
    }else if(type==TYPE_INT16){
        ret.type=TYPE_INT32;
        ret.value.int32Val=*(int16_t*)valPtr;
    }else if(type==TYPE_INT32){
        ret.type=TYPE_INT32;
        ret.value.uint32Val=*(int32_t*)valPtr;
    }else if(type==TYPE_FLOAT){
        ret.type=TYPE_FLOAT;
        ret.value.floatVal=*(float*)valPtr;
    }
    return ret;
}
circBufferElem MicroBitCircularBuffer::pop(){
    int type=0;
    for(type=0;type<MICROBIT_FASTLOG_TYPE_COUNT;type++){
        if(typeMetas[type]!=NULL){
            circBufferElem ret = _generateRet((ValueType)type, typeMetas[type]->start);
            if(typeMetas[type]->start == typeMetas[type]->last){
                free(typeMetas[type]);
                typeMetas[type]=NULL;
            }else{
                uint8_t* potStart = typeMetas[type]->start+typeMetas[type]->bytes;
                if((potStart+typeMetas[type]->bytes)>dataEnd){
                    potStart=dataStart;
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
    circBufferElem retElem = {.type=TYPE_NONE};
    return retElem;
}


circBufferElem MicroBitCircularBuffer::get(int index){
    int count =0;
    uint8_t* current = NULL;
    TypeMeta *meta = typeMetas[0];
    int i=0;
    for(i=0;i<MICROBIT_FASTLOG_TYPE_COUNT;i++){
        meta = typeMetas[i];
        if(meta == NULL){
            continue;
        }
        current = meta->start;
        if(meta->last < meta->start){
            while(current+meta->bytes<=dataEnd){
                if(index==count){
                    goto getValueFound;
                }
                current+=meta->bytes;
                count+=1;
            }
            current=dataStart;
        }
        while(current<=meta->last){
            if(index==count){
                goto getValueFound;
            }
            count+=1;
            current+=meta->bytes;
        }
    }
    circBufferElem ret;
    ret.type= TYPE_NONE;
    return ret;
getValueFound:
    return _generateRet((ValueType)i, current);
}


int MicroBitCircularBuffer::count(){
    if (!(status & MICROBIT_FASTLOG_STATUS_INITIALIZED)){
        return 0;
    }

    TypeMeta* meta = typeMetas[0];
    int count=0;
    for(int i=0;i<MICROBIT_FASTLOG_TYPE_COUNT;i++){
        meta = typeMetas[i];
        if(typeMetas[i]!=NULL){
            uint8_t* current = meta->start;
            if(meta->last < meta->start){
                while(current+meta->bytes<=dataEnd){
                    count+=1;
                    current+=meta->bytes;
                }
                current=dataStart;
            }
            while(current<=meta->last){
                count+=1;
                current+=meta->bytes;
            }
        }
    }
    return count;
}



/**
 * Internal function used to add a number of bytes provided to the head of the buffer
 * Assumes that the value provided matches the type of the buffer region metadata provided
 * @param metaPtr pointer to the metadata for a region in the circular buffer to use
 * @param datra pointer to the data to log
 */
void MicroBitCircularBuffer::_logData(ValueType type,uint8_t* data){
    init();
    if(typeMetas[type]->start==NULL){
        typeMetas[type]->next=dataStart;
        typeMetas[type]->start=dataStart;
        typeMetas[type]->last=dataStart;
        bool abool = false;
        for(int i=type-1;i>=0;i--){
            if(typeMetas[i]!=NULL){
                abool=true;
            }
        }
        if(!abool){
            typeMetas[type]->next=dataStart+typeMetas[type]->bytes;
            memcpy(typeMetas[type]->start,data,typeMetas[type]->bytes);
            return;
        }
        for(int i=type-1;i>=0;i--){
            if(typeMetas[i]!=NULL){
                typeMetas[type]->next=typeMetas[i]->next;
                typeMetas[type]->last=typeMetas[i]->last+typeMetas[i]->bytes;
                break;
            }
        }
        if((typeMetas[type]->next+typeMetas[type]->bytes)>dataEnd){
            typeMetas[type]->next=dataStart;
        }
        typeMetas[type]->start=typeMetas[type]->next;
        for(int i=type-1;i>=0;i--){
            if(typeMetas[i]!=NULL){
                _clearRegion((ValueType)i,typeMetas[type]->last,typeMetas[type]->next+typeMetas[type]->bytes);
            }
        }
    }else{
        for(int i=type-1;i>=0;i--){
            if(typeMetas[i]!=NULL){
                _clearRegion((ValueType)i,typeMetas[type]->last+typeMetas[type]->bytes,typeMetas[type]->next+typeMetas[type]->bytes);
            }
        }
        int bytes = typeMetas[type]->bytes;
        _clearRegion(type,typeMetas[type]->next,typeMetas[type]->next+typeMetas[type]->bytes);
        if(typeMetas[type]==NULL){
            typeMetas[type]=(TypeMeta*)malloc(sizeof(TypeMeta));
            typeMetas[type]->bytes=bytes;
            typeMetas[type]->start=dataStart;
            typeMetas[type]->last=dataStart;
            typeMetas[type]->next=dataStart;
        }
    }
    memcpy(typeMetas[type]->next,data,typeMetas[type]->bytes);
    typeMetas[type]->last=typeMetas[type]->next;

    typeMetas[type]->next= typeMetas[type]->next+typeMetas[type]->bytes;
    if((typeMetas[type]->next+typeMetas[type]->bytes)>dataEnd){
        typeMetas[type]->next=dataStart;
    }
}

/**
 * Internal function used to remove elements from the back of the buffer
 * Continuously removes the oldest elements until the provided address region is unoccupied
 * If all elements of a given type are removed the metadata for the region is freed
 *
 * @param metaPtr pointer to the metadata for a region in the circular buffer to be cleared
 * @param from start address of the region to be cleared
 * @param to end address of the region to be cleared
 */
void MicroBitCircularBuffer::_clearRegion(ValueType type, uint8_t* from, uint8_t* to){
    if(typeMetas[type]==NULL){
        return;
    }
    TypeMeta* meta = typeMetas[type];
    if(to<from){
        //clear from from to end, then 0 to from
        _clearRegion(type,from,dataEnd);
        return _clearRegion(type,dataStart,to);
    }

    if(meta->start==meta->last){
        if(meta->start >=to){
            return;
        }
        if(meta->start <from){
            return;
        }
        free(typeMetas[type]);
        typeMetas[type]=NULL;
        return;
    }
    //from itn16Start to int16End
    //if from to is outside of this range then just return
    if(meta->start < meta->last){
        if(meta->last<from){
            return;
        }
        while(meta->start<to){
            uint8_t* possibleNext = meta->start+meta->bytes;
            meta->start=possibleNext;
            if(meta->start > meta->last){
                free(typeMetas[type]);
                typeMetas[type]=NULL;
                return;
            }
        }
        return;
    }else if(meta->start>meta->last){
        //last is greater than start
        if((meta->last < from)&&(meta->start >=to)){
            return;
        }

        bool flag=false;
        if(meta->start >to){
            //reset start to 0
            meta->start=dataStart;
            flag=true;
        }
        while(meta->start <to){
            uint8_t* possibleNext = meta->start+meta->bytes;
            meta->start=possibleNext;
            if(flag==true){
                if(meta->start > meta->last){
                    free(typeMetas[type]);
                    typeMetas[type]=NULL;
                    return;
                }
            }
        }
        if(to==dataEnd){
            meta->start=dataStart;
        }
    }
}
