#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pigpio.h>
#include <stdlib.h>

    static const int MAX_CHANNELS = 128;
    static const int NUM_CHANNELS = 64;
    static const int HERTZ = 100;
    static const int SECONDS_PER_FILE = 10;

int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

struct BUTTON_DATA {
	int pressed;
};

static void _cb(int gpio, int level, uint32_t tick, void *user){
    double *lastEvent;
    lastEvent = user;
	if(level == 1 && lastEvent + CLOCKS_PER_SEC * 1 > clock()){
  	    printf("Button Pressed\n");
	}
}

int setupButtonListener( ) {
    double lastEvent = 0;
	if (gpioInitialise() < 0){
	    printf("Unable to initialize button");
	     return 1;
	}
	gpioSetMode(4, PI_INPUT);
	gpioSetWatchdog(4, 10000);
  gpioSetAlertFuncEx(4, _cb, &lastEvent);
}

unsigned long setupDAQ() {

    unsigned char gainCodes[ NUM_CHANNELS ];
    uint64_t serialNumber;
    unsigned long result;
    unsigned long deviceMask;
    int MAX_NAME_SIZE = 20;
    char name[ MAX_NAME_SIZE + 2 ];
    unsigned long productID, nameSize, numDIOBytes, numCounters;
    unsigned long deviceIndex = 0;
    unsigned long deviceFound = 0;
    ADConfigBlock configBlock = {0};
    char *calibration_type;
    /*
     * MUST call AIOUSB_Init() before any meaningful AIOUSB functions;
     * AIOUSB_GetVersion() above is an exception
     */
    result = AIOUSB_Init();
    if ( result != AIOUSB_SUCCESS ) {
        printf( "AIOUSB_Init() error: %d\n" , (int)result);
        return -1;
    }

    deviceMask = GetDevices(); /** call GetDevices() to obtain "list" of devices found on the bus */
    if ( !deviceMask  ) {
        printf( "No ACCES devices found on USB bus\n" );
        return -1;
    }

    /*
     * at least one ACCES device detected, but we want one of a specific type
     */
    while( deviceMask != 0 ) {
        if( ( deviceMask & 1 ) != 0 ) {
            // found a device, but is it the correct type?
            nameSize = MAX_NAME_SIZE;
            result = QueryDeviceInfo( deviceIndex, &productID, &nameSize, name, &numDIOBytes, &numCounters );
            if( result == AIOUSB_SUCCESS ) {
                if( productID >= USB_AI16_16A && productID <= USB_AIO12_128E ) {
                    deviceFound = 1;                    // found a USB-AI16-16A family device
                    break;
                }
            } else
                printf( "Error '%s' querying device at index %lu\n",
                        AIOUSB_GetResultCodeAsString( result ),
                        deviceIndex
                        );
        }
        deviceIndex++;
        deviceMask >>= 1;
    }

    if (deviceFound < 1 ) {
        printf( "Failed to find USB-AI16-16A device\n" );
        return -1;
    }

    AIOUSB_Reset( deviceIndex );

    /** automatic A/D calibration */
    calibration_type = strdup(":AUTO:");
    result = ADC_SetCal( deviceIndex, calibration_type );

    if( result == AIOUSB_SUCCESS ) {
        printf( "Automatic calibration completed successfully\n" );
    } else if (result == AIOUSB_ERROR_NOT_SUPPORTED) {
        printf ("Automatic calibration not supported on this device\n");
    } else {
        printf( "Error '%s' performing automatic A/D calibration\n", AIOUSB_GetResultCodeAsString( result ) );
        return -1;
    }

    /*
     * Scan channels and measure voltages
     */
    for( int channel = 0; channel < NUM_CHANNELS; channel++ ){
        gainCodes[ channel ] = AD_GAIN_CODE_0_10V;
    }
    ADC_RangeAll( deviceIndex, gainCodes, AIOUSB_TRUE );
    ADC_SetOversample( deviceIndex, 10 );
    ADC_SetScanLimits( deviceIndex, 0, NUM_CHANNELS - 1 );
    ADC_ADMode( deviceIndex, 0 /* TriggerMode */, AD_CAL_MODE_NORMAL );
    return deviceIndex;
}

int main( int argc, char **argv ) {
    setupButtonListener();
	unsigned long deviceIndex = setupDAQ();

	/*
	* Initialize variables used in scan loop
	*/
    unsigned short volts[ MAX_CHANNELS ];
    int FREQUENCY = 1000000 * 1 / HERTZ;
    unsigned long result;
	int fileIdx = 0; /* Index to be used in filenames */
	int scansWritten = 0; /* Used to track how many scans have been written to a file */
	FILE *fp; /* File pointer to write to */
	char filename[32]; /* Placeholder for filename */

	/* End Initialization */

	/* Begin scanning loop */
	clock_t tick = clock();
	double nextTick = (double)tick + (double)CLOCKS_PER_SEC / (double)HERTZ;
	while(1){
	    tick = clock();
	    if((double)tick >= nextTick){
	        nextTick = nextTick + (double)CLOCKS_PER_SEC / (double)HERTZ;
            if(scansWritten == 0){
                // Open a new file
                snprintf(filename, 32, "/home/pi/ggc/in/%d.dat", fileIdx);
                printf("Writing to file %s\n", filename);
                fp = fopen(filename, "w");
            }
            result = ADC_GetScan( deviceIndex, volts );
            if( result == AIOUSB_SUCCESS ) {
                fwrite(&volts, 2, NUM_CHANNELS, fp);
                ++scansWritten;
            } else {
                printf( "Error '%s' performing A/D channel scan\n", AIOUSB_GetResultCodeAsString( result ) );
            }

            /*
            * If we've reached the capacity for the file, then close it
            */
            if(scansWritten >= HERTZ * SECONDS_PER_FILE){
                scansWritten = 0;
                ++fileIdx;
                fclose(fp);
            }

	    }
		/*
		* Wait a certain amount of time defined by HERTZ
		*/
//		msleep(FREQUENCY);
//		gpioDelay(FREQUENCY);
        gpioDelay(1); /* Delay give the button time to respond if needed */
    }

    /* End Scanning loop */

/*
 * MUST call AIOUSB_Exit() before program exits,
 * but only if AIOUSB_Init() succeeded
 */
 out_after_init:
    AIOUSB_Exit();

 out_main:
    return ( int ) result;
}
