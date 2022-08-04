#ifndef __MAIN_H__
 #define __MAIN_H__

// The debug symbol controls the mDNS name and other serial outputs
#define _DEBUG_

// Generally, you should use "unsigned long" 32 bits, for variables that hold time
// The value will quickly become too large for an int to store
uint32_t previousMillis = 0;    // will store last time DHT was updated

// Updates readings every 10 seconds
const uint32_t interval = 10000;

#define SEALEVELPRESSURE_HPA (1013.25f)

#define MAC_OUTSIDE		( 0x4864FE )
#define MAC_UPSTAIRS	( 0x4852E3 )
#define MAC_DOWNSTAIRS	( 0x48409D )


// main.cpp prototypes
void sendTelemetry( void );
void preModeChange( void );
void sendDelayStatus( bool status );
void sendCurrentMode( void );
void configureRoutes( void );

void startWiFi( void );
void startMDNS( void );
void wakeupCB( void );


#endif