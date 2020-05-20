#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pigpio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

static const int MAX_CHANNELS = 128;
static const int NUM_CHANNELS = 64;
static const int HERTZ = 100;
static const int SECONDS_PER_FILE = 10;
static const uint32_t INTERVAL = 500000; /* Half second timeout for button events*/


void getTimestampStr(char *stamp){
    long            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }
    snprintf(stamp, 14, "%"PRIdMAX"%04ld", (intmax_t)s, spec.tv_nsec);
    printf("%04ld", spec.tv_nsec);
}

struct BUTTON_DATA {
	uint32_t lastEvent;
};

static void _cb(int gpio, int level, uint32_t tick, void *user){
    struct BUTTON_DATA *data;
    data = user;
	if(level == 1 && data->lastEvent + INTERVAL < tick ){
	    data->lastEvent = tick;
  	    printf("Button Pressed\n");
  	    char stamp[14];
  	    getTimestampStr(stamp);
  	    printf("Current time: %s\n", stamp);
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

void getTimestamp(){
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    //do stuff
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
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
	while(1){
        if(scansWritten == 0){
            // Open a new file
            snprintf(filename, 32, "/home/pi/ggc/in/%d.dat", fileIdx);
            printf("Writing to file %s\n", filename);
            fp = fopen(filename, "w");
        }
        result = ADC_GetScan( deviceIndex, volts );
        if( result == AIOUSB_SUCCESS ) {
            char stamp[15];
            getTimestampStr(stamp);
            fwrite(stamp , 1, 14, fp);
            fwrite(&volts, 2, NUM_CHANNELS, fp);
            ++scansWritten;
//                for( int channel = 0; channel < NUM_CHANNELS; channel++ ){
//                    printf( "%u,", volts[ channel ] );
//                }
//                printf("\n");
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

		/*
		* Wait a certain amount of time defined by HERTZ
		*/
//		msleep(FREQUENCY);
		gpioDelay(FREQUENCY);
//        gpioDelay(1); /* Delay give the button time to respond if needed */
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
