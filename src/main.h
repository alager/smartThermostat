#ifndef __MAIN_H__
 #define __MAIN_H__


// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates readings every 10 seconds
const unsigned long interval = 10000;

#define SEALEVELPRESSURE_HPA (1013.25f)


// main.cpp prototypes
void sendTelemetry( void );
void preModeChange( void );
void sendDelayStatus( bool status );
void sendCurrentMode( void );
void configureRoutes( void );


#endif