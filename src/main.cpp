/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <AsyncElegantOTA.h>

#include "myThermostat.h"

// local prototypes
void sendTelemetry( void );
void preModeChange( void );


// variables
MyThermostat *someTherm;
MyThermostat someThermObj;

// network credentials
const char* ssid = "NestRouter1";
const char* password = "This_isapassword9";

// current temperature & humidity, updated in loop()
// float t = 0.0;
// float h = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// create a web socket object
AsyncWebSocket ws("/ws");

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates readings every 10 seconds
const unsigned long interval = 10000;

#define SEALEVELPRESSURE_HPA (1013.25f)

/////////////////////////////
// code start
// bool ledState = 0;

void notifyClients( std::string data )
{
	ws.textAll( (char *)data.c_str() );
}


// a message from the websocket has arrived
// so process it.
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
	std::string replyStr;

	Serial.println( "Got Data from WebSocket");
	data[len] = 0;
	Serial.println( (char *)data );

	AwsFrameInfo *info = (AwsFrameInfo *)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		data[len] = 0;
		if (strcmp((char *)data, "temperatureUp") == 0)
		{
			float theTemp = someTherm->getTemperatureSetting();
			theTemp += 0.5f;
			someTherm->setTemperatureSetting( theTemp );
			
			// Serial.print( "UP theTemp: " );
			// Serial.println( theTemp );

			replyStr = to_string( theTemp );
			// send the new temperature setting to the websocket clients
			notifyClients( "tempSet:" + replyStr );
		}
		else
		if (strcmp((char *)data, "temperatureDown") == 0)
		{
			float theTemp = someTherm->getTemperatureSetting();
			theTemp -= 0.5f;
			someTherm->setTemperatureSetting( theTemp );

			replyStr = to_string( theTemp );
			// send the new temperature setting to the websocket clients
			notifyClients( "tempSet:" + replyStr );
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
			replyStr = to_string( mode );
			notifyClients( "modeSet:" + replyStr );
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
	ws.onEvent(onEvent);
	server.addHandler(&ws);
}


void setup()
{
    // Serial port for debugging purposes
    Serial.begin(115200);

	// MyThermostat someThermObj;
	someTherm = &someThermObj;
	someTherm->init();

	// debug, set the mode to cooling
	Serial.print( "isMode: " );
	Serial.println( someTherm->getMode() );

	
	// Initialize SPIFFS
	if(!LittleFS.begin())
	{
		Serial.println("An Error has occurred while mounting SPIFFS");
		return;
	}

	// Connect to Wi-Fi
	WiFi.begin(ssid, password);
	Serial.println("Connecting to WiFi");
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.println(".");
	}

	// Print ESP8266 Local IP Address
	Serial.println(WiFi.localIP());

	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		// request->send(LittleFS, "/index.html", String(), false, processor);
		request->send( LittleFS, "/index.html", "text/html" );
	});

	// server.on("/page2.html", HTTP_GET, [](AsyncWebServerRequest *request)
	// {
	// 	request->send(LittleFS, "/page2.html", String(), false, processor);
	// });

	// Route to load style.css file
	server.on("/mario.css", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, "/mario.css", "text/css");
	});

	// sen the sprites
	server.on("/Mario_3_sprites.jpg", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, "/Mario_3_sprites.jpg", "image/jpeg");
	});

	// send the fonts
	server.on("/marioFont.woff", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(LittleFS, "/marioFont.woff", "font/woff");
	});

	initWebSocket();

	// Start ElegantOTA
	AsyncElegantOTA.begin(&server);
	// AsyncElegantOTA.begin(&server, "username", "password");
	
	// Start server
	server.begin();
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
		someTherm->runTick();

		// update websocket data pipe
		sendTelemetry();
		
		// thermostat logic
		if( someTherm->isMode( MODE_COOLING ) )
		{
			if( someTherm->getTemperature_f() > someTherm->getTemperatureSetting() )
			{
				// turn on the cooler (if we can)
				if( someTherm->turnOnCooler() )
				{
					notifyClients( "currentMode:" + to_string( someTherm->currentState() ) );
					notifyClients( "delay:false" );
				}
				// else
				// {
				// 	notifyClients( "delay:true" );
				// }

				// allow the extended fan run time happen again
				someTherm->clearFanRunOnce();
			}
			else
			if( someTherm->getTemperature_f() <= someTherm->getTemperatureSetting() )
			{
				// turn off the cooler, but run fan for a little longer
				someTherm->turnOffCooler();
				notifyClients( "currentMode:" + to_string( someTherm->currentState() ) );
				notifyClients( "delay:false" );
			}

		}
		else
		if( someTherm->isMode( MODE_HEATING ) )
		{
			if( someTherm->getTemperature_f() < someTherm->getTemperatureSetting() )
			{
				// turn on the heater (if we can)
				if( someTherm->turnOnHeater() )
				{
					notifyClients( "currentMode:" + to_string( someTherm->currentState() ) );
					notifyClients( "delay:false" );
				}
				// else
				// {
				// 	notifyClients( "delay:true" );
				// }

				// allow the extended fan run time happen again
				someTherm->clearFanRunOnce();
			}
			else
			if( someTherm->getTemperature_f() >= someTherm->getTemperatureSetting() )
			{
				// turn off the heater, but run fan for a little longer
				someTherm->turnOffHeater();
				notifyClients( "currentMode:" + to_string( someTherm->currentState() ) );
				notifyClients( "delay:false" );
			}
		}
		else
		{
			// off
			someTherm->turnOffAll();
			notifyClients( "delay:false" );

		}

		// only check to update the eeprom once per loop
		// the eeprom will only write to the flash if the 
		// datastructure cache has been changed
		someTherm->saveSettings();
		// EEPROM.commit();
	} // end of 10s loop
}


void sendTelemetry( void )
{
	std::string telemetry;

	// build a telemetry string
	telemetry = "telemetry:";
	telemetry += to_string( someTherm->getMode() ) + ",";
	telemetry += to_string( someTherm->getTemperatureSetting() ) + ",";
	telemetry += to_string( someTherm->currentState() ) + ",";
	telemetry += someTherm->getTemperature() + ",";
	telemetry += someTherm->getHumidity() + ",";
	telemetry += someTherm->getPressure();

	// send it to the clients
	notifyClients( telemetry );
}


// set the IO and additional items for a mode change
void preModeChange( void )
{
	// turn on the delay blinker.  
	// If going to off mode, then it'll set it to false on its own
	notifyClients( "delay:true" );

	// turn off all IO
	someTherm->turnOffAll();
}