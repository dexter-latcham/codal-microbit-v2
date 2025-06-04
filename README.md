# micro:bit Logging Enhancements

## FastLogger
- A new data logging component that improves performance
- Logged data is stored in a circular buffer.
- This is then saved to permanent storage once logging is complete.

### Included files:
- MicroBitCircularBuffer.cpp
- MicroBitFastLog.cpp

### Demo

```cpp
#include "MicroBit.h"
MicroBit uBit;
int main(){
    uBit.init();
    uBit.sleep(200);
    uBit.log.clear(true);

    MicroBitFastLog alog;
    alog.setTimeStamp(codal::TimeStampFormat::Seconds);
    int i=0;
    for(int x=0;x<1000;x++){
        alog.beginRow();
        alog.logData("foo",i);
        alog.endRow();
        i=i+1;
    }
}
```

## Bluetooth Log Service
A custom Bluetooth service for wirelessly accessing data log during runtime.

### Web Bluetooth Client
- Example frontend client can be found in the "btSite/" folder.
- Web Bluetooth is still experimental, this was tested on Chrome (Linux) with the --enable-experimental-web-platform-features flag


### Demo
```cpp
#include "MicroBit.h"
MicroBit uBit;

const char * const connect_emoji ="\
    000,000,000,000,000\n\
    000,000,000,000,255\n\
    000,000,000,255,000\n\
    255,000,255,000,000\n\
    000,255,000,000,000\n";

const char * const disconnect_emoji ="\
    255,000,000,000,255\n\
    000,255,000,255,000\n\
    000,000,255,000,000\n\
    000,255,000,255,000\n\
    255,000,000,000,255\n";

MicroBitImage connect(connect_emoji);
MicroBitImage disconnect(disconnect_emoji);

int connected = 0;

void onConnected(MicroBitEvent){
    uBit.display.print(connect);
    connected = 1;

    for(int i=0;i<1000;i++){
        uBit.log.beginRow();
        uBit.log.logData("foo",i);
        uBit.log.endRow();
        uBit.sleep(200);
    }
}

void onDisconnected(MicroBitEvent) {
    uBit.display.print(disconnect);
    connected = 0;
}

int main() {
    uBit.init();
    uBit.sleep(100);

    uBit.log.clear(true);
    uBit.log.setTimeStamp(codal::TimeStampFormat::Milliseconds);
    uBit.log.setSerialMirroring(false);

    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onConnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_DISCONNECTED, onDisconnected);

    new MicroBitLogService(*uBit.ble,uBit.log);
    uBit.display.print(disconnect);
    release_fiber();
}
```
