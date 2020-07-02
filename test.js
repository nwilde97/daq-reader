const daq = require('./build/Release/module');
process.on('exit', (code) => {
    console.log(`Shutting down DAQ`, daq.shutdownDAQ());
});

(()=>{
    const DEVICE_INDEX = daq.setupDAQ();
    if(DEVICE_INDEX < 0){
        console.log("No devices configured");
    } else {
        let i = 0;
        console.time("loop");
        const LOOP = setInterval(() => {
            daq.scanChannels(DEVICE_INDEX)
            if(++i > 100){
                clearInterval(LOOP);
                console.timeEnd("loop");
            }
        }, 10);
    }
})();

