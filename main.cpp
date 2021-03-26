#include "mbed.h"
#include "http_request.h"
#include "OLEDDisplay.h"
#include <time.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <ctime>
#include "glibr.h"

OLEDDisplay oled( MBED_CONF_IOTKIT_OLED_RST, MBED_CONF_IOTKIT_OLED_SDA, MBED_CONF_IOTKIT_OLED_SCL );
SPI spi( MBED_CONF_IOTKIT_LED_SPI_MOSI, NC, MBED_CONF_IOTKIT_LED_SPI_SCLK );

char host[] = "http://api.thingspeak.com/update";
char key[] = "2UA773WW6HBQ3PZ5";
unsigned int strip[9];
char message[1024];
char* color;
char* slideSide;
int gLED, rLED, bLED = 0;

void updateOLED();
void updateHost();
void clearLED();
void writeLED();

WiFiInterface* network;

glibr GSensor( D14, D15 );

int main()
{
    if ( GSensor.ginit() ) {
        printf("APDS-9960 initialization complete\n\r");
    } else {
        printf("Something went wrong during APDS-9960 init\n\r");
    }
 
    // Start running the APDS-9960 gesture sensor engine
    if ( GSensor.enableGestureSensor(true) ) {
        printf("Gesture sensor is now running\n\r");
    } else {
        printf("Something went wrong during gesture sensor init!\n\r");
    }

    // Connect to the network with the default networking interface
    network = WiFiInterface::get_default_instance();
    if (!network) {
        printf("\r\nNo WiFi-Interface found.\r\n");
        return -1;
    }

    printf("\r\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    int ret = network->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("Connection error: %d\r\n", ret);
        return -1;
    }

    SocketAddress a;
    network->get_ip_address(&a);
    printf("Success! - MAC: %s, IP: %s\r\n", network->get_mac_address(), a.get_ip_address());

    printf("\r\nHost: %s, Key: %s\r\n", host, key);

    //Fetch time
    HttpRequest* request = new HttpRequest(network, HTTP_GET, "http://worldtimeapi.org/api/ip");
    HttpResponse* response = request->send();

    if(!response){
        printf("\r\nFailed to fetch time.\r\n");
    }
    else{
        std::string body = response->get_body_as_string();
        std:string::size_type i = body.find("datetime\":\"");

        if(i != std::string::npos){
            body.erase(0, i + 11); //Erase string up to datetime

            i = body.find("\"");

            if(i != std::string::npos){
                body.erase(i, body.length() - i); //Erase string after datetime

                //Set device time to parsed datetime
                struct tm datetime;
                std::istringstream ss(body.c_str());
                ss >> std::get_time(&datetime, "%Y-%m-%dT%H:%M:%S");
                time_t curtime = mktime(&datetime);
                printf("\r\nFetched time: %s\r\n", ctime(&curtime));
                set_time(curtime);
            }
        }
    }
    delete request;

    //Get, display and send data
    printf("\r\nData:\r\n");

    int timestamp1 = std::time(0) * 1000;
    int timestamp2 = std::time(0) * 1000;
    updateOLED();

    while(1)
    {
        if(timestamp1 + 15500 < std::time(0) * 1000){
            timestamp1 = std::time(0) * 1000;
            updateHost();
        }
        if(timestamp2 + 950 < std::time(0) * 1000){
            timestamp2 = std::time(0) * 1000;
            updateOLED();
        }
        thread_sleep_for(30);
        if ( GSensor.isGestureAvailable() )
         {
            //oled.printf( "Gesture sensor " );
            oled.cursor( 0, 0 );
            switch ( GSensor.readGesture() )
            {
                case DIR_UP:
                    slideSide = "UP";
                    color = "Rot";
                    rLED = 32;
                    gLED = 0;
                    bLED = 0;
                    break;
                case DIR_DOWN: 
                    slideSide = "DOWN";
                    color = "Gruen";
                    rLED = 0;
                    gLED = 32;
                    bLED = 0;
                    break;
                case DIR_LEFT:
                    slideSide = "LEFT";
                    color = "Blau";
                    rLED = 0;
                    gLED = 0;
                    bLED = 32;
                    break;
                case DIR_RIGHT:
                    slideSide = "RIGHT";
                    color = "Cyan";
                    rLED = 0;
                    gLED = 32;
                    bLED = 32;
                    break;
                case DIR_NEAR:
                    slideSide = "NEAR";
                    color = "Gelb";
                    rLED = 32;
                    gLED = 32;
                    bLED = 0;
                    break;
                case DIR_FAR:
                    slideSide = "FAR";
                    color = "Magenta";
                    rLED = 32;
                    gLED = 0;
                    bLED = 32;
                    break;
                default:
                    slideSide = "NONE";
                    color = "Weiss";
                    rLED = 32;
                    gLED = 32;
                    bLED = 32;
                    break;
            }
        }
        thread_sleep_for( 10 );
		// Gruen, Rot, Blau - von Dunkel bis Hell
		for ( int i = 0; i < 128; i+=32 )
		{
				// LED 1
				strip[0] = rLED;
				strip[1] = gLED;
				strip[2] = bLED;
				// LED 2
				strip[3] = rLED;
				strip[4] = gLED;
				strip[5] = bLED;
				// LED 3
				strip[6] = rLED;
				strip[7] = gLED;
				strip[8] = bLED;
				writeLED();
				thread_sleep_for( 100 );
		}
		thread_sleep_for( 1000 );
		clearLED();
    }
}

void updateOLED(){

    oled.cursor( 0, 0 );
    auto time = std::time(nullptr);
    char timeString[10];
    std::strftime(timeString, sizeof(timeString), "%T", std::localtime(&time));

    printf("Time: %s, Color: %s, Slide: %s\r\n", (char*) &timeString, color, slideSide);

    oled.clear();
    thread_sleep_for(30);
    oled.cursor(0, 0);
    thread_sleep_for(30);
    oled.printf("Time:   %s\ncolor:  %s\nslide:  %s", (char*) &timeString, color, slideSide);
}

void updateHost(){
    sprintf(message, "%s?key=%s&field1=%s&field2=%s", host, key, color, slideSide);
    
    HttpRequest* request = new HttpRequest(network, HTTP_POST, message);
    HttpResponse* response = request->send();
    if (!response)
    {
        printf("HttpPostRequest failed (error code %d)\n", request->get_error());
    }
    delete request;
}

void writeLED()
{
	for ( int p = 0; p < 9; p++ )
		spi.write( strip[p] );
}

void clearLED()
{
	for ( int p = 0; p < 9; p++ )
	{
		strip[p] = 0;
		spi.write( strip[p] );
	}
}
