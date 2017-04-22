
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

ESP8266WebServer server ( 80 );


byte addr[8]; 
float temperature;




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


const int led = 13;



void handleRoot() 
{
	digitalWrite ( led, 1 );
	char temp[400];
  temperature = getTemp();
  Serial.println(temperature);
  int tepm= temperature;
	snprintf ( temp, 400,

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
    <center><h1>Температура в комнате.</h1></center>\
    <center><p> %d градуса </p></center>\
  </body>\
  </html>",tepm
	);
  
	server.send ( 200, "text/html", temp );
	digitalWrite ( led, 0 );
}



void handleNotFound() 
{
	  digitalWrite ( led, 1 );
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
	  digitalWrite ( led, 0 );
}



void setup ( void ) 
{
	  pinMode ( led, OUTPUT );
	  digitalWrite ( led, 0 );
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
	  server.on ( "/test.svg", drawGraph );
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




void drawGraph() 
{
	  String out = "";
	  char temp[100];
	  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
 	  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
 	  out += "<g stroke=\"black\">\n";
 	  int y = rand() % 130;
  
  	for (int x = 10; x < 390; x+= 10) 
 	  {
 		  int y2 = rand() % 130;
 		  sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
 		  out += temp;
 		  y = y2;
 	  }
  
	  out += "</g>\n</svg>\n";

	  server.send ( 200, "image/svg+xml", out);
}
