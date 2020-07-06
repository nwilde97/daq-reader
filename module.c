#include <node_api.h>
#include <aiousb.h>

int NUM_CHANNELS = 64;

unsigned long setupDAQ() {
    unsigned char gainCodes[ NUM_CHANNELS ];
    unsigned long result;
    unsigned long deviceMask;
    int MAX_NAME_SIZE = 20;
    char name[ MAX_NAME_SIZE + 2 ];
    unsigned long productID, nameSize, numDIOBytes, numCounters;
    unsigned long deviceIndex = 0;
    unsigned long deviceFound = 0;
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

napi_value shutdownDAQ(napi_env env, napi_callback_info info) {
	AIOUSB_Exit();
	return NULL;
}

napi_value setupDAQWrapper(napi_env env, napi_callback_info info){
	napi_value result;
	napi_create_int32(env, setupDAQ(), &result);
	return result;
}

napi_value scanChannels(napi_env env, napi_callback_info info){
	double volts[ NUM_CHANNELS ];
	unsigned int deviceIndex;
	size_t argc = 1;
	napi_value argv[1];
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	napi_get_value_uint32(env, argv[0], &deviceIndex);
//	printf( "Querying Channels for device %u\n",deviceIndex);
	ADC_GetScanV( deviceIndex, volts );
	napi_value result;
	napi_create_array(env, &result);

	napi_value num_result;
	for (int i = 0; i < NUM_CHANNELS; ++i) {
		napi_create_double(env, volts[i], &num_result);
		napi_set_element(env, result, i, num_result);
	}

	return result;
}

napi_value Init(napi_env env, napi_value exports) {
    napi_value fn;
    napi_create_function(env, NULL, 0, setupDAQWrapper, NULL, &fn);
    napi_set_named_property(env, exports, "setupDAQ", fn);

    napi_create_function(env, NULL, 0, shutdownDAQ, NULL, &fn);
    napi_set_named_property(env, exports, "shutdownDAQ", fn);

    napi_create_function(env, NULL, 0, scanChannels, NULL, &fn);
    napi_set_named_property(env, exports, "scanChannels", fn);
  	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)