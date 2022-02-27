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
} timezone_e;

class Scheduler
{
	public:
		Scheduler();

		void tick( void );
		void init( timezone_e tz );

		std::string timeZone [5];

		// create a time & timezone object
		Timezone myTZ;

	private:
		timezone_e tz;
};

 #endif