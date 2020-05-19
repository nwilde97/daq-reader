#include <stdio.h>
#include <pigpio.h>
#include <stdlib.h>

struct BUTTON_DATA {
	int pressed;
};

static void _cb(int gpio, int level, uint32_t tick, void *user){
	struct BUTTON_DATA * data;
	data = user;
	if(level == 1 && data->pressed < 1){
  	printf("Pressed\n");
		data->pressed = 1;
	}
}

int main( int argc, char **argv ) {
	struct BUTTON_DATA * data;
	data = malloc(sizeof(data));
	data->pressed = 0;
	if (gpioInitialise() < 0) return 1;
	gpioSetMode(4, PI_INPUT);
	gpioSetWatchdog(4, 1000);
  gpioSetAlertFuncEx(4, _cb, data);
  while(1){
  	if(data->pressed == 1){
  		printf("Logging event %d\n", data->pressed);
  		data->pressed = 0;
  	}
  }
  free(data);
  data = NULL;
  gpioTerminate();
}
