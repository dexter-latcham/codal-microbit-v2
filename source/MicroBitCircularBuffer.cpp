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


void CircBuffer::init() {
    if (status & MICROBIT_FASTLOG_STATUS_INITIALIZED){
        return;
    }

    int16Meta=NULL;
    int32Meta=NULL;
    floatMeta=NULL;
    full=false;
    dataStart = (uint8_t*)malloc(maxByteSize);
    if (dataStart == NULL) {
        return;
    }
    dataEnd = dataStart+maxByteSize;
    status |= MICROBIT_FASTLOG_STATUS_INITIALIZED;
}

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
    if(meta->start<meta->last){
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

uint8_t* CircBuffer::_getNextWriteLoc(TypeMeta* meta){
    uint8_t* possibleNext = meta->next+meta->bytes;
    if((possibleNext+meta->bytes)>dataEnd){
        possibleNext=dataStart;
        full=true;
    }
    return possibleNext;
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
    init();

    if(floatMeta!=NULL){
        float valfloat = (float)val;
        return logVal(valfloat);
    }

    if(int32Meta!=NULL){
        int32_t valInt = (int32_t)val;
        return logVal(valInt);
    }
    if(int16Meta==NULL){
        int16Meta=(TypeMeta*)malloc(sizeof(TypeMeta));
        int16Meta->start=NULL;
        int16Meta->bytes=sizeof(uint16_t);
    }
    _logData(&int16Meta,(uint8_t*)&val);
}


void CircBuffer::logVal(int32_t val) {
    init();

    if(floatMeta!=NULL){
        float valfloat = (float)val;
        return logVal(valfloat);
    }

    if(int32Meta==NULL){
        int32Meta=(TypeMeta*)malloc(sizeof(TypeMeta));
        int32Meta->start=NULL;
        int32Meta->bytes=sizeof(int32_t);
    }
    _logData(&int32Meta,(uint8_t*)&val);
}
void CircBuffer::logVal(float val) {
    init();
    if(floatMeta==NULL){
        floatMeta=(TypeMeta*)malloc(sizeof(TypeMeta));
        floatMeta->start=NULL;
        floatMeta->bytes=sizeof(float);
    }
    _logData(&floatMeta,(uint8_t*)&val);
}

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

static ManagedString floatToStr(float num){
    int integerPart = (int)num;
    int fractionPart = (int)((num-integerPart)*100000.0f);

    ManagedString ret(integerPart);
    ManagedString zero = "0";
    if(fractionPart!=0){
        ManagedString fracPart(fractionPart);
        while(fracPart.length()<5){
            fracPart=zero+fracPart;
        }
        ret=ret+"."+fracPart;
    }
    return ret;
}

int CircBuffer::_countSection(TypeMeta*meta){
    if(meta==NULL){
        return 0;
    }
    int count=0;
    uint8_t* from = meta->start;
    uint8_t* to = meta->last;

    uint8_t* current = from;
    if(to < from){
        current=from;
        while(current+meta->bytes<=dataEnd){
            count+=1;
            current+=meta->bytes;
        }
        current=dataStart;
    }
    while(current<=to){
        count+=1;
        current+=meta->bytes;
    }
    return count;
}


uint8_t* CircBuffer::_getElemIndex(TypeMeta* meta, int index){
    if(meta==NULL){
        return NULL;
    }
    uint8_t* from = meta->start;
    uint8_t* to = meta->last;

    uint8_t* current = from;
    int count=0;
    if(index<0){
        current= to;
        count=-1;
        //from to to from
        //if to is less than from, from to to start then end to to
        if(to<from){
            while(current>=dataStart){
                if(count==index){
                    return current;
                }
                current-=meta->bytes;
                count-=1;
            }
            int entries=(dataEnd-from)/meta->bytes;
            current = (from+entries*meta->bytes)-meta->bytes;
        }
        while(current>=from){
            if(count==index){
                return current;
            }
            current-=meta->bytes;
            count-=1;
        }
        return NULL;
    }

    if(to < from){
        current=from;
        while(current+meta->bytes<=dataEnd){
            if(index==count){
                return current;
            }
            current+=meta->bytes;
            count+=1;
        }
        current=dataStart;
    }
    while(current<=to){
        if(index==count){
            return current;
        }
        count+=1;
        current+=meta->bytes;
    }
    return NULL;
}

ManagedString CircBuffer::getElem(int index){
    int int16Entries = _countSection(int16Meta);
    int int32Entries = _countSection(int32Meta);
    int floatEntries = _countSection(floatMeta);
    if(index<0){
        if(floatEntries>=(-1*index)){
            uint8_t* addr = _getElemIndex(floatMeta,index);
            if(addr==NULL){
                return ManagedString::EmptyString;
            }
            if(addr!=NULL){
                float val = *(float*)addr;
                ManagedString floatStr=floatToStr(val);
                return floatStr;
            }
        }
        if((int32Entries+floatEntries)>=(-1*index)){
            uint8_t* addr = _getElemIndex(int32Meta,index+floatEntries);
            if(addr==NULL){
                return ManagedString::EmptyString;
            }
            if(addr!=NULL){
                int32_t val = *(int32_t*)addr;
                int intVal = (int)val;
                return ManagedString(intVal);
            }
        }
        if((int32Entries+floatEntries+int16Entries)>=(-1*index)){
            uint8_t* addr = _getElemIndex(int16Meta,index+floatEntries+int32Entries);
            if(addr==NULL){
                return ManagedString::EmptyString;
            }
            if(addr!=NULL){
                uint16_t num = *(uint16_t*)addr;
                return ManagedString(num);
            }
        }
        return ManagedString::EmptyString;
    }

    if(index < int16Entries){
        uint8_t* addr = _getElemIndex(int16Meta,index);
        if(addr==NULL){
            return ManagedString::EmptyString;
        }
        if(addr!=NULL){
            uint16_t num = *(uint16_t*)addr;
            return ManagedString(num);
        }
    }
    if(index < (int32Entries+int16Entries)){
        uint8_t* addr = _getElemIndex(int32Meta,index-int16Entries);
        if(addr==NULL){
            return ManagedString::EmptyString;
        }
        if(addr!=NULL){
            int32_t val = *(int32_t*)addr;
            int intVal = (int)val;
            return ManagedString(intVal);
        }
    }
    if(index < (int32Entries+int16Entries+floatEntries)){
        uint8_t* addr = _getElemIndex(floatMeta,index-int16Entries-int32Entries);
        if(addr==NULL){
            return ManagedString::EmptyString;
        }
        if(addr!=NULL){
            float val = *(float*)addr;
            ManagedString floatStr=floatToStr(val);
            return floatStr;
        }
    }
    return ManagedString::EmptyString;
}


int CircBuffer::getElementCount(){
    init();
    return _countSection(int16Meta)+_countSection(int32Meta)+_countSection(floatMeta);
}


// void CircBuffer::printElem(int index){
//     int int16Entries = _countSection(int16Meta);
//     if(index < int16Entries){
//         return _printElemIndex(int16Meta,index,print16);
//     }
//     int int32Entries = _countSection(int32Meta);
//     if(index < (int32Entries+int16Entries)){
//         return _printElemIndex(int32Meta,index-int16Entries,print32);
//     }
//     int floatEntries = _countSection(floatMeta);
//     if(index < (int32Entries+int16Entries+floatEntries)){
//         return _printElemIndex(floatMeta,index-int16Entries-int32Entries,printFloat);
//     }
// }

// void print16(const void* ptr) {
//     uint16_t val = *(uint16_t*)ptr;
//     ManagedString toSend = ManagedString("16: ")+ManagedString(val)+ManagedString("\n");
//     uBit.serial.printf(toSend.toCharArray());
// }

// void print32(const void* ptr) {
//     int32_t val = *(int32_t*)ptr;
//     int intVal = (int)val;
//     ManagedString toSend = ManagedString("32: ")+ManagedString(intVal)+ManagedString("\n");
//     uBit.serial.printf(toSend.toCharArray());
// }

// void printFloat(const void* ptr) {
//     float val = *(float*)ptr;
//     ManagedString floatStr=floatToStr(val);
//     ManagedString toSend = ManagedString("fl: ")+floatStr+ManagedString("\n");
//     uBit.serial.printf(toSend.toCharArray());
// }

// void CircBuffer::_printSection(TypeMeta* meta, PrintFunc func){
//     if(meta==NULL){
//         return;
//     }
//     uint8_t* from = meta->start;
//     uint8_t* to = meta->last;

//     uint8_t* current = from;
//     if(to < from){
//         current=from;
//         while(current+meta->bytes<=dataEnd){
//             func(current);
//             current+=meta->bytes;
//         }
//         current=dataStart;
//     }
//     while(current<=to){
//         func(current);
//         current+=meta->bytes;
//     }
// }

// void CircBuffer::print() {
//     _printSection(int16Meta,print16);
//     _printSection(int32Meta,print32);
//     _printSection(floatMeta,printFloat);
// }


// void CircBuffer::_printElemIndex(TypeMeta* meta, int index,PrintFunc func){
//     if(meta==NULL){
//         return;
//     }
//     int count=0;

//     uint8_t* from = meta->start;
//     uint8_t* to = meta->last;

//     uint8_t* current = from;
//     if(to < from){
//         current=from;
//         while(current+meta->bytes<=dataEnd){
//             if(index==count){
//                 func(current);
//                 return;
//             }
//             current+=meta->bytes;
//             count+=1;
//         }
//         current=dataStart;
//     }
//     while(current<=to){
//         if(index==count){
//             func(current);
//             return;
//         }
//         count+=1;
//         current+=meta->bytes;
//     }
// }

