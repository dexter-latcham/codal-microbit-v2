var CUSTOM_SRV = 'e95d0754-251d-470a-a062-fa1922dfa9a8';


function getSupportedProperties(characteristic) {
    let supportedProperties = [];
    for (const p in characteristic.properties) {
        if (characteristic.properties[p] === true) {
            supportedProperties.push(p.toUpperCase());
        }
    }
    return '[' + supportedProperties.join(', ') + ']';
}

const charUUIDMin={
    ROWCOUNT: 'ca4c',
    HEADERCOUNT: 'fb25',
    GETHEADER: 'fb58',
    GETROW: 'fb59',
    NEWROWLOGGED: 'fb60',
};

const basePrefix = 'e95d';
const baseSuffix = '251d-470a-a062-fa1922dfa9a8';

const characteristicUUIDs = {};
for (const [key, shortUUID] of Object.entries(charUUIDMin)) {
    characteristicUUIDs[key] = `${basePrefix}${shortUUID}-${baseSuffix}`;
}



class btLog{
    constructor(){
        this.connected = false;

        this.onConnectCallback=function(){};
        this.onDisconnectCallback=function(){};

        this.characteristics={};
        this.headers=[];
        this.headerCount=0;
        this.rowCount=0;
        this.data=[];

        this.rowBuffer="";
    }

    onConnect(callbackFunction){
        this.onConnectCallback=callbackFunction;
    }

    onDisconnect(callbackFunction){
        this.onDisconnectCallback=callbackFunction;
    }



    async getRow(number) {
        try {
            const char = this.characteristics.GETROW;
            const buffer = new ArrayBuffer(2);
            const view = new DataView(buffer);
            view.setUint16(0, number, true);

            await char.startNotifications();
            return await new Promise((resolve, reject) => {
                let accumulated = '';
                const decoder = new TextDecoder('utf-8');

                const handler = (event) => {
                    const value = event.target.value;
                    const chunk = decoder.decode(value);
                    accumulated += chunk;
                    if (chunk.includes('\n')) {
                        char.removeEventListener('characteristicvaluechanged', handler);
                        let row = accumulated.replace(/\n/g, '').trim()
                        resolve(row.split(","))
                    }
                };

                char.addEventListener('characteristicvaluechanged', handler);

                char.writeValue(buffer).catch(error => {
                    char.removeEventListener('characteristicvaluechanged', handler);
                    reject(error);
                });
                setTimeout(() => {
                    char.removeEventListener('characteristicvaluechanged', handler);
                    resolve([])
                }, 5000);
            });
        } catch (error) {
            console.error('Failed to get header', error);
            throw error;
        }
    }

    async getHeader(number) {
        try {
            const char = this.characteristics.GETHEADER;
            const buffer = new ArrayBuffer(2);
            const view = new DataView(buffer);
            view.setUint16(0, number, true);

            await char.startNotifications();
            return await new Promise((resolve, reject) => {
                let accumulated = '';
                const decoder = new TextDecoder('utf-8');

                const handler = (event) => {
                    const value = event.target.value;
                    const chunk = decoder.decode(value);
                    accumulated += chunk;
                    if (chunk.includes('\n')) {
                        char.removeEventListener('characteristicvaluechanged', handler);
                        resolve(accumulated.replace(/\n/g, ''));
                    }
                };

                char.addEventListener('characteristicvaluechanged', handler);

                char.writeValue(buffer).catch(error => {
                    char.removeEventListener('characteristicvaluechanged', handler);
                    reject(error);
                });
                setTimeout(() => {
                    char.removeEventListener('characteristicvaluechanged', handler);
                    reject(new Error('Timeout while waiting for full header'));
                }, 5000);
            });
        } catch (error) {
            console.error('Failed to get header', error);
            throw error;
        }
    }

    async handleHeaderCount(event){
        const value = event.target.value;
        let newHeaderCount = value.getUint16(0,true);
        if(newHeaderCount=this.headerCount){
            for(let i=this.headerCount;i<newHeaderCount;i++){
                this.headers.push(await this.getHeader(i));
            }
            this.headerCount=newHeaderCount;
        }
    }

    async handleNewRow(event) {
        const value = event.target.value;
        const decoder = new TextDecoder('utf-8');
        const receivedText = decoder.decode(value);
        this.rowBuffer+=receivedText;
        let parts = this.rowBuffer.split("\n");
        for(let i=0;i<parts.length-1;i++){
            let row = parts[i].trim()
            if(row){
                let values = row.split(",");
                this.data.push(values);
            }
        }
        this.rowBuffer=parts[parts.length-1]
    }


    async getRowCount(){
        const countValue = await this.characteristics.ROWCOUNT.readValue();
        let rowCount = countValue.getUint16(0, true);
        return rowCount
    }


    async getStoredRows(){
        const rowCountBytes = await this.characteristics.ROWCOUNT.readValue();
        let rowCount = rowCountBytes.getUint16(0, true);
        let returnedRows=[];
        for(let i=0;i<rowCount;i++){
            let rowN = await this.getRow(i);
            if(rowN.length!=0){
                returnedRows.push(rowN);
            }
        }
        if(returnedRows.length!=0){
            this.data = [...returnedRows,...this.data]
        }
    }

    enableLiveRows(){
        this.characteristics.NEWROWLOGGED.startNotifications();
        this.characteristics.NEWROWLOGGED.addEventListener('characteristicvaluechanged',this.handleNewRow.bind(this));
    }

    async onConnected(){
        const countValue = await this.characteristics.HEADERCOUNT.readValue();
        this.headerCount = countValue.getUint16(0, true);
        for(let i=0;i<this.headerCount;i++){
            this.headers.push(await this.getHeader(i));
        }
        this.characteristics.HEADERCOUNT.startNotifications();
        this.characteristics.HEADERCOUNT.addEventListener('characteristicvaluechanged',this.handleHeaderCount.bind(this));

        await this.getStoredRows()
        this.enableLiveRows()
    }

    onDisconnected() {
        console.warn("Device disconnected");
    }

    async connectToDevice(){
        var options = {
            filters: [{namePrefix: 'BBC micro:bit'}],
        };
        options.optionalServices = [CUSTOM_SRV];
        let device = await navigator.bluetooth.requestDevice(options);

        device.addEventListener('gattserverdisconnected', this.onDisconnected);

        console.log("Connecting to GATT server...");
        let server = await device.gatt.connect();

        console.log("Getting primary service...");
        let service = await server.getPrimaryService(CUSTOM_SRV);

        const allCharacteristics = await service.getCharacteristics();
        console.log("Available characteristics UUIDs:");
        allCharacteristics.forEach(char => {
            console.log(char.uuid);
        });

        for (const [name, uuid] of Object.entries(characteristicUUIDs)) {
            console.log(`Getting characteristic: ${name}`);
            const char = await service.getCharacteristic(uuid);
            this.characteristics[name] = char;
            console.log(getSupportedProperties(char));
        }
        await this.onConnected();
    } catch (error) {
        console.error("Bluetooth connection failed:", error);
    }
}


