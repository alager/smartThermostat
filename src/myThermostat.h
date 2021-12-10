
#ifndef __MYTHERMOSTAT_H__
 #define __MYTHERMOSTAT_H__

#include "Arduino.h"
#include <string>
using namespace std;

#define GPIO_FAN 		(2)
// #define GPIO_COOLING	(16)
#define GPIO_COOLING	(14)
#define GPIO_HEATING	(12)
#define GPIO_EMGHEAT	(13)

// 5 minutse in second
#define FIVE_MINUTES ( 300 )

typedef enum
{
	MODE_OFF,
	MODE_COOLING,
	MODE_HEATING
} mode_e;




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
		bool isMode( mode_e mode );
		void setMode( mode_e mode );
		void updateMeasurements( void );
		void runTick( void );

		float getTemperatureSetting( void );
		void setTemperatureSetting( float );

		void setFanRunTime( unsigned long );
		unsigned long getFanRunTime( void );
		void decrementFanRunTime( void );

		void turnOffCooler( void );
		void turnOnCooler( void );
		void clearFanRunOnce( void );

		void turnOffHeater( void );
		void turnOnHeater( void );

		void turnOffAll( void );

		bool isSafeToRunCompressor( void );
		void setSafeToRunCompressor( bool safe );
		void decrementCompressorOffTime( void );
		unsigned long getCompressorOffTime( void );
		void setCompressorOffTime( unsigned long );
		
	private:
		mode_e 			modeSetting;
		unsigned long 	fanRunTime;
		float 			heatingTemperature;
		float 			coolingTemperature;
		unsigned long	compressorOffTime;

		bool 			fanRunOnce;
		bool 			safeToRunCompressor;

};
#endif