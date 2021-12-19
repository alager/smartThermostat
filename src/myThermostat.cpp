#include <Arduino.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <string>
using namespace std;


#include "myThermostat.h"

uint8_t BME280_i2caddr = 0x76;

// create the bme object for I2C (SPI takes parameters)
Adafruit_BME280 bme; // I2C



// create the initial data structures
// and connect to the BME280
MyThermostat::MyThermostat( )
{
	// read in the eeprom settings
	eepromInit();
}

MyThermostat::MyThermostat( mode_e mode )
{
	int addr = 0;
	// read in the eeprom settings
	eepromInit();

	eepromData.mode = mode;
	EEPROM.put( addr, eepromData );
}

// destructor does nothing right now
MyThermostat::~MyThermostat( )
{}



void MyThermostat::init()
{
	// default settings
	safeToRunCompressor = true;

	unsigned status;
    status = bme.begin( BME280_i2caddr );  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }
    
	bme.setSampling(
		Adafruit_BME280::MODE_FORCED,
		Adafruit_BME280::SAMPLING_X1, // temperature
		Adafruit_BME280::SAMPLING_X1, // pressure
		Adafruit_BME280::SAMPLING_X1, // humidity
		Adafruit_BME280::FILTER_OFF,
		Adafruit_BME280::STANDBY_MS_1000
		);
	// suggested rate is 1/60Hz (1m)

    Serial.println("\n\n-- BME280 connected on address 0x76 --");

	// configure GPIO
	Serial.println("\n\n-- Configuring GPIO --");
	pinMode( GPIO_FAN, OUTPUT );
	pinMode( GPIO_COOLING, OUTPUT );
	pinMode( GPIO_HEATING, OUTPUT );
	pinMode( GPIO_EMGHEAT, OUTPUT );

	// test GPIO
	digitalWrite( GPIO_FAN, HIGH );
	digitalWrite( GPIO_COOLING, HIGH );
	digitalWrite( GPIO_HEATING, HIGH );
	digitalWrite( GPIO_EMGHEAT, HIGH );
	delay(500);
	digitalWrite( GPIO_FAN, LOW );
	digitalWrite( GPIO_COOLING, LOW );
	digitalWrite( GPIO_HEATING, LOW );
	digitalWrite( GPIO_EMGHEAT, LOW );
	delay(500);
	digitalWrite( GPIO_FAN, HIGH );
	digitalWrite( GPIO_COOLING, HIGH );
	digitalWrite( GPIO_HEATING, HIGH );
	digitalWrite( GPIO_EMGHEAT, HIGH );
	delay(500);
	digitalWrite( GPIO_FAN, LOW );
	digitalWrite( GPIO_COOLING, LOW );
	digitalWrite( GPIO_HEATING, LOW );
	digitalWrite( GPIO_EMGHEAT, LOW );
	delay(500);
	digitalWrite( GPIO_FAN, HIGH );
	digitalWrite( GPIO_COOLING, HIGH );
	digitalWrite( GPIO_HEATING, HIGH );
	digitalWrite( GPIO_EMGHEAT, HIGH );
	delay(500);
	digitalWrite( GPIO_FAN, LOW );
	digitalWrite( GPIO_COOLING, LOW );
	digitalWrite( GPIO_HEATING, LOW );
	digitalWrite( GPIO_EMGHEAT, LOW );
}


float MyThermostat::getTempRaw()
{
	float temp = bme.readTemperature();
	return temp * 1.8 + 32;
}


float MyThermostat::getTemperature_f() 
{
	static bool firstRun = true;
	float temperature;
	static float outputTemp;
	static float a = 16.0f;

	// read the temperature sensor
	temperature = bme.readTemperature();

	// convert to Fahrenheit
	temperature = 1.8f * temperature + 32.0f;

	// check for invalid value and put in -1 so 
	// the user can see something is wrong
	if (isnan( temperature )) 
	{
		temperature = -1.0f;
	}

	// // pre-load the array on the first run
	if( firstRun )
	{
		outputTemp = temperature;
		firstRun = false;
	}


	// low pass filter
	outputTemp += ( temperature - outputTemp ) / a;
	
	Serial.print("TempAvg:	");
	Serial.println( outputTemp );
	return outputTemp;
}


// get the temperature and convert it to a string
std::string MyThermostat::getTemperature()
{
	return to_string( getTemperature_f() );
}


// Returns the pressure from the sensor (in Pascal)
float MyThermostat::getPressure_f()
{
	float pressure;

	// read the temperature sensor and convert to in-Hg
	pressure = bme.readPressure() / 3386.3886666667f;

	Serial.print("Pressure:	");
	Serial.println( pressure );
	return pressure;
}


// return a string of the pressure
std::string MyThermostat::getPressure()
{
	return to_string( getPressure_f() );
}


float MyThermostat::getHumidity_f() 
{
	static bool firstRun = true;
	static float humidArry[ 4 ] = {0,0,0,0};
	static unsigned char idx = 0;
	float humidity;

	humidity = bme.readHumidity();

	// check for invalid value and put in -1 so 
	// the user can see something is wrong
	if (isnan( humidity )) 
	{
		humidity = -1.0f;
	}

	// store and average this reading in with the rest
	humidArry[ idx ] = humidity;
	idx++;
	idx &= 0x03;

	if( firstRun )
	{
		humidArry[3] = humidArry[2] = humidArry[1] = humidArry[0];
		firstRun = false;
	}

	// reuse the temp variable, and get the average
	humidity = 0;
	for( unsigned char loop = 0; loop < 4; loop++ )
	{
		humidity += humidArry[ loop ];
	}
	humidity /= 4.0f;

	Serial.print( "Humidity:	");
	Serial.println( humidity );
	return humidity;
}


string MyThermostat::getHumidity()
{
	return to_string( getHumidity_f() );
}


// check what mode we are in
bool MyThermostat::isMode( mode_e mode )
{
	if( mode == eepromData.mode )
		return true;
	else
		return false;
}


// set the mode setting
// usually based upon a user input
void MyThermostat::setMode( mode_e mode )
{
	int addr = 0;
	eepromData.mode = mode;
	EEPROM.put( addr, eepromData );
}


// return the mode
mode_e MyThermostat::getMode( void )
{
	return eepromData.mode;
}


// get the readings, which triggers reading the sensors, 
// but don't bother to return the values
void MyThermostat::updateMeasurements( void )
{
	bme.takeForcedMeasurement();
	getTemperature_f();
	getHumidity_f();
}


// run background logic and timers
void MyThermostat::runTick( void )
{
	// decrement or clear the run once flag, so the fan can be set to run again
	decrementFanRunTime();

	if( getFanRunTime() == 0 )
		clearFanRunOnce();


	// decrement and 
	decrementCompressorOffTime();

	if( getCompressorOffTime() == 0 )
		setSafeToRunCompressor( true );
}


// read the non-volatile temperature setting for the mode we are in
float MyThermostat::getTemperatureSetting( void )
{
	if( isMode( MODE_COOLING ) )
	{
		return eepromData.coolTemp;
	}
	else
	if( isMode( MODE_HEATING ) )
	{
		return eepromData.hotTemp;
	}
	else
	{
		return -66.6f;
	}
}


// set the non-volatile temperature setting for the mode we are in
void MyThermostat::setTemperatureSetting( float newTemp )
{
	int addr = 0;
	
	if( isMode( MODE_COOLING ) )
	{
		eepromData.coolTemp = newTemp;
		Serial.println( "isMode: cooling" );
	}
	else
	if( isMode( MODE_HEATING ) )
	{
		eepromData.hotTemp = newTemp;
		Serial.println( "isMode: heating" );
	}
	else
	if( isMode( MODE_OFF ) )
	{
		Serial.println( "isMode: off" );
	}
	else
	{
		Serial.println( "isMode: unknown" );
	}

	EEPROM.put( addr, eepromData );
}


// set the fan run time in seconds (10 seconds minimum)
// set to 0 to turn off
void MyThermostat::setFanRunTime( unsigned long time )
{
	fanRunTime = time / 10;
}


// returns the remaining fan run time in seconds
unsigned long MyThermostat::getFanRunTime( void )
{
	return fanRunTime * 10;
}


void MyThermostat::decrementFanRunTime( void )
{
	if( fanRunTime )
		fanRunTime--;
}


// turn off the cooler but set the fan to run for a while
void MyThermostat::turnOffCooler( void )
{
	if( !fanRunOnce )
	{
		fanRunOnce = true;
		setFanRunTime( FIVE_MINUTES );
		setCompressorOffTime( FIVE_MINUTES );
	}

	// turn off the compressor
	digitalWrite( GPIO_COOLING, LOW );


	// turn off the fan after a delay
	if( getFanRunTime() == 0 )
	{
		digitalWrite( GPIO_FAN, LOW );
	}
		
}


void MyThermostat::turnOnCooler( void )
{
	if( isSafeToRunCompressor() )
	{
		// set the flag to prevent short cycling
		setSafeToRunCompressor( false );

		// set the mode to cooling
		setMode( MODE_COOLING );

		// the active state is cooling turn on the fan and compressor
		digitalWrite( GPIO_FAN, HIGH );
		digitalWrite( GPIO_COOLING, HIGH );
	}
}


void MyThermostat::turnOffHeater( void )
{
	if( !fanRunOnce )
	{
		fanRunOnce = true;
		setFanRunTime( FIVE_MINUTES );
		setCompressorOffTime( FIVE_MINUTES );
	}

	// turn off the compressor
	digitalWrite( GPIO_HEATING, LOW );


	// turn off the fan after a delay
	if( getFanRunTime() == 0 )
	{
		digitalWrite( GPIO_FAN, LOW );
	}
}


void MyThermostat::turnOnHeater( void )
{
	if( isSafeToRunCompressor() )
	{
		// set the flag to prevent short cycling
		setSafeToRunCompressor( false );

		// set the mode to cooling
		setMode( MODE_HEATING );

		// the active state is cooling turn on the fan and compressor
		digitalWrite( GPIO_FAN, HIGH );
		digitalWrite( GPIO_HEATING, HIGH );
	}
}


void MyThermostat::turnOffAll( void )
{
	setMode( MODE_OFF );

	// turn off the fan
	digitalWrite( GPIO_FAN, LOW );

	// turn off the heating & cooling
	digitalWrite( GPIO_HEATING, LOW );
	digitalWrite( GPIO_COOLING, LOW );

	// set the flag to prevent short cycling
	setSafeToRunCompressor( false );
	setCompressorOffTime( FIVE_MINUTES );

}


void MyThermostat::clearFanRunOnce( void )
{
	fanRunOnce = false;
}


bool MyThermostat::isSafeToRunCompressor( void )
{
	return safeToRunCompressor;
}


void MyThermostat::setSafeToRunCompressor( bool safe )
{
	safeToRunCompressor = safe;
}


void MyThermostat::decrementCompressorOffTime( void )
{
	if( compressorOffTime )
		compressorOffTime--;
}


// returns the remaining fan run time in seconds
unsigned long MyThermostat::getCompressorOffTime( void )
{
	return compressorOffTime * 10;
}


// set the fan run time in seconds (10 seconds minimum)
// set to 0 to turn off
void MyThermostat::setCompressorOffTime( unsigned long time )
{
	compressorOffTime = time / 10;
}


// read in the eeprom
// if the cookie is invalid, then initialize the eeprom
void MyThermostat::eepromInit( void )
{
	// since we use a structure, the address is always 0
	int addr = 0;

	// initialize the eeprom datastructure
	// and allocate enough bytes for our structure
	EEPROM.begin( sizeof( eepromData ) );

	// read the bytes into our structure
	EEPROM.get( addr, eepromData );

	// check the cookie
	if( !eepromCookieIsValid() )
	{
		// write the initial eeprom values
		eepromWriteFirstValues();
	}

	// the put command writes local data back to 
	// the eeprom cache, but it isn't commited to flash yet 
	//  EEPROM.put( addr, eepromData );

	// actually write the content of byte-array cache to
	// hardware flash.  flash write occurs if and only if one or more byte
	// in byte-array cache has been changed, but if so, ALL sizeof(eepromData) bytes are 
	// written to flash
	// EEPROM.commit();  
}


// return true if the cookie is valid
// otherwise return false
bool MyThermostat::eepromCookieIsValid( void )
{
	if( MAGIC_COOKIE == eepromData.cookie )
		return true;
	else
		return false;
}


// write some sane values to the eeprom
void MyThermostat::eepromWriteFirstValues( void )
{
	int addr = 0;
	
	eepromData.cookie =		MAGIC_COOKIE;
	eepromData.coolTemp =	75.0f;
	eepromData.hotTemp =	66.5f;
	eepromData.mode =		MODE_OFF;

	EEPROM.put( addr, eepromData );
	EEPROM.commit();
}


// update the eeprom values to make any changes permanent
// Note: it only wirtes the flash if it has been changed.
void MyThermostat::saveSettings( void )
{
	EEPROM.commit();
}