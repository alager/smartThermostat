
#ifndef __MYTHERMOSTAT_H__
 #define __MYTHERMOSTAT_H__

#include "Arduino.h"
#include <string>
#include <EEPROM.h>
using namespace std;

#define GPIO_FAN 		(2)
// #define GPIO_COOLING	(16)
#define GPIO_COOLING	(14)
#define GPIO_HEATING	(12)
#define GPIO_EMGHEAT	(13)

// 5 minutse in second
#define FIVE_MINUTES ( 30 )

typedef enum
{
	MODE_OFF,
	MODE_COOLING,
	MODE_HEATING
} mode_e;

// simplified list of time zones
typedef enum
{
	ePST,
	eMST,
	eCST,
	eEST
} timezone_e;

#define MAGIC_COOKIE	( 0xdebb1e04 )
typedef struct 
{
	unsigned long	cookie;			// magic cookie for versioning
	float			coolTemp;		// the cool temperature setting
	float			hotTemp;		// the hot temperature setting
	mode_e			mode;			// the last mode

	unsigned short	fanDelay;				// number of seconds the fan runs after the compressor turns off
	unsigned short	compressorOffDelay;		// how long the compressor must stay off once turned off
	unsigned short	compressorMaxRuntime;	// how long the compressor can run before being forced off

	timezone_e		localTimeZone;			// what time zone we are in
} myEEprom_t;



class MyThermostat
{
	public:
		MyThermostat();
		MyThermostat( mode_e mode );
		~MyThermostat();
		void init();
		float getTempRaw();
		float getTemperature_f();
		string getTemperature();
		float getHumidity_f();
		string getHumidity();
		float getPressure_f();
		string getPressure();

		bool isMode( mode_e mode );
		void setMode( mode_e mode );
		mode_e getMode( void );
		void updateMeasurements( void );
		void runTick( void );

		float getTemperatureSetting( void );
		void setTemperatureSetting( float );

		void setFanRunTime( unsigned long );
		unsigned long getFanRunTime( void );
		void decrementFanRunTime( void );

		void turnOffCooler( void );
		bool turnOnCooler( void );
		void clearFanRunOnce( void );

		void turnOffHeater( void );
		bool turnOnHeater( void );

		mode_e currentState( void );
		void turnOffAll( void );

		bool isSafeToRunCompressor( void );
		void setSafeToRunCompressor( bool safe );
		void decrementCompressorOffTime( void );
		unsigned long getCompressorOffTime( void );
		void setCompressorOffTime( unsigned long );

		void eepromInit( void );
		bool eepromCookieIsValid( void );
		void eepromWriteFirstValues( void );
		void saveSettings( void );

		unsigned short settings_getFanDelay( void );
		unsigned short settings_getCompressorOffDelay( void );
		unsigned short settings_getCompressorMaxRuntime( void );
		void settings_setFanDelay( unsigned short );
		void settings_setCompressorOffDelay( unsigned short );
		void settings_setCompressorMaxRuntime( unsigned short );

		void timeZone_set( timezone_e tz );
		timezone_e timeZone_get( void );

		std::string timeZone [4];
		
	private:
		mode_e 			currentMode;
		unsigned long 	fanRunTime;
		unsigned long	compressorOffTime;

		bool 			fanRunOnce;
		bool 			safeToRunCompressor;

		myEEprom_t		eepromData;


		int digitalReadOutputPin(uint8_t pin);

};
#endif