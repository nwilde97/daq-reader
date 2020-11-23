"use strict";
// const daq = require('../build/Release/module');
const gnuplot = require('gnuplot');
const fs = require("fs").promises;
const EOL = require('os').EOL;
process.on('exit', (code) => {
    // console.log(`Shutting down DAQ`, daq.shutdownDAQ());
    console.log("Done", code);
});
function sleep(ms) {
    return new Promise((resolve) => {
        setTimeout(resolve, ms);
    });
}
(async () => {
    let history = [];
    history[999] = 0;
    history.fill(0,0,1000);

    for(let p = 0; p < 1000; ++p ) {
        let x = Math.random() * 10;
        history = history.slice(1);
        history.push(x);
    }

    const plot = gnuplot();
    plot.set("xrange [0:1000]");
    plot.set("yrange [0:20]");
    plot.println(`plot "data.out" using 1:2 with lines`);
    plot.println(`bind "q" "unset output ; exit gnuplot"`);
    for(let i = 0; i < 1000; ++i){
        await sleep(10);
        for(let p = 0; p < 100; ++p ) {
            let x = Math.random() * 10;
            history = history.slice(1);
            history.push(x);
        }
        const out = history.map((val, idx) => `${idx}\t${val}`).join(EOL);
        await fs.writeFile("data.out", out);
        plot.println("replot");
    }
})().catch(err => console.log(err));
