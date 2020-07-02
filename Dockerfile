FROM debian:latest

RUN apt-get update && apt-get install -y libusb-1.0-0 libusb-1.0-0-dev cmake build-essential git

RUN cd / &&\
	git clone https://github.com/accesio/AIOUSB.git &&\
	cd AIOUSB/AIOUSB &&\
    mkdir build &&\
    cd build &&\
    cmake -DBUILD_AIOUSBCPPDBG_SHARED=OFF -DBUILD_AIOUSBCPP_SHARED=OFF -DBUILD_AIOUSBDBG_SHARED=OFF -DBUILD_AIOUSB_SHARED=OFF .. &&\
    make &&\
    make install

RUN cd /AIOUSB/AIOUSB && \
	chmod 755 sourceme.sh && \
	./sourceme.sh && \
	mkdir /root/work

ENV CPATH=/usr/include/libusb-1.0/:/usr/local/include/aiousb

RUN cd / &&\
	git clone https://github.com/joan2937/pigpio.git && \
	cd pigpio && \
    make && \
    make install

RUN apt-get install -y curl && \
	curl -sL https://deb.nodesource.com/setup_14.x | bash - && \
    apt-get install -y nodejs

RUN npm install -g node-gyp

WORKDIR /root/work


