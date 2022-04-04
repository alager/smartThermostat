#ifndef __SCHEDULER_H__
 #define __SCHEDULER_H__

#include "Arduino.h"
#include <string>
#include <ezTime.h>

// simplified list of time zones
typedef enum
{
	none,
	ePST,
	eMST,
	eCST,
	eEST
} __attribute__((packed)) timezone_e;

typedef enum
{
	AM,
	PM
} __attribute__((packed)) ampm_e;

typedef struct
{
	uint8_t		hour;
	uint8_t		minute;
	ampm_e		ampm;
	float		temperature;
} __attribute__((packed)) setTime_t;


typedef struct
{
	setTime_t setting[ 4 ];
} __attribute__((packed)) sched_t;

typedef sched_t schedAry_t[ 2 ];


typedef struct 
{
	bool	newValue;
	float	temp;
} newTemperature_t;


typedef enum
{
	eOff,
	eCool,
	eHeat
} SchedMode_e;

class Scheduler
{
	public:
		Scheduler();

		newTemperature_t tick( SchedMode_e mode );
		void init( timezone_e tz );
		void loadSchedule( schedAry_t *sched );


		std::string		timeZoneStr [5];		// array of strings that holds the time zone options
		Timezone		myTZ;					// create a time & timezone object
		timezone_e 		tz;						// track the currently selected time zone

		// the array is 8 because 0 is not used due to the 
		// pre-defined day values from ezTime.h
		schedAry_t  *schedule;

	private:
};

 #endif