Smart-ish thermostat based on ESP8266 and a BME280 temperature, humidity and pressure sensor.
The user interface is web based, and optimized for a sideways (landscape) cell phone experience.

## Screen Shots:
#### Cooling
![cooling](https://github.com/alager/smartThermostat/raw/main/ScreenShots/Cooling.PNG?raw=true)

#### Heating
![heating](https://github.com/alager/smartThermostat/raw/main/ScreenShots/Heating.PNG?raw=true)

<br>

TODO:
- optimize the sprite sheet to remove unused sprites
- add a few more mario sprites to allow "running" when heat or cool is active
- add schedules
- ~~add time zone user entry.~~ Added, but ezTime doesn't update, it needs a restart. delete & recreate the object causes WDT reset
