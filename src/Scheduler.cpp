
#include "Arduino.h"
#include <string>
#include <ezTime.h>
#include <Streaming.h>

#include <Scheduler.h>

// constructor
Scheduler::Scheduler()
{
	// time zone string array
	// must be in the same order as timezone_e
	timeZoneStr[0] = std::string();	//empty string
	timeZoneStr[1] = "America/Los_Angeles";
	timeZoneStr[2] = "America/Denver";
	timeZoneStr[3] = "America/Chicago";
	timeZoneStr[4] = "America/New_York";
}


// tick is run
void Scheduler::tick( void )
{
	// run the ezTime task
	// this will poll pool.ntp.org about every 30 minutes
	events();
}



void Scheduler::init( timezone_e new_tz )
{
	this->tz = new_tz;

	// debug ezTime
	setDebug(INFO);

	Serial << (F( "Syncing NTP" ) ) << mendl;
	
	// wait for ezTime to sync
	waitForSync();

	// Provide official timezone names
	// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
	Serial <<  F("our Timezone: " ) << timeZoneStr[ tz ].c_str() << mendl;
	

	myTZ.setLocation( timeZoneStr[ tz ].c_str() );
	Serial << F("Central Time:     ") << myTZ.dateTime() << mendl;


}


void Scheduler::loadSchedule( sched_t *sched )
{

}
