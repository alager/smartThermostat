/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WifiMulti.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <AsyncElegantOTA.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <Streaming.h>

#include "myThermostat.h"

#include "main.h"


// global variables
MyThermostat *someTherm;

// network credentials
const char* ssid = "NestRouter1";
const char* password = "This_isapassword9";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// create a web socket object
AsyncWebSocket webSock("/ws");


/////////////////////////////
// code start

void notifyClients( std::string data )
{
	webSock.textAll( (char *)data.c_str() );
}


// a message from the websocket has arrived
// so process it.
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
	std::string replyStr;

	Serial.println( F( "Got Data from WebSocket" ));
	data[len] = 0;
	Serial.println( (char *)data );

	AwsFrameInfo *info = (AwsFrameInfo *)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		data[len] = 0;
		if (strcmp((char *)data, "temperatureUp") == 0)
		{
			float theTemp = someTherm->getTemperatureSetting();
			if( theTemp == 0 || theTemp == -1 || isnan( theTemp ) )
				theTemp = 65.0f;

			theTemp += 0.5f;
			someTherm->setTemperatureSetting( theTemp );
			
			// Serial << F( "UP theTemp: " ) << theTemp << mendl;

			replyStr = std::to_string( theTemp );
			// send the new temperature setting to the websocket clients
			notifyClients( "{\"tempSet\":" + replyStr + "}" );
		}
		else
		if (strcmp((char *)data, "temperatureDown") == 0)
		{
			float theTemp = someTherm->getTemperatureSetting();
			if( theTemp == 0 || theTemp == -1 || isnan( theTemp ) )
				theTemp = 65.0f;

			theTemp -= 0.5f;
			someTherm->setTemperatureSetting( theTemp );

			replyStr = std::to_string( theTemp );
			// send the new temperature setting to the websocket clients
			notifyClients( "{\"tempSet\":" + replyStr + "}" );
		}
		else
		if (strcmp((char *)data, "modeClick") == 0)
		{
			mode_e mode = someTherm->getMode();

			// cycle through the modes of operation
			if( mode == MODE_HEATING )
				mode = MODE_OFF;
			else
				mode = (mode_e)(mode + 1);

			// prep for a mode change
			preModeChange();

			someTherm->setMode( mode );

			// send the new temperature setting to the websocket clients
			replyStr = std::to_string( mode );
			notifyClients( "{\"modeSet\":" + replyStr + "}" );

			sendTelemetry();
		}
		else  
		if (strcmp((char *)data, "sendSettings") == 0)
		{
			// send the settings to the websockets
			std::string settingsStr;

			// StaticJsonObject allocates memory on the stack
			StaticJsonDocument<200> doc;
			JsonObject settings  =	doc.createNestedObject("settings");
			settings[ "fanDelay" ] = 			 someTherm->settings_getFanDelay();
			settings[ "compressorOffDelay" ]  =	 someTherm->settings_getCompressorOffDelay();
			settings[ "compressorMaxRuntime" ] = someTherm->settings_getCompressorMaxRuntime();
			settings[ "timeZone" ] =			 someTherm->timeZone_get();
		
			//serializeJsonPretty( doc, Serial );

			// put it into a buffer to send to the clients
			serializeJson( doc, settingsStr );

			// send it to the clients
			notifyClients( settingsStr );
		}
		else
		{
			Serial << ( F("trying it as JSON")) << mendl;

			// try to parse as JSON
			StaticJsonDocument<200> json;
			DeserializationError err = deserializeJson(json, data);
			if (err)
			{
				Serial << (F("deserializeJson() failed with code ")) << err.c_str() << mendl;
				return;
			}

			JsonObject fan = json["fanClick"];
			
			if( !fan.isNull() )
			{
				std::string JSONRetStr;

				bool off = fan[ "off" ];
				if( off )
				{
					// we got an off command, so clear the fan timer
					// then notify the UI to update the fan
					Serial << "FAN: Off" << mendl;

					// turn off the fan in a safe way in case heat/cooling is running
					someTherm->turnOffFan();

					Serial << "fanTime: " << someTherm->getFanRunTime() << mendl;

					
					// put it into a buffer to send to the clients
					serializeJson( json, JSONRetStr );

					// send it to the clients
					notifyClients( JSONRetStr );
					sendTelemetry();
					return;
				}

				bool add15minutes = fan[ "add15minutes" ];
				// serializeJsonPretty( add15minutes, Serial );
				Serial << "add15minutes: " << add15minutes << mendl;

				if( add15minutes )
				{
					// we got an add command, so add to the fan timer (and turn it on if applicable)
					// then notify the UI to update the fan
					Serial << "FAN: add 15 minutes" << mendl;
					
					// only turn on the fan if it is off right now
					if( MODE_OFF == someTherm->currentState() )
					{
						someTherm->turnOnFan();
						someTherm->setFanRunTime( (60 * 15) + someTherm->getFanRunTime() );

						Serial << "fanTime: " << someTherm->getFanRunTime() << mendl;

						// put it into a buffer to send to the clients
						serializeJson( json, JSONRetStr );

						// send it to the clients
						notifyClients( JSONRetStr );
					}
					sendTelemetry();
					return;
				}
			}
			
			JsonObject settings = json["settings"];
			if( !settings.isNull() )
			 {
				
				someTherm->settings_setFanDelay( settings[ "fanDelay" ] );
				someTherm->settings_setCompressorOffDelay( settings[ "compressorOffDelay" ] );
				someTherm->settings_setCompressorMaxRuntime( settings[ "compressorMaxRuntime" ] );
				
				uint16_t newTz = settings[ "timeZone" ];

				// store the new timezone value
				someTherm->timeZone_set( (timezone_e)newTz );

				// debug console output
				Serial << F("json value: ") << newTz << mendl;
				// Serial << F("Setting new time zone to: ") << someTherm->mySched.timeZone[ someTherm->mySched.tz ].c_str() << mendl;

				// the timezone possible just changed, so update the ezTime object
				// someTherm->mySched.myTZ.setLocation( someTherm->mySched.timeZone[ someTherm->mySched.tz ].c_str() );

				// myTZ.setLocation( F("America/Chicago") );
				// waitForSync();

				Serial << F("Setting new time zone DONE") << mendl;
				return;
			 }
		}
	}
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
			 void *arg, uint8_t *data, size_t len)
{
	switch (type)
	{
	case WS_EVT_CONNECT:
		Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
		sendTelemetry();
		break;
	case WS_EVT_DISCONNECT:
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		break;
	case WS_EVT_DATA:
		handleWebSocketMessage(arg, data, len);
		break;
	case WS_EVT_PONG:
	case WS_EVT_ERROR:
		break;
	}
}

void initWebSocket()
{
	webSock.onEvent(onEvent);
	server.addHandler(&webSock);
}


void setup()
{
    // Serial port for debugging purposes
    Serial.begin(115200);

	// MyThermostat someThermObj;
	someTherm = new MyThermostat;

	// someTherm = &someThermObj;
	someTherm->init();

	// debug, set the mode to cooling
	#ifdef _DEBUG_
	  Serial << ( F( "isMode: " )) << someTherm->getMode() << mendl;
	#endif
	
	// Initialize SPIFFS
	if(!LittleFS.begin())
	{
		Serial << ( F( "An Error has occurred while mounting LittleFS" )) << mendl;
		return;
	}

	// Connect to Wi-Fi
	WiFi.begin(ssid, password);
	#ifdef _DEBUG_
	  Serial << (F("Connecting to WiFi")) << mendl;
	#endif
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		#ifdef _DEBUG_
		  Serial.println(".");
		#endif
	}

	// Print ESP8266 Local IP Address
	Serial << (WiFi.localIP()) << mendl;

	Serial << "chip ID: 0x";
	Serial << ( ESP.getChipId(), HEX) << mendl;

	String therm1 = "therm1";
	String therm2 = "therm2";
	String therm9 = "therm9";

	// Now that WiFi is connected start mDNS
	if( WiFi.status() == WL_CONNECTED ) 
	{
		Serial << (F("MDNS started: " ));

		// Start mDNS with name esp8266
		if( 0x4864FE == ESP.getChipId() )
		{
			// Start mDNS with name esp8266
			if( MDNS.begin( therm9 ) )
			{ 
				Serial << ( therm9 ) << mendl;
			}
		}
		else
		if( 0x4852E3 == ESP.getChipId() )
		{
			// Start mDNS with name esp8266
			if( MDNS.begin( therm2 ) )
			{ 
				Serial << ( therm2 ) << mendl;
			}
		}
		else
		if( 0x48409D == ESP.getChipId() )
		{
			// Start mDNS with name esp8266
			if( MDNS.begin( therm1 ) )
			{ 
				Serial << ( therm1 ) << mendl;
			}
		}

		// add this for mDNS to respond
		MDNS.addService("http", "TCP", 80);	

		// after the network is up, we can init the scheduler
		// it needs networking for NTP first
		someTherm->sched_init();

		// configure web server routes
		configureRoutes();

		// configure web server web socket
		initWebSocket();

		// Start Elegant OTA
		AsyncElegantOTA.begin(&server);
		// AsyncElegantOTA.begin(&server, "username", "password");
		
		// Start server
		server.begin();
	}
	else
	{
		Serial << ( F( "WiFi Failed to connect" )) << mendl;
		
	}


}
 
void loop()
{	
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= interval)
	{
		// update counter for the next interval
		previousMillis = currentMillis;

		// update our current readings
		someTherm->updateMeasurements();

		// run thermostat background logic and timers
		someTherm->runSlowTick();

		// update websocket data pipe
		sendTelemetry();

		// run the ezTime task
		// this will poll pool.ntp.org about every 30 minutes
		someTherm->loopTick();
		
		// thermostat logic
		if( someTherm->isMode( MODE_COOLING ) )
		{
			if( someTherm->getTemperature_f() > ( someTherm->getTemperatureSetting() + someTherm->getTempHysteresis() ) )
			{
				// turn on the cooler (if we can)
				if( someTherm->turnOnCooler() )
				{
					sendCurrentMode();
					sendDelayStatus( false );
				}

				// allow the extended fan run time happen again
				someTherm->clearFanRunOnce();
			}
			else
			if( someTherm->getTemperature_f() <= someTherm->getTemperatureSetting() - someTherm->getTempHysteresis() )
			{
				// turn off the cooler, but run fan for a little longer
				someTherm->turnOffCooler();
				sendCurrentMode();
				sendDelayStatus( false );
			}

		}
		else
		if( someTherm->isMode( MODE_HEATING ) )
		{
			if( someTherm->getTemperature_f() < ( someTherm->getTemperatureSetting() - someTherm->getTempHysteresis() ) )
			{
				// turn on the heater (if we can)
				if( someTherm->turnOnHeater() )
				{
					sendCurrentMode();
					sendDelayStatus( false );
				}

				// allow the extended fan run time happen again
				someTherm->clearFanRunOnce();
			}
			else
			if( someTherm->getTemperature_f() >= someTherm->getTemperatureSetting() + someTherm->getTempHysteresis() )
			{
				// turn off the heater, but run fan for a little longer
				someTherm->turnOffHeater();
				sendCurrentMode();
				sendDelayStatus( false );
			}
		}
		else
		{
			// off
			someTherm->turnOffAll();
			sendDelayStatus( false );

		}

		// only check to update the eeprom once per loop
		// the eeprom will only write to the flash if the 
		// datastructure cache has been changed
		someTherm->saveSettings();

		// clean up dangling websockets
		 webSock.cleanupClients();

	} // end of 10s loop

	// run the mDNS processor loop
	MDNS.update();
}


void configureRoutes( void )
{
	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send( LittleFS, F("/index.html"), F("text/html") );
	});

	// Route to load style.css file
	server.on("/mario.css", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/mario.css"), F("text/css") );
	});

	// sen the sprites
	server.on("/Mario_3_sprites.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/Mario_3_sprites.jpg"), F("image/jpeg") );
	});
	server.on("/thermSprites.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/thermSprites.jpg"), F("image/jpeg") );
	});

	// send the fonts
	server.on("/marioFont.woff", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/marioFont.woff"), F("font/woff") );
	});

	// deal with the favicon.ico
	server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/favicon.ico"), F("image/x-image") );
	});

	// long press javascript
	server.on("/longClick.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, F("/longClick.js"), F("text/javascript") );
	});

	// deal with not found ( 404 )
	server.onNotFound( []( AsyncWebServerRequest *request )
	{
		request->send(404, "text/plain", F( "404: Not found") );
	});
}


void sendTelemetry( void )
{
	std::string telemetryStr;

	// StaticJsonObject allocates memory on the stack
	StaticJsonDocument<200> doc;

	JsonObject telemetry  =			doc.createNestedObject("telemetry");
	telemetry[ "mode" ] =			someTherm->getMode();
	telemetry[ "tempSetting" ] =	someTherm->getTemperatureSetting();
	telemetry[ "currentMode" ] =	someTherm->currentState();
	telemetry[ "tempAvg" ] =		someTherm->getTemperature_f();
	telemetry[ "humidAvg" ] =		someTherm->getHumidity_f();
	telemetry[ "presAvg" ] =		someTherm->getPressure_f();
	telemetry[ "time" ] =			someTherm->timeZone_getTimeStr();
	telemetry[ "delayTime" ] =		someTherm->getCompressorOffTime();
	
	if( MODE_OFF == someTherm->currentState() )
		telemetry[ "fanTime" ] =		someTherm->getFanRunTime();
	else
		telemetry[ "fanTime" ] = 0;

	// Generate the prettified JSON and send it to the Serial port.
	// serializeJsonPretty(doc, Serial);

	// put it into a buffer to send to the clients
	serializeJson( doc, telemetryStr );

	// send it to the clients
	notifyClients( telemetryStr );
}


// set the IO and additional items for a mode change
void preModeChange( void )
{
	// turn on the delay blinker.  
	// If going to off mode, then it'll set it to false on its own
	sendDelayStatus( true );

	// turn off all IO
	someTherm->turnOffAll();
}


// let the web socket know the delay status
// use simple preformated json string
void sendDelayStatus( bool status )
{
	if( status )
		notifyClients( "{\"delay\":true}" );
	else
		notifyClients( "{\"delay\":false}" );
}


void sendCurrentMode( void )
{
	std::string currentState;
	currentState = "{\"currentMode\":";
	currentState +=  std::to_string( someTherm->currentState() );
	currentState += "}";

	// Serial.println( currentState.c_str() );

	notifyClients( currentState );
}