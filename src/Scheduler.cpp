
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
newTemperature_t Scheduler::tick( SchedMode_e mode )
{
	setTime_t storedTime;
	newTemperature_t newTemp = { .newValue = false, .temp = 0.0f };

	// run the ezTime task
	// this will poll pool.ntp.org about every 30 minutes
	events();

	// day of week, dow
	uint8_t dow = myTZ.weekday();	
	
	// off, don't search or update the temperature
	if( mode == eOff )
		return newTemp;

	// shift the mode down by 1, so that we don't have to waste 
	// ram for an Off schedule.  Only heat and cool are in the arrays
	mode = (SchedMode_e)(mode - 1);

	// check date & time against the stored schedule
	for( int idx = 0; idx < 4; idx++ )
	{
		storedTime.hour = schedule[dow][mode].setting[idx].hour;
		storedTime.minute = schedule[dow][mode].setting[idx].minute;
		storedTime.ampm = schedule[dow][mode].setting[idx].ampm;

		if( storedTime.hour > 0 )
		{
			
			Serial << "storedTime: " << storedTime.hour << ":" << storedTime.minute << ((storedTime.ampm == AM ) ? "AM" : "PM" ) << mendl;
			Serial << "time: " << myTZ.hourFormat12() << ":" << myTZ.minute() << ( (myTZ.isAM() ) ? "AM" : "PM" ) << mendl;

			if( myTZ.isAM() == storedTime.ampm )
			{
				if( myTZ.hourFormat12() == storedTime.hour )
				{
					if( myTZ.minute() == storedTime.minute )
					{
						// we have a match!
						Serial << "We have a match!" << mendl;
						newTemp.newValue = true;
						newTemp.temp =  schedule[dow][mode].setting[idx].temperature;
						break;
					}
				}
			}
		}
	}

	// if there is a match then indicate that the temperature should be updated
	return newTemp;
}



void Scheduler::init( timezone_e new_tz )
{
	this->tz = new_tz;

	// debug ezTime
	// setDebug(INFO);

	Serial << (F( "Syncing NTP" ) ) << mendl;
	
	// wait for ezTime to sync
	waitForSync();

	// Provide official timezone names
	// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
	Serial <<  F("Timezone: " ) << timeZoneStr[ tz ].c_str() << mendl;
	Serial <<  myTZ.dateTime() << mendl;

	myTZ.setLocation( timeZoneStr[ tz ].c_str() );
	myTZ.setDefault();
}


// assume control over the schedule data
void Scheduler::loadSchedule( schedAry_t *sched )
{
	this->schedule = sched;
}


void Scheduler::loadFanSched( fanTimeAry_t *fanSched )
{
	this->fanTime = fanSched;
}