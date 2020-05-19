#include <stdio.h>
#include <pigpio.h>
#include <stdlib.h>

struct BUTTON_DATA {
	double lastEvent;
};

static void _cb(int gpio, int level, uint32_t tick, void *user){
    struct BUTTON_DATA *data;
    data = user;
    double now = (double)tick;
    double INTERVAL = (double)CLOCKS_PER_SEC;
	if(level == 1 && data->lastEvent + INTERVAL < now ){
	    data->lastEvent = now;
  	    printf("Button Pressed %d %d %d\n", data->lastEvent , INTERVAL , now);
	} else if(level == 1){
	printf("Button Pressed %d %d %d\n", data->lastEvent , INTERVAL , now);
	}
}

int setupButtonListener( ) {
    struct BUTTON_DATA *data;
    data = malloc(sizeof(data));
    data->lastEvent = 0;
	if (gpioInitialise() < 0){
	    printf("Unable to initialize button");
	     return 1;
	}
	gpioSetMode(4, PI_INPUT);
	gpioSetWatchdog(4, 100);
  gpioSetAlertFuncEx(4, _cb, data);
}

int main(){
setupButtonListener( );
printf("Listening\n");
while(1){
    gpioDelay(100);
}
}
