#include <aiousb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

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

int main( int argc, char **argv ) {
    int CAL_CHANNEL = 5;
    int MAX_CHANNELS = 128;
    int NUM_CHANNELS = 64;
    unsigned short counts[ MAX_CHANNELS ];
    unsigned short volts[ MAX_CHANNELS ];
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
    time_t start;
    int hertz;
    int frequency;

    /*
     * MUST call AIOUSB_Init() before any meaningful AIOUSB functions;
     * AIOUSB_GetVersion() above is an exception
     */
    result = AIOUSB_Init();
    if ( result != AIOUSB_SUCCESS ) {
        printf( "AIOUSB_Init() error: %d\n" , (int)result);
        goto out_main;
    }

    deviceMask = GetDevices(); /** call GetDevices() to obtain "list" of devices found on the bus */
    if ( !deviceMask  ) {
        printf( "No ACCES devices found on USB bus\n" );
        goto out_after_init;
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
        goto out_after_init;
    }

    AIOUSB_Reset( deviceIndex );

    /*
     * demonstrate A/D configuration; there are two ways to configure the A/D;
     * one way is to create an ADConfigBlock instance and configure it, and then
     * send the whole thing to the device using ADC_SetConfig(); the other way
     * is to use the discrete API functions such as ADC_SetScanLimits(), which
     * send the new settings to the device immediately; here we demonstrate the
     * ADConfigBlock technique; below we demonstrate use of the discrete functions
     */
//    AIOUSB_InitConfigBlock( &configBlock, deviceIndex, AIOUSB_FALSE );
//
//    AIOUSB_SetAllGainCodeAndDiffMode( &configBlock, AD_GAIN_CODE_10V, AIOUSB_FALSE );
//    ADCConfigBlockSetCalMode( &configBlock, AD_CAL_MODE_NORMAL );
//    ADCConfigBlockSetTriggerMode( &configBlock, 0 );
//    AIOUSB_SetScanRange( &configBlock, 0, 63 );
//    ADCConfigBlockSetOversample( &configBlock, 0 );
//
//    result = ADC_SetConfig( deviceIndex, configBlock.registers, &configBlock.size );
//
//    if ( result != AIOUSB_SUCCESS ) {
//        printf( "Error '%s' setting A/D configuration\n",
//                AIOUSB_GetResultCodeAsString( result )
//                );
//        goto out_after_init;
//    }
//    printf( "A/D settings successfully configured\n" );

    /** automatic A/D calibration */
    calibration_type = strdup(":AUTO:");
    result = ADC_SetCal( deviceIndex, calibration_type );

    if( result == AIOUSB_SUCCESS ) {
        printf( "Automatic calibration completed successfully\n" );
    } else if (result == AIOUSB_ERROR_NOT_SUPPORTED) {
        printf ("Automatic calibration not supported on this device\n");
    } else {
        printf( "Error '%s' performing automatic A/D calibration\n", AIOUSB_GetResultCodeAsString( result ) );
        goto out_after_init;
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

    start = time(NULL);
    hertz = 100;
    frequency = 1000 * 1 / hertz;
//    for(;time(NULL) < start + 10;){
    for(int i = 0;; ++i){
        char filename[32];
        int byteCount = 0;
        snprintf(filename, 32, "/home/pi/ggc/in/%d.dat", i);
        printf("\nWriting to file %s", filename);
        FILE *fp = fopen(filename, "w");
        for(int j = 0; j < hertz * 10; ++j){
            result = ADC_GetScan( deviceIndex, volts );
            if( result == AIOUSB_SUCCESS ) {
                for( int channel = 0; channel < NUM_CHANNELS; channel++ ){
                    printf( "%u,", volts[ channel ] );
                }
                fwrite(&volts, 2, NUM_CHANNELS, fp);
                byteCount += 2 * NUM_CHANNELS;
                printf("\n");
            } else {
                printf( "Error '%s' performing A/D channel scan\n", AIOUSB_GetResultCodeAsString( result ) );
            }
            msleep(frequency);
        }
        printf("\nWrote %d bytes to file %s", byteCount, filename);
        fclose(fp);
    }

    /*
     * demonstrate reading a single channel in volts
     */
//    result = ADC_GetChannel( deviceIndex, CAL_CHANNEL, &volts[ CAL_CHANNEL ] );
//    if( result == AIOUSB_SUCCESS )
//        printf( "Volts read from A/D channel %d = %f\n", CAL_CHANNEL, volts[ CAL_CHANNEL ] );
//    else
//        printf( "Error '%s' reading A/D channel %d\n",
//                AIOUSB_GetResultCodeAsString( result ),
//                CAL_CHANNEL );


/*
 * MUST call AIOUSB_Exit() before program exits,
 * but only if AIOUSB_Init() succeeded
 */
 out_after_init:
    AIOUSB_Exit();

 out_main:

    return ( int ) result;
}
