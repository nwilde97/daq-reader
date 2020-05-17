# DAQ Reader

This project contains code to read from an ACCESIO DAQ Reader at a rate of 100 hertz and log the data to .dat files in 10 second chunks. Building the code produces the executable that can be run

## Prerequisites
```
1) Docker installed
2) Directory for files to be written to: /home/pi/ggc/in
```

## Building compiled code
```
docker build --tag daq-reader .
docker run 
docker run -it -v $(pwd):/root/work daq-reader:1.0 gcc -std=gnu99 -D_GNU_SOURCE -pthread -fPIC main.c -laiousb -lusb-1.0 -lm -o main
```

