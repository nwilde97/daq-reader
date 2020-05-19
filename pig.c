#include <stdio.h>
#include <pigpio.h>
#include <stdlib.h>

struct BUTTON_DATA {
	uint32_t lastEvent;
};

static void _cb(int gpio, int level, uint32_t tick, void *user){
    struct BUTTON_DATA *data;
    double INTERVAL = 1000;
    data = user;
	if(level == 1 && &data->lastEvent + INTERVAL < tick ){
	    data->lastEvent = tick;
  	    printf("Button Pressed %d %d %d\n", &data->lastEvent , INTERVAL , tick);
	} else if(level == 1){
	printf("Button Pressed %d %d %d\n", &data->lastEvent , INTERVAL , tick);
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
