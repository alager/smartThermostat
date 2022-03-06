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

typedef struct
{
	uint32_t	time;
	float		heatTemp;
	float		coolTemp;
} __attribute__((packed)) setTime_t;

typedef struct
{
	setTime_t setTime[ 4 ];
} __attribute__((packed)) sched_t;



class Scheduler
{
	public:
		Scheduler();

		void tick( void );
		void init( timezone_e tz );
		void loadSchedule( sched_t *sched );


		std::string		timeZoneStr [5];		// array of strings that holds the time zone options
		Timezone		myTZ;					// create a time & timezone object
		timezone_e 		tz;						// track the currently selected time zone

		// the array is 8 because 0 is not used due to the 
		// pre-defined day values from ezTime.h
		sched_t  *schedule[ 8 ];

	private:
};

 #endif