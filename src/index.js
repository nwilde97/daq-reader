"use strict";
const open = require("open");
const daq = require('../build/Release/module');
process.on('exit', (code) => {
    console.log(`Shutting down DAQ`, daq.shutdownDAQ());
});
(async () => {
    //Setup express
    let express = require('express');
    const app = express();
    const server = require('http').createServer(app);
    app.use("/", express.static('public/index.html'));
    app.use(express.static('public'));
    //Setup Server
    const io = require('socket.io')(server);
    io.on('connection', client => {
        // console.log("Client connected");
        // client.on('disconnect', () => console.log("Client disconnected"));
    });


    //Setup DAQ Listener
    const DEVICE_INDEX = daq.setupDAQ();
    if(DEVICE_INDEX < 0){
        console.log("No devices configured");
    } else {
        let i = 0;
        const LOOP = setInterval(() => {
            const voltages = daq.scanChannels(DEVICE_INDEX);
            io.emit("scan", {voltages, timestamp: Date.now()});
        }, 50);
    }
    // setInterval(() => {
    //     const voltages = [];
    //     for (let i = 0; i < 64; ++i) {
    //         voltages.push(Math.random() * 10);
    //     }
    //     io.emit("scan", {voltages, timestamp: Date.now()});
    // }, 50);

    //Start the server
    server.listen(8080);
    console.log("The server is now running. You can visit it by visiting\nhttp://localhost:8080");
    await open("http://localhost:8080");
})();