#include <stdlib.h>
#include <pigpio.h>

void _cb(int gpio, int level, uint32_t tick, void *user){
	printf("Level %d", level);
}

int main( int argc, char **argv ) {
	if (gpioInitialise() < 0) return 1;
	gpioSetMode(4, PI_INPUT);
	gpioSetWatchdog(4, 10);
  gpioSetAlertFuncEx(4, _cb, null);
  gpioTerminate();
}
