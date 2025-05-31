class basicLinePlot {
    constructor() {
        this.handlers=[];
        this.headersList=[];
        this.headers = ["time"];
        this.data = [];
        this.dataParsedUpto=[];

        document.getElementById("dataChart").style.display="block";
        let ctx = document.getElementById("dataChart").getContext("2d");
        this.chart = new Chart(ctx, {
            type: 'line',
            data: {
                datasets: this._initDatasets()
            },
            options: {
                responsive: true,
                parsing: false,
                spanGaps: false, // keep gaps for missing values
                scales: {
                    x: {
                        type: 'linear',
                        title: {
                            display: true,
                            text: 'Time'
                        }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Values'
                        }
                    }
                }
            }
        });
    }

    _randomColor() {
        const r = Math.floor(Math.random() * 200);
        const g = Math.floor(Math.random() * 200);
        const b = Math.floor(Math.random() * 200);
        return `rgb(${r}, ${g}, ${b})`;
    }
    _initDatasets() {
        return this.headers.slice(1).map((header,index) => ({
            label: header,
            data: [],
            borderWidth: 2,
            borderColor: this._randomColor(),
            fill: false
        }));
    }


    _updateChartDatasets() {
        this.chart.data.datasets.forEach(ds => ds.data = []);

        for (let col = 1; col < this.headers.length; col++) {
            const dataset = this.chart.data.datasets[col - 1];
            dataset.data = this.data
            .filter(row => row[col] !== undefined && row[col] !== null)
            .map(row => ({ x: row[0], y: row[col] }));
        }
        this.chart.update();
    }

    addDevice(device){
        this.handlers.push(device);
        this.dataParsedUpto.push(0);
        this.headersList.push([]);
    }

    checkHeaders(){
        for(let i=0;i<this.handlers.length;i++){
            let correctHeaders=this.handlers[i].headers;
            for(let headerI=1;headerI<correctHeaders.length;headerI++){
                let headerName = this.handlers[i].getName()+correctHeaders[headerI];
                if(!(this.headers.includes(headerName))){
                    this.headers.push(headerName);
                    this.headersList[i].push(headerName);
                    const newColIndex = this.headers.length - 1;
                    this.data = this.data.map(row => {
                        const newRow = [...row];
                        newRow[newColIndex] = undefined;
                        return newRow;
                    });
                    this.chart.data.datasets.push({
                        label: headerName,
                        data: [],
                        borderColor: this._randomColor(),
                        borderWidth: 2,
                        fill: false
                    });
                }
            }
        }
    }

    update(){
        if(this.handlers.length==0){
            return;
        }

        this.checkHeaders();
        for(let i=0;i<this.handlers.length;i++){
            let rowsToAdd = this.handlers[i].data.slice(this.dataParsedUpto[i]);
            this.dataParsedUpto[i]=this.handlers[i].data.length;
            if(rowsToAdd.length!=0){
                for(let rowIndex=0;rowIndex<rowsToAdd.length;rowIndex++){
                    let row = rowsToAdd[rowIndex];
                    if((row.length!=0) && (!(isNaN(Number(row[0]))))){
                        let newRow=[Number(row.shift())]
                        for(let headerIndex=1;headerIndex<this.headers.length;headerIndex++){
                            let toPush = NaN;
                            if(this.headersList[i].includes(this.headers[headerIndex])){
                                toPush = Number(row.shift());
                            }
                            if(!isNaN(toPush)){
                                newRow.push(toPush);
                            }else{
                                newRow.push(undefined);
                            }
                        }
                        this.data.push(newRow);
                    }
                }
            }
        }
        this.data.sort((a, b) => a[0] - b[0]); // Sort by timestamp
        this._updateChartDatasets();
    }
}

class graph{
    constructor(){
        this.graphInstance;
        this.graphType="line";
        this.graphTitle="";
    }
    createGraph(headers){
        if(headers.length==0 || this.graphType==""){
            return;
        }
        document.getElementById("dataChart").style.display="block";
        if(this.graphInstance){
            this.graphInstance.delete();
        }
        if(this.graphType == "line"){
            this.graphInstance = new lineChart(this.graphTitle,headers,this.graphType)
        }else if(this.graphType == "bar" || this.graphType == "pie"){
            this.graphInstance = new sumChart(this.graphTitle,headers,this.graphType)
        }
    }

    updateGraph(headers,data){
        if(!this.graphInstance){
            this.createGraph(headers);
            return;
        }
        if(!data){
            return;
        }
        if(this.graphInstance.getDrawnTo() == data.length){
            return;
        }
        let newData = data.slice(this.graphInstance.getDrawnTo(),data.length);
        let dataWithoutHeaders=[];

        for(const stringRow of newData){
            let parsedDataRow=[];
            for(const elem of stringRow){
                if(elem.length==0){
                    parsedDataRow.push(NaN);
                }else{
                    parsedDataRow.push(elem*1.0);
                }
            }
            if(! parsedDataRow.every(Number.isNaN)){
                dataWithoutHeaders.push(parsedDataRow);
            }
        }

        this.graphInstance.updateChartData(dataWithoutHeaders);
        this.graphInstance.updateDrawnUpTo(newData.length);
        this.graphInstance.updateHeaders(headers);
    }

}

class graphBase{
    constructor(title="",headers=[],type=""){
        this.chart;
        this.title = title;
        this.type = type;
        this.headers = headers;
        this.drawnUpTo=0;
    }


    updateDrawnUpTo(newRows){
        this.drawnUpTo=this.drawnUpTo+newRows;
    }

    getDrawnTo(){
        return this.drawnUpTo;
    }

    delete(){
        if(this.chart){
            this.chart.destroy();
        }
    }

    drawGraph(dataSets,xLables=[],scales={}){
        let ctx = document.getElementById("dataChart").getContext("2d");
        this.chart = new Chart(ctx, {
            type: this.type,
            data: {
                labels: xLables,
                datasets: dataSets,
            },
            options: {
                responsive: true,
                scales : scales,
                plugins: {
                    title: {
                        display: true,
                        text: this.title,
                        font: {
                            size:20
                        }
                    },
                    legend: {  // Increase legend font size
                        labels: {
                            font: {
                                size: 16 // Adjust the size as needed
                            }
                        }
                    }
                }
            },
        });
    }


    updateHeaders(newHeaders){
        if(!this.chart){
            this.headers = newHeaders;
            return;
        }
        this.headers = newHeaders;
        this.updateHeadersImpl();
    }
    updateHeadersImpl(){
        throw new Error("update headers abstract method called")

    }

    updateTitle(newTitle){
        if(newTitle == this.title){
            return;
        }
        this.title = newTitle;
        if(this.chart){
            this.chart.options.plugins.title.text = newTitle;
            this.chart.update();
        }

    }
    updateChartData(newData){
        throw new Error("update with new data abstract method called");
    }
}

class sumChart extends graphBase{
    constructor(title="",headers=[],type=""){
        super(title,headers,type);
        this.drawSumChart();
    }
    drawSumChart(){
        let labels = this.headers;
        if(this.headers.length !=0){
            if(this.headers[0].startsWith("Time")){
                labels = labels.slice(1);
            }
        }
        let dataToPlot = [{
                label: "",
                data: new Array(labels.length).fill(0),
                backgroundColor: ["red", "blue", "green", "yellow", "purple", "orange"]
            }];

        this.drawGraph(dataToPlot,labels,{});
    }

    updateHeadersImpl(){
        let labels = this.headers;
        if(this.headers[0].startsWith("Time")){
            labels = this.headers.slice(1);
        }
        let currentColumnCount = this.chart.data.labels.length;
        for(let i=currentColumnCount;i<labels.length;i++){
            this.chart.data.datasets[0].data.push(0);
        }
        this.chart.data.labels = labels;
    }

    updateChartData(newData){
        if(!this.chart){
            return;
        }

        let rowOffset =0;
        if(this.headers[0].startsWith("Time")){
            rowOffset = 1;
        }
        for(let row of newData){
            let maxI = Math.min(row.length,this.headers.length);
            for(let i=rowOffset;i<maxI;i++){
                let elem = row[i];
                if(!isNaN(elem)){
                    this.chart.data.datasets[0].data[i-rowOffset]+=elem;
                }
            }
        }

        this.chart.update();
    }
}

class lineChart extends graphBase{
    constructor(title="",headers=[],type=""){
        super(title,headers,type);
        this.lastDatapointTimestamp =0;
        this.timestampOffset=0;
        this.drawLineChart();
    }

    drawLineChart(){
        let xAxisTitle = "";
        if(this.headers.length!=0){
            xAxisTitle = this.headers[0];
        }
        let tmpScales = {x: { 
                    type:"linear",
                    title: { 
                        display: true,
                        text: xAxisTitle,
                        font: { size: 18 }
                    },
        }};
        let tmpDatasets=[];
        if(this.headers.length!=0){
            tmpDatasets = this.headers.slice(1).map((header,index) => ({
                label:header,
                data:[],
                borderColor: `hsl(${index * 60}, 100%, 50%)`,
                fill: false
            }));
        }
        this.lastDatapointTimestamp =0;
        this.timestampOffset=0;
        this.drawGraph(tmpDatasets,[],tmpScales);
    }
    updateHeadersImpl(){
        let currentColumnCount = this.chart.data.datasets.length;
        if(currentColumnCount==0){
            this.chart.options.scales.x.title.text=this.headers[0];
        }

        for(let i=currentColumnCount;i<this.headers.length-1;i++){
            this.chart.data.datasets.push({
                label:this.headers[i+1],
                data:[],
                borderColor: `hsl(${i * 60}, 100%, 50%)`,
                fill: false
            })

        }
    }
    updateChartData(newData){
        if(!this.chart){
            return;
        }

        for(let row of newData){
            if(this.headers[0].startsWith("Time")){
                if(row[0] < this.lastDatapointTimestamp){
                    this.addDividorToLineChart(this.lastDatapointTimestamp);
                    this.timestampOffset = this.lastDatapointTimestamp-row[0]
                }
                this.lastDatapointTimestamp = row[0];
                row[0] = row[0]+this.timestampOffset
            }
            let maxI = Math.min(row.length,this.headers.length);
            for(let i=1;i<maxI;i++){
                this.chart.data.datasets[i-1].data.push({y:row[i],x:""+row[0]});
            }
        }
        this.chart.update();
    }

    addDividorToLineChart(xPosition){
        if (!this.chart.options.plugins.annotation) {
            this.chart.options.plugins.annotation = { annotations: [] };
        }

        // Add a new vertical line annotation with a hover tooltip
        this.chart.options.plugins.annotation={annotations:[{
            type: 'line',
            scaleID: 'x',
            mode: 'vertical',
            value: xPosition,
            borderColor: 'red',
            borderWidth: 2,
            borderDash: [6, 6], // Dashed line (optional)
            label: {
                display: false,
                content: `time values restart from 0`,
                backgroundColor: "rgba(0,0,0,0.8)", // Dark background
                color: "white",
                font: {
                    size: 14
                },
                padding: 6
            },
            enter({ element }, event) {
                element.label.options.display = true;
                return true; // force update
            },
            leave({ element }, event) {
                element.label.options.display = false;
                return true;
            }
                
        }]};
    }
}
