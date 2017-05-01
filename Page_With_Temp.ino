
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

// GPIO, куда подцелено реле
uint8_t PowerPin = 5;
bool    PowerOn  = false;
bool    StateSleep  = false;
bool    FullCharge  = false;
bool    StopCharge  = false;

IPAddress ip(192,168,8,101);  
IPAddress gateway(192,168,8,1);
IPAddress subnet(255,255,255,0);

MDNSResponder mdns;

ESP8266WebServer server (80);


byte addr[8]; 
float voltage;

float getVoltage()
{
    float AOvar = (unsigned int) analogRead(A0);
    AOvar = AOvar * 3383;
    AOvar = AOvar / 1024;
    AOvar = AOvar / 1000;
    return AOvar;
}



void handleRoot() 
{

	char temp[400];
 
  voltage = getVoltage();
  const char *tmpSign2 = (voltage < 0) ? "-" : "";
  float tmpVal2 = (voltage < 0) ? -voltage : voltage;

  int tmpInt12 = tmpVal2;                  // Get the integer (678).
  float tmpFrac2 = tmpVal2 - tmpInt12;     // Get fraction (0.0123).
  int tmpInt22 = trunc(tmpFrac2 * 100);    // Turn into integer (123).

  
  Serial.println(voltage);
	snprintf ( temp, 400,

  "<html>\
  <head>\
    <meta charset='UTF-8' >\
    <meta http-equiv='refresh' content='5'/>\
    <title>Вольтметр</title>\
    <style>\
      body { background-color: #dfdfdf; font-family: Arial, Helvetica, Sans-Serif; Color: #777777; }\
    </style>\
  </head>\
  <body>\
    <center><h1>Напряжение АКБ</h1></center>\
    <center><p> %s%d.%02d вольт </p></center>\
  </body>\
  </html>", tmpSign2, tmpInt12, tmpInt22
	);
  
	server.send ( 200, "text/html", temp );


 //gpio.mode(1, gpio.OUTPUT) --RELAY

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

void handleRelay() 
{
  bool stat = false;

  if( server.hasArg("stat") ){
     if( strncmp(server.arg("stat").c_str(),"1",1) == 0 )stat = true;
  }
  else {
     stat = PowerOn;
  }
  
  String out = "";

  out =
"<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <title>WiFi реле</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>WiFi реле. Версия 1.0</h1>\n";

  if( stat ){
      out+="\
    <h2>Состояние: Включено</br>\
    <a href=\"/relay?stat=0\">Выключить</a></h2>\
    ";
  }
  else {
      out+="\
    <h2>Состояние: Выключено</br>\
    <a href=\"/relay?stat=1\">Включить</a><h2>\
    ";            
  }
   out+= "\
  </body>\
</html>";
   server.send ( 200, "text/html", out );
   if( stat != PowerOn ){
      PowerOn = stat;
      digitalWrite(PowerPin , PowerOn);
      if( PowerOn )Serial.println("Power is ON");
      else Serial.println("Power is OFF");
   }
}



void handleSleep() 
{
  bool statsleep = false;

  if( server.hasArg("statsleep") ){
     if( strncmp(server.arg("statsleep").c_str(),"1",1) == 0 )statsleep = true;
  }
  else {
     statsleep = StateSleep;
  }
  
  String outsleep = "";

  outsleep =
"<html>\
  <head>\
    <meta charset=\"utf-8\" />\
    <title>SLEEP STATE</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>СОН</h1>\n";

  if( statsleep ){
      outsleep+="\
    <h2>Состояние: Включено</br>\
    <a href=\"/sleep?statsleep=0\">Выключить</a></h2>\
    ";
  }
  else {
      outsleep+="\
    <h2>Состояние: Выключено</br>\
    <a href=\"/sleep?statsleep=1\">Включить</a><h2>\
    ";            
  }
   outsleep+= "\
  </body>\
</html>";
   server.send ( 200, "text/html", outsleep );
   if( statsleep != StateSleep ){
      StateSleep = statsleep;
   }
}



void setup ( void ) 
{
    //relay
    PowerOn = false;
    pinMode(PowerPin, OUTPUT); 
    digitalWrite(PowerPin, PowerOn);
    //digitalWrite(5, HIGH);

    pinMode(A0, INPUT);
	  Serial.begin ( 115200 );
	  WiFi.begin ( ssid, password );
    WiFi.config(ip, gateway, subnet);
	  Serial.println ( "" );

	  // Wait for connection
	  while ( WiFi.status() != WL_CONNECTED ) 
	  {
		  delay ( 100 );
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
    server.on ( "/relay", handleRelay );
    server.on ( "/sleep", handleSleep );
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

  if(!StateSleep)
  {

     //один раз в 5 секунд
     if(millis()-currentMillis > 5000)
      {
        voltage = getVoltage();
        if(voltage > 3.30)
        {
          PowerOn = true;
          digitalWrite(PowerPin , PowerOn); 
          FullCharge  = true;    
        }
        else
        {
          FullCharge  = false; 
        }

        if(voltage > 3.50)
        {
          StopCharge =  true;
        }
        else
        {
           StopCharge =  false;
        }

        
        currentMillis = millis();
      }
      
      mdns.update();
      server.handleClient();
  
  }
  else
  {
    Serial.println();
    Serial.println("closing connection");
    Serial.println("ESP8266 in sleep mode");
    ESP.deepSleep(sleepTimeS * 1000000, WAKE_RF_DEFAULT);
  }
    
}



