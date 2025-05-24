#include "MicroBitCircularBuffer.h"
#include "stdint.h"


using namespace codal;

CircBuffer::CircBuffer(int size) {
    this->maxByteSize=size;
    this->full=false;
    this->status = 0;
    this->dataStart = NULL; //void* holding data
    this->int16Meta=NULL;
    this->int32Meta=NULL;
    this->floatMeta=NULL;
}

CircBuffer::~CircBuffer(){
    if(int16Meta!=NULL){
        free(int16Meta);
        int16Meta=NULL;
    }
    if(int32Meta!=NULL){
        free(int32Meta);
        int32Meta=NULL;
    }
    if(floatMeta!=NULL){
        free(floatMeta);
        floatMeta=NULL;
    }
    if(dataStart!=NULL){
        free(dataStart);
        dataStart=NULL;
    }
}

void CircBuffer::init() {
    if (status & MICROBIT_FASTLOG_STATUS_INITIALIZED){
        return;
    }
    full=false;
    dataStart = (uint8_t*)malloc(maxByteSize);
    if (dataStart == NULL) {
        return;
    }
    dataEnd = dataStart+maxByteSize;
    status |= MICROBIT_FASTLOG_STATUS_INITIALIZED;
}



void CircBuffer::logVal(int value) {
    if(value >=0 && value < 65535){
        return logVal((uint16_t) value);
    }else if(value >= INT32_MIN && value <= INT32_MAX){
        return logVal((int32_t) value);
    }else{
        return logVal((float) value);
    }
}

void CircBuffer::logVal(uint16_t val) {
    if(floatMeta!=NULL){
        float valfloat = (float)val;
        return _logData(&floatMeta,(uint8_t*)&valfloat);
    }

    if(int32Meta!=NULL){
        int32_t valInt = (int32_t)val;
        return _logData(&int32Meta,(uint8_t*)&valInt);
    }

    if(int16Meta==NULL){
        int16Meta=(TypeMeta*)malloc(sizeof(TypeMeta));
        int16Meta->start=NULL;
        int16Meta->bytes=sizeof(uint16_t);
    }
    _logData(&int16Meta,(uint8_t*)&val);
}


void CircBuffer::logVal(int32_t val) {
    if(floatMeta!=NULL){
        float valfloat = (float)val;
        return _logData(&floatMeta,(uint8_t*)&valfloat);
    }

    if(int32Meta==NULL){
        int32Meta=(TypeMeta*)malloc(sizeof(TypeMeta));
        int32Meta->start=NULL;
        int32Meta->bytes=sizeof(int32_t);
    }
    _logData(&int32Meta,(uint8_t*)&val);
}


void CircBuffer::logVal(double val) {
    logVal((float)val);
}

void CircBuffer::logVal(float val) {
    if(floatMeta==NULL){
        floatMeta=(TypeMeta*)malloc(sizeof(TypeMeta));
        floatMeta->start=NULL;
        floatMeta->bytes=sizeof(float);
    }
    _logData(&floatMeta,(uint8_t*)&val);
}




/**
 * Internal function used to find the next location data can be written to
 * A valid location is the next address that can fit the bytes required by meta without exceeding the memory available
 * @param meta metatata for the circular buffer region of a provided type
 *
 * @return the next valid address
 */
uint8_t* CircBuffer::_getNextWriteLoc(TypeMeta* meta){
    uint8_t* possibleNext = meta->next+meta->bytes;
    if((possibleNext+meta->bytes)>dataEnd){
        possibleNext=dataStart;
        full=true;
    }
    return possibleNext;
}

/**
 * Internal function used to add a number of bytes provided to the head of the buffer
 * Assumes that the value provided matches the type of the buffer region metadata provided
 * @param metaPtr pointer to the metadata for a region in the circular buffer to use
 * @param datra pointer to the data to log
 */
void CircBuffer::_logData(TypeMeta ** metaPtr,uint8_t* data){
    init();
    TypeMeta* meta = *metaPtr;
    if(meta->start==NULL){
        uint8_t* possibleNext;
        uint8_t* possibleLast;
        if((int32Meta!=NULL) && (int32Meta->start!=NULL)){
            possibleNext=int32Meta->next;
            possibleLast=int32Meta->last;
        }else if((int16Meta!=NULL) && (int16Meta->start!=NULL)){
            possibleNext=int16Meta->next;
            possibleLast=int16Meta->last;
        }else{
            meta->start=dataStart;
            meta->last=dataStart;
            meta->next=dataStart+meta->bytes;
            memcpy(meta->start,data,meta->bytes);
            return;
        }
        if((possibleNext+meta->bytes)>dataEnd){
            possibleNext=dataStart;
            full=true;
        }
        meta->start=possibleNext;
        meta->last=possibleNext;
        meta->next=possibleNext;
        if(full){
            if(floatMeta!=NULL){
                if(int16Meta!=NULL){
                    _clearRegion(&int16Meta,possibleLast+sizeof(uint16_t),possibleNext+sizeof(float));
                    _clearRegion(&int32Meta,possibleLast+sizeof(uint16_t),possibleNext+sizeof(float));
                }else{
                    _clearRegion(&int32Meta,possibleLast+sizeof(int32_t),possibleNext+sizeof(float));
                }
            }else if(int32Meta!=NULL){
                _clearRegion(&int16Meta,possibleLast+sizeof(uint16_t),possibleNext+sizeof(int32_t));
            }
        }
        memcpy(meta->start,data,meta->bytes);
        meta->next=_getNextWriteLoc(meta);
        return;
    }
    
    if(full){
        if(floatMeta!=NULL){
            _clearRegion(&int16Meta,meta->last+meta->bytes,meta->next+meta->bytes);
            _clearRegion(&int32Meta,meta->last+meta->bytes,meta->next+meta->bytes);
        }else if(int32Meta!=NULL){
            _clearRegion(&int16Meta,meta->last+meta->bytes,meta->next+meta->bytes);
        }
        _clearRegion(metaPtr,meta->next,meta->next+meta->bytes);
    }

    memcpy(meta->next,data,meta->bytes);
    meta->last=meta->next;
    meta->next=_getNextWriteLoc(meta);
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
void CircBuffer::_clearRegion(TypeMeta** metaPtr, uint8_t* from, uint8_t* to){
    if(metaPtr==NULL || *metaPtr==NULL){
        return;
    }
    TypeMeta* meta = *metaPtr;

   if(to<from){
        //clear from from to end, then 0 to from
        _clearRegion(metaPtr,from,dataEnd);
        return _clearRegion(metaPtr,dataStart,to);
    }
    
    //from itn16Start to int16End
    //if from to is outside of this range then just return
    if(meta->start < meta->last){
        if(meta->last<from){
            return;
        }
        if(meta->start>=to){
            return;
        }

        while(meta->start<to){
            uint8_t* possibleNext = meta->start+meta->bytes;
            meta->start=possibleNext;
            if(meta->start > meta->last){
                free(*metaPtr);
                *metaPtr=NULL;
                return;
            }
        }
        return;
    }else if(meta->start>meta->last){
        //last is greater than start
        bool flag=false;
        if((meta->last < from)&&(meta->start >=to)){
            return;
        }

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
                    free(*metaPtr);
                    *metaPtr=NULL;
                    return;
                }
            }
        }
        if(to==dataEnd){
            meta->start=dataStart;
        }
    }else{
        //start=last
        //only 1 entry in me
        if(meta->start >=to){
            return;
        }
        if(meta->start <from){
            return;
        }
        free(*metaPtr);
        *metaPtr=NULL;
        return;
    }
}


returnedBufferElem CircBuffer::getElem(int index){
    TypeMeta *meta = int16Meta;
    int count =0;
    TypeMeta *metaList[]={int16Meta,int32Meta,floatMeta};
    uint8_t* current = NULL;

    returnedBufferElem ret;
    for(int i=0;i<3;i++){
        meta = metaList[i];
        if(meta == NULL){
            continue;
        }
        current = meta->start;
        if(meta->last < meta->start){
            while(current+meta->bytes<=dataEnd){
                if(index==count){
                    goto valueFound;
                }
                current+=meta->bytes;
                count+=1;
            }
            current=dataStart;
        }
        while(current<=meta->last){
            if(index==count){
                goto valueFound;
            }
            count+=1;
            current+=meta->bytes;
        }
    }
    ret.type=TYPE_NONE;
    return ret;
valueFound:
    if(meta==int16Meta){
        ret.type=TYPE_UINT16;
        ret.value.int16Val=*(uint16_t*)current;
    }else if(meta==int32Meta){
        ret.type=TYPE_INT32;
        ret.value.int32Val=*(int32_t*)current;
    }else{
        ret.type=TYPE_FLOAT;
        ret.value.floatVal=*(float*)current;
    }
    return ret;
}


int CircBuffer::getElementCount(){
    init();
    TypeMeta* meta = int16Meta;
    TypeMeta *metaList[]={int16Meta,int32Meta,floatMeta};
    int count=0;
    for(int i=0;i<3;i++){
        meta = metaList[i];
        if(meta==NULL){
            continue;
        }
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
    return count;
}

// //
// /**
// * returns the number of elements of a provided type stored in the buffer
// */
// int CircBuffer::_countSection(TypeMeta*meta){
//     if(meta==NULL){
//         return 0;
//     }
//     int count=0;
//     uint8_t* from = meta->start;
//     uint8_t* to = meta->last;

//     uint8_t* current = from;
//     if(to < from){
//         current=from;
//         while(current+meta->bytes<=dataEnd){
//             count+=1;
//             current+=meta->bytes;
//         }
//         current=dataStart;
//     }
//     while(current<=to){
//         count+=1;
//         current+=meta->bytes;
//     }
//     return count;
// }

// returnedBufferElem CircBuffer::getElem(int index){
//     TypeMeta *meta = int16Meta;
//     int count =0;
//     returnedBufferElem ret;
//     for(int i=0;i<3;i++){
//         if(i==0){
//             meta=int16Meta;
//             ret.type=TYPE_UINT16;
//         }else if(i==1){
//             meta = int32Meta;
//             ret.type=TYPE_INT32;
//         }else{
//             meta = floatMeta;
//             ret.type=TYPE_FLOAT;
//         }
//         if(meta == NULL){
//             continue;
//         }
//         uint8_t* from = meta->start;
//         uint8_t* to = meta->last;

//         uint8_t* current = from;
//         if(to < from){
//             current=from;
//             while(current+meta->bytes<=dataEnd){
//                 if(index==count){
//                     if(i==0){
//                         ret.value.int16Val=*(uint16_t*)current;
//                     }else if(i==1){
//                         ret.value.int32Val=*(int32_t*)current;
//                     }else{
//                         ret.value.floatVal=*(float*)current;
//                     }
//                     return ret;
//                 }
//                 current+=meta->bytes;
//                 count+=1;
//             }
//             current=dataStart;
//         }
//         while(current<=to){
//             if(index==count){
//                 if(i==0){
//                     ret.value.int16Val=*(uint16_t*)current;
//                 }else if(i==1){
//                     ret.value.int32Val=*(int32_t*)current;
//                 }else{
//                     ret.value.floatVal=*(float*)current;
//                 }
//                 return ret;
//             }
//             count+=1;
//             current+=meta->bytes;
//         }
//     }
//     ret.type=TYPE_NONE;
//     return ret;
// }
// /**
// * returns the string representation of the nth element in the buffer
// */
// ManagedString CircBuffer::getElem(int index){
//     int int16Entries = _countSection(int16Meta);
//     int int32Entries = _countSection(int32Meta);
//     int floatEntries = _countSection(floatMeta);
//     if(index<0){
//         if(floatEntries>=(-1*index)){
//             uint8_t* addr = _getElemIndex(floatMeta,index);
//             if(addr==NULL){
//                 return ManagedString::EmptyString;
//             }
//             if(addr!=NULL){
//                 float val = *(float*)addr;
//                 ManagedString floatStr=floatToStr(val);
//                 return floatStr;
//             }
//         }
//         if((int32Entries+floatEntries)>=(-1*index)){
//             uint8_t* addr = _getElemIndex(int32Meta,index+floatEntries);
//             if(addr==NULL){
//                 return ManagedString::EmptyString;
//             }
//             if(addr!=NULL){
//                 int32_t val = *(int32_t*)addr;
//                 int intVal = (int)val;
//                 return ManagedString(intVal);
//             }
//         }
//         if((int32Entries+floatEntries+int16Entries)>=(-1*index)){
//             uint8_t* addr = _getElemIndex(int16Meta,index+floatEntries+int32Entries);
//             if(addr==NULL){
//                 return ManagedString::EmptyString;
//             }
//             if(addr!=NULL){
//                 uint16_t num = *(uint16_t*)addr;
//                 return ManagedString(num);
//             }
//         }
//         return ManagedString::EmptyString;
//     }

//     if(index < int16Entries){
//         uint8_t* addr = _getElemIndex(int16Meta,index);
//         if(addr==NULL){
//             return ManagedString::EmptyString;
//         }
//         if(addr!=NULL){
//             uint16_t num = *(uint16_t*)addr;
//             return ManagedString(num);
//         }
//     }
//     if(index < (int32Entries+int16Entries)){
//         uint8_t* addr = _getElemIndex(int32Meta,index-int16Entries);
//         if(addr==NULL){
//             return ManagedString::EmptyString;
//         }
//         if(addr!=NULL){
//             int32_t val = *(int32_t*)addr;
//             int intVal = (int)val;
//             return ManagedString(intVal);
//         }
//     }
//     if(index < (int32Entries+int16Entries+floatEntries)){
//         uint8_t* addr = _getElemIndex(floatMeta,index-int16Entries-int32Entries);
//         if(addr==NULL){
//             return ManagedString::EmptyString;
//         }
//         if(addr!=NULL){
//             float val = *(float*)addr;
//             ManagedString floatStr=floatToStr(val);
//             return floatStr;
//         }
//     }
//     return ManagedString::EmptyString;
// }

/**
* returns the total number of elements in the buffer
*/
/**
* returns the start address of the n'th element in a region of the buffer
* special case for -1, the newest element in the region
*/
// uint8_t* CircBuffer::_getElemIndex(TypeMeta* meta, int index){
//     if(meta==NULL){
//         return NULL;
//     }
//     uint8_t* from = meta->start;
//     uint8_t* to = meta->last;

//     uint8_t* current = from;
//     int count=0;
//     if(index<0){
//         current= to;
//         count=-1;
//         //from to to from
//         //if to is less than from, from to to start then end to to
//         if(to<from){
//             while(current>=dataStart){
//                 if(count==index){
//                     return current;
//                 }
//                 current-=meta->bytes;
//                 count-=1;
//             }
//             int entries=(dataEnd-from)/meta->bytes;
//             current = (from+entries*meta->bytes)-meta->bytes;
//         }
//         while(current>=from){
//             if(count==index){
//                 return current;
//             }
//             current-=meta->bytes;
//             count-=1;
//         }
//         return NULL;
//     }

//     if(to < from){
//         current=from;
//         while(current+meta->bytes<=dataEnd){
//             if(index==count){
//                 return current;
//             }
//             current+=meta->bytes;
//             count+=1;
//         }
//         current=dataStart;
//     }
//     while(current<=to){
//         if(index==count){
//             return current;
//         }
//         count+=1;
//         current+=meta->bytes;
//     }
//     return NULL;
// }


// /**
// * helper function to convert a float to a string
// */
// static ManagedString floatToStr(float num){
//     int integerPart = (int)num;
//     int fractionPart = (int)((num-integerPart)*100000.0f);

//     ManagedString ret(integerPart);
//     ManagedString zero = "0";
//     if(fractionPart!=0){
//         ManagedString fracPart(fractionPart);
//         while(fracPart.length()<5){
//             fracPart=zero+fracPart;
//         }
//         ret=ret+"."+fracPart;
//     }
//     return ret;
// }
