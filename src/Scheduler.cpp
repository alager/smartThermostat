
#include "Arduino.h"
#include <string>
#include <ezTime.h>
#include <Streaming.h>

#include <Scheduler.h>


Scheduler::Scheduler()
{
	// time zone string array
	// must be in the same order as timezone_e
	timeZone[1] = "America/Los_Angeles";
	timeZone[2] = "America/Denver";
	timeZone[3] = "America/Chicago";
	timeZone[4] = "America/New_York";


	// debug ezTime
	setDebug(INFO);

	// wait for ezTime to sync
	Serial << (F( "Syncing with NTP" ) ) << mendl;
	waitForSync();


	// Provide official timezone names
	// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
	Serial <<  F("our Timezone: " ) << timeZone[ tz ].c_str() << mendl;
	

	// // myTZ.setLocation( "America/Chicago" );
	// // Serial << F("Central Time:     ") << mendl;
	// // Serial.println( myTZ.dateTime() );

}

void Scheduler::tick( void )
{
	// run the ezTime task
	// this will poll pool.ntp.org about every 30 minutes
	events();
}

void Scheduler::init( timezone_e tz )
{
	this->tz = tz;
}