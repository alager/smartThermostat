/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/
#define DEBUG_ESP_WIFI
#define DEBUG_ESP_PORT
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
#include <WebSocketsClient.h>

#include "myThermostat.h"

#include "main.h"


// global variables
MyThermostat *someTherm;

// network credentials
const char* ssid = "NestRouter1";
const char* password = "This_isapassword9";

// MDNS names
String therm1 = "therm1";
String therm2 = "therm2";
String therm9 = "therm9";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// create a web socket object
AsyncWebSocket webSock("/ws");


// for the websocket client
WebSocketsClient webSocketClient;

ESP8266WiFiMulti wifiMulti;

/////////////////////////////
// code start

// websocket server
void notifyClients( std::string data )
{
	webSock.textAll( (char *)data.c_str() );
}


// a message from the websocket has arrived
// so process it.
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
	std::string replyStr;

	Serial << ( F( "Got Data from WebSocket" ));
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

			// serializeJsonPretty( json, Serial );

			JsonObject auxClick = json[ "auxClick" ];
			serializeJsonPretty( auxClick, Serial );
			if( !auxClick.isNull() )
			{
				std::string JSONRetStr;

				bool state = auxClick[ "state" ];

				if( state == true )
				{
					// add 10 minutes to the aux heater
					someTherm->turnOnAuxHeater( 60 * 10 );
					Serial << "state: true" << mendl;
				}
				else
				if( state == false )
				{
					someTherm->turnOffAuxHeater();
					Serial << "state: false" << mendl;

				}

				bool add15minutes = auxClick[ "add15minutes" ];
				Serial << "add15minutes: " << add15minutes << mendl;

				// put it into a buffer to send to the clients
				serializeJson( json, JSONRetStr );

				// send it to the clients
				notifyClients( JSONRetStr );
				sendTelemetry();
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
					Serial << F( "FAN: Off" ) << mendl;

					// turn off the fan in a safe way in case heat/cooling is running
					someTherm->turnOffFan();

					Serial << F( "fanTime: " ) << someTherm->getFanRunTime() << mendl;

					
					// put it into a buffer to send to the clients
					serializeJson( json, JSONRetStr );

					// send it to the clients
					notifyClients( JSONRetStr );
					sendTelemetry();
					return;
				}

				bool add15minutes = fan[ "add15minutes" ];
				// serializeJsonPretty( add15minutes, Serial );
				// Serial << "add15minutes: " << add15minutes << mendl;

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

						// Serial << "fanTime: " << someTherm->getFanRunTime() << mendl;

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

				// Serial << F("Setting new time zone DONE") << mendl;
				return;
			 }
		}
	}
}

// websocket server events
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
			 void *arg, uint8_t *data, size_t len)
{
	switch (type)
	{
	case WS_EVT_CONNECT:
		Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
		
		if( MAC_OUTSIDE == ESP.getChipId() )
		{
			// wsConnected = true;
		}

		sendTelemetry();
		break;
	case WS_EVT_DISCONNECT:
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		if( MAC_OUTSIDE == ESP.getChipId() )
		{
			// wsConnected = false;
		}
		break;
	case WS_EVT_DATA:
		handleWebSocketMessage(arg, data, len);
		break;
	case WS_EVT_PONG:
	case WS_EVT_ERROR:
		break;
	}
}

// websocket server
void initWebSocket()
{
	// add a function pointer for the "on" event handler
	webSock.onEvent( onEvent );
	server.addHandler(&webSock);
}

// init to true for first time through the loop
bool connected = true;
uint16_t discoCount = 0;
#define DEBUG_SERIAL ( Serial )
// websocket client events
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
    switch(type) {
        case WStype_DISCONNECTED:
            DEBUG_SERIAL.printf("[WSc] Disconnected!\n");
            connected = false;
			discoCount++;
            break;
        case WStype_CONNECTED: {
            DEBUG_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
            connected = true;
 			discoCount = 0;
            // send message to server when Connected
            // DEBUG_SERIAL.println("[WSc] SENT: Connected");
            // webSocketClient.sendTXT("Connected");
        }
            break;
        case WStype_TEXT:
            DEBUG_SERIAL.printf("[WSc] RESPONSE: %s\n", payload);
            break;
        case WStype_BIN:
            DEBUG_SERIAL.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);
            break;
                case WStype_PING:
                        // pong will be send automatically
                        DEBUG_SERIAL.printf("[WSc] get ping\n");
                        break;
                case WStype_PONG:
                        // answer to a ping we send
                        DEBUG_SERIAL.printf("[WSc] get pong\n");
                        break;

		default:
			break;
    }
 
}

void setup()
{
    // Serial port for debugging purposes
    Serial.begin(115200);

	// MyThermostat someThermObj;
	someTherm = new MyThermostat;

	// init the object for first run
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
	startWiFi();

	// Now that WiFi is connected start mDNS
	if( wifiMulti.run() == WL_CONNECTED )
	{
		if( MAC_OUTSIDE != ESP.getChipId() )
		{
			// start MDNS if not the outside therm
			startMDNS();
		}
		
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

		// deal with CORS access
		DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
		DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "content-type");

		// Start web server
		server.begin();
	}
	else
	{
		Serial << ( F( "WiFi Failed to connect" )) << mendl;
	}
}

// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 15000;

// configure and connect to the local wifi
void startWiFi( void )
{
	// Set in station mode
 	WiFi.mode( WIFI_STA );
	
	// Register multi WiFi networks
	wifiMulti.addAP( ssid, password );

	Serial << ( F("Connecting to WiFi 1") ) << mendl;
	if( WL_CONNECTED == wifiMulti.run(connectTimeoutMs) )
	{
		Serial << ( F( "Connected") ) << mendl;
	}
	else
	{
		Serial << ( F("Connecting to WiFi 2") ) << mendl;
		while( wifiMulti.run() != WL_CONNECTED )
		{
			Serial << wifiMulti.run();
		}
		Serial << wifiMulti.run() << mendl;
	}

	// stay on the same wifi
	WiFi.setAutoReconnect(true);
	WiFi.persistent(true);

	// Print ESP8266 Local IP Address
	Serial << ( WiFi.localIP() ) << mendl;

	Serial << "chip ID: 0x";
	Serial.println( ESP.getChipId(), HEX); // this won't print correctly using the stdout notation
}


void startMDNS( void )
{
	if( MAC_OUTSIDE == ESP.getChipId() )
	{
		// Start mDNS with name esp8266
		if( MDNS.begin( therm9 ) )
		{ 
			Serial << ( therm9 ) << mendl;
		}
	}
	else
	if( MAC_UPSTAIRS == ESP.getChipId() )
	{
		// Start mDNS with name esp8266
		if( MDNS.begin( therm2 ) )
		{ 
			Serial << ( therm2 ) << mendl;
		}
	}
	else
	if( MAC_DOWNSTAIRS == ESP.getChipId() )
	{
		// Start mDNS with name esp8266
		if( MDNS.begin( therm1 ) )
		{ 
			Serial << ( therm1 ) << mendl;
		}
	}

	// add this for mDNS to respond
	MDNS.addService("http", "TCP", 80);
	Serial << (F("MDNS started: " ));
}


// mandatory call back function for using the sleep API
void wakeupCB( void )
{
	Serial << F( "wake up call back" ) << mendl;
	Serial.flush();
}

// put the device to sleep
// sleep_time is how long to sleep in seconds
void startSleep( uint32_t sleep_time )
{
	Serial << F( "Going to SLEEP" ) << mendl;
	Serial.flush();

	// convert from seconds to milliseconds
	sleep_time *= 1000;

	// this device only reports temperature, so we sleep a bunch instead
	// this also reduces internal cabinet temperature
	//wifi_station_disconnect(); //not needed
	
	// for timer-based light sleep to work, the os timers need to be disconnected
	extern os_timer_t *timer_list;
	timer_list = nullptr;

	wifi_set_opmode( NULL_MODE );
	wifi_fpm_set_sleep_type( LIGHT_SLEEP_T );
	wifi_fpm_open();
	wifi_fpm_set_wakeup_cb( wakeupCB );	// mandatory callback function

	/////////////// SLEEP BEGIN ///////////////
	delay( 10 );
	// fpm sleep time in micro seconds
	wifi_fpm_do_sleep( sleep_time * 1000 );

	// we must call delay for the actual LIGHT_SLEEP to happen
	// timed light sleep is only entered when the sleep command is
	// followed by a delay() that is at least 1ms longer than the sleep
	delay( sleep_time + 10 );
	/////////////// SLEEP END ///////////////
}

// websocket client
void makeWSConnection( void )
{
	Serial << "ws client opening connection" << mendl;
	// server address, port and URL
    webSocketClient.begin("therm.home", 3000, "/ws");
 
    // event handler
    webSocketClient.onEvent(webSocketEvent);
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

		// check wifi status
		if( (WiFi.status() != WL_CONNECTED) )
		{
				// it's been 10s and we haven't connected, so shut wifi down
				// so we can start afresh
				WiFi.disconnect();
				
				// start the wifi again
				startWiFi();
		}
		
		if( MAC_OUTSIDE == ESP.getChipId() )
		{
			// *** this is the outside device ***
			// 1	Go to sleep
			// 2	wake up
			// 3	get on wifi
			// 4	make websocket connection to ws://therm.home:3000/ws
			// 5	send telemetry
			// 6	goto 1

			if( connected || discoCount > 10 )
			{
				discoCount = 0;
				//close websocket client
				Serial << "ws client closing connection" << mendl;
				webSocketClient.disconnect();

				// shut wifi down before going to sleep
				WiFi.disconnect();

				// This will put the device to sleep
				// execution will continue from here when it wakes up
				startSleep( SLEEP_TIME );

				// we are awake again
				// start the wifi again
				startWiFi();

				// prevent sleep mode from happening until next time
				wifi_set_sleep_type(NONE_SLEEP_T);
				delay( 1 );

				makeWSConnection();
			}
		}
		else
		if( someTherm->isMode( MODE_COOLING ) )
		{
			// thermostat logic
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

	// keep the wifi happy
	wifiMulti.run();

	if( MAC_OUTSIDE != ESP.getChipId() )
		// run the mDNS processor loop if not the outside therm
		MDNS.update();

	if( MAC_OUTSIDE == ESP.getChipId() )
		webSocketClient.loop();
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

	if( MAC_OUTSIDE == ESP.getChipId() )
	{
		Serial << "Sending WS data to server" << mendl;
		// send it to the server
		webSocketClient.sendTXT( telemetryStr.c_str() );	
	}
	else
	{
		// send it to the clients
		notifyClients( telemetryStr );
	}

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