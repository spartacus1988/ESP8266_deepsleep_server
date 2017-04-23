
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <Auth.h>
OneWire  ds(2); 


//this code in Auth.h
//const char *ssid = "SSID";
//const char *password = "PASSWORD";

//10 секунд глубокий сон
const int sleepTimeS = 10;

//20 секунд штатной работы сервера
long loopTime = 20000;  

MDNSResponder mdns;

ESP8266WebServer server (80);


byte addr[8]; 
float temperature;
float voltage;

float getVoltage()
{
    float AOvar = (unsigned int) analogRead(A0);
    AOvar = AOvar * 3130;
    AOvar = AOvar / 1024;
    AOvar = AOvar / 1000;
    return AOvar;
}




float getTemp()
{
    byte data[12];  
    if (!ds.search(addr)) 
    {
      Serial.println("No more addresses."); 
      while(1);
    }
    ds.reset_search(); 
    if (OneWire::crc8(addr, 7) != addr[7]) 
    {
      Serial.println("CRC is not valid!");
      while(1);
    }
    ds.reset();            
    ds.select(addr);        
    ds.write(0x44);      
    delay(1000);   
    ds.reset();
    ds.select(addr);    
    ds.write(0xBE);          
    for (int i = 0; i < 9; i++) 
    {           
      data[i] = ds.read();  
    }
    int raw = (data[1] << 8) | data[0]; 
    if (data[7] == 0x10) raw = (raw & 0xFFF0) + 12 - data[6];  
    return raw / 16.0;   
} 






void handleRoot() 
{

	char temp[800];
  temperature = getTemp();

  //float adc_read = 678.0123;
  const char *tmpSign = (temperature < 0) ? "-" : "";
  float tmpVal = (temperature < 0) ? -temperature : temperature;

  int tmpInt1 = tmpVal;                  // Get the integer (678).
  float tmpFrac = tmpVal - tmpInt1;      // Get fraction (0.0123).
  int tmpInt2 = trunc(tmpFrac * 100);    // Turn into integer (123).
  
  voltage = getVoltage();
  const char *tmpSign2 = (voltage < 0) ? "-" : "";
  float tmpVal2 = (voltage < 0) ? -voltage : voltage;

  int tmpInt12 = tmpVal2;                  // Get the integer (678).
  float tmpFrac2 = tmpVal2 - tmpInt12;     // Get fraction (0.0123).
  int tmpInt22 = trunc(tmpFrac2 * 100);    // Turn into integer (123).

  
  Serial.println(temperature);
  Serial.println(voltage);
  //int tepm= temperature;
	snprintf ( temp, 800,

  "<html>\
  <head>\
    <meta charset='UTF-8' >\
    <meta http-equiv='refresh' content='5'/>\
    <title>Термометр</title>\
    <style>\
      body { background-color: #dfdfdf; font-family: Arial, Helvetica, Sans-Serif; Color: #777777; }\
    </style>\
  </head>\
  <body>\
    <center><h1>Температура датчика</h1></center>\
    <center><p> %s%d.%02d градуса </p></center>\
    <center><h2>Напряжение АКБ</h2></center>\
    <center><p> %s%d.%02d вольт </p></center>\
  </body>\
  </html>",tmpSign, tmpInt1, tmpInt2, tmpSign2, tmpInt12, tmpInt22
	);
  
	server.send ( 200, "text/html", temp );

}



void handleNotFound() 
{

	  String message = "File Not Found\n\n";
	  message += "URI: ";
	  message += server.uri();
	  message += "\nMethod: ";
	  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	  message += "\nArguments: ";
	  message += server.args();
	  message += "\n";

	  for ( uint8_t i = 0; i < server.args(); i++ ) 
	  {
		  message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	  }

	  server.send ( 404, "text/plain", message );

}



void setup ( void ) 
{

    pinMode(A0, INPUT);
	  Serial.begin ( 115200 );
	  WiFi.begin ( ssid, password );
	  Serial.println ( "" );

	  // Wait for connection
	  while ( WiFi.status() != WL_CONNECTED ) 
	  {
		  delay ( 500 );
		  Serial.print ( "." );
	  }

	  Serial.println ( "" );
	  Serial.print ( "Connected to " );
	  Serial.println ( ssid );
	  Serial.print ( "IP address: " );
	  Serial.println ( WiFi.localIP() );

	  if ( mdns.begin ( "esp8266", WiFi.localIP() ) ) 
	  {
		  Serial.println ( "MDNS responder started" );
	  }

	  server.on ( "/", handleRoot );
	  server.on ( "/inline", []() 
	  {
		  server.send ( 200, "text/plain", "this works as well" );
	  } );
    
	  server.onNotFound ( handleNotFound );
	  server.begin();
	  Serial.println ( "HTTP server started" );
}




void loop ( void ) 
{
    unsigned long currentMillis = millis();
 
    while(millis()-currentMillis<=loopTime)
    {
      mdns.update();
      server.handleClient();
    }
  
    //delay(10000);
    Serial.println();
    Serial.println("closing connection");
    Serial.println("ESP8266 in sleep mode");
    ESP.deepSleep(sleepTimeS * 1000000, WAKE_RF_DEFAULT);
    
}



