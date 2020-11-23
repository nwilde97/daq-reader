"use strict";
const daq = require('../build/Release/module');
const gnuplot = require('gnuplot');
const fs = require("fs").promises;
const EOL = require('os').EOL;
process.on('exit', (code) => {
    console.log(`Shutting down DAQ`, daq.shutdownDAQ());
    console.log("Done", code);
});
(async () => {

    const CHANNEL = parseInt(process.argv[3])-1;
    if(CHANNEL < 1 || CHANNEL > 64){
        console.log(`Invalid channel ${process.argv[3]}`);
        exit(1);
    }

    let data = [];
    data[999] = 0;
    data.fill(0,0,1000);

    const plot = gnuplot();
    plot.set("xrange [0:1000]");
    plot.set("yrange [0:20]");
    plot.println(`plot "data.out" using 1:2 with lines`);
    plot.println(`bind "q" "unset output ; exit gnuplot"`);

    //Setup DAQ Listener
    const DEVICE_INDEX = daq.setupDAQ();
    if(DEVICE_INDEX < 0){
        console.log("No devices configured");
        plot.end();
    } else {
        let i = 0;
        const LOOP = setInterval(async () => {
            const voltages = daq.scanChannels(DEVICE_INDEX);
            const x = voltages[CHANNEL];
            data = data.slice(1);
            data.push(x);
            const out = data.map((val, idx) => `${idx}\t${val}`).join(EOL);
            await fs.writeFile("data.out", out);
            plot.println("replot");
        }, 50);
    }
})().catch(err => console.log(err));
