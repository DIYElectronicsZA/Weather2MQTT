// Weather2MQTT
// Ramb0t 2021 
// DIYElectronics.co.za
// ESP8266 / Wemos D1 mini Arduino project to 
// Read data from DFRobot SEN0186 weather station over software serial 
// and publish to MQTT server over wifi 
// Based off DFRobot sample code and ESP8266 MQTT examples

#include <Arduino.h>
#include <PubSubClient.h> // Connect and publish to the MQTT broker
#include <ESP8266WiFi.h>  // Enables the ESP8266 to connect to the local network (via WiFi)
#include <SoftwareSerial.h> // Software serial port for reading weather data, since the onbaord uart is used for USB / PC uploads
//Config Includes (remeber to rename 'config_sample.h' to 'config.h'
#include "config.h"

#define PUBLISH_PERIOD  60000 // ms value of the MQTT publish period 

// MQTT Defines 
const char* humidity_topic      = "home/outside/humidity";
const char* temperature_topic   = "home/outside/temperature";
const char* pressure_topic      = "home/outside/pressure";
const char* windspdavg1m_topic  = "home/outside/windspdavg1m";
const char* windspdmax5m_topic  = "home/outside/windspdmax5m";
const char* winddirection_topic = "home/outside/winddirection";
const char* rain1hr_topic       = "home/outside/rain1hr";
const char* rain24hr_topic      = "home/outside/rain24hr";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient); 
// Soft Serial - Use pin D2 for Rx of data. Need this as hardware UART is used on WEMOS for USB/Serial
SoftwareSerial ss(D2, D3); 


// Globals
long oldtime;
bool fdataValid; 
char                 databuffer[38];
double               temp;


// Custom function to connet to the MQTT broker via WiFi
void connect_MQTT(){
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, pass);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.print(WiFi.status()); 
  }

  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}

//Get weather status data
// Read weather data from software serial port
void getBuffer()                                                                    
{
  int index;
  for (index = 0;index < 38;index ++){
    if(ss.available()){
      databuffer[index] = (char)ss.read();
      if (databuffer[0] != 'c'){
        index = -1;
      }
    }else{
      index --;
    }
  }
}

//char to int
int transCharToInt(char *_buffer,int _start,int _stop)                               
{
  int _index;
  int result = 0;
  int num = _stop - _start + 1;
  int _temp[num];
  for (_index = _start;_index <= _stop;_index ++)
  {
    _temp[_index - _start] = _buffer[_index] - '0';
    result = 10*result + _temp[_index - _start];
  }
  return result;
}

//Wind Direction
int WindDirection()                                                                  
{
  int itemp = transCharToInt(databuffer,1,3); 
  if (itemp <= 360 && itemp >= 0){ //sanity check
    return itemp; 
  }else{
    //error
    return -1; 
  }
}

//Air Speed (1 minute)
float WindSpeedAverage()                                                             
{
  temp = 0.44704 * transCharToInt(databuffer,5,7);
  if (temp>= 0 && temp <= 60){
    return temp;
  }else{
    // error
    return -1; 
  }
}

//Max air speed (5 minutes)
float WindSpeedMax()                                                                 
{
  temp = 0.44704 * transCharToInt(databuffer,9,11);
    if (temp>= 0 && temp <= 60){
    return temp;
  }else{
    // error
    return -1; 
  }
}

//Temperature ("C")
float Temperature()                                                                  
{
  temp = (transCharToInt(databuffer,13,15) - 32.00) * 5.00 / 9.00;
  if (temp>= -20 && temp <= 60){
    return temp;
  }else{
    // error
    return -100; 
  }
}

//Rainfall (1 hour)
float RainfallOneHour()                                                              
{
  temp = transCharToInt(databuffer,17,19) * 25.40 * 0.01;
  if (temp>= 0 && temp <= 200){
    return temp;
  }else{
    // error
    return -1; 
  }
}

//Rainfall (24 hours)
float RainfallOneDay()                                                               
{
  temp = transCharToInt(databuffer,21,23) * 25.40 * 0.01;
  if (temp>= 0 && temp <= 2000){
    return temp;
  }else{
    // error
    return -1; 
  }
}

//Humidity
int Humidity()                                                                       
{
  int itemp = transCharToInt(databuffer,25,26);
  if (itemp>= 0 && itemp <= 100){
    return itemp;
  }else{
    // error
    return -1; 
  }
  
}

//Barometric Pressure
float BarPressure()                                                                  
{
  temp = transCharToInt(databuffer,28,32);
  temp = temp / 100.00; 
  if (temp>= 50 && temp <= 150){
    return temp;
  }else{
    // error
    return -1; 
  }
}

// See if the weather data makes sense
bool checkValid(){
  if(Temperature() != -100 && Humidity() != -1 && BarPressure() != -1){
    if (WindDirection() != -1 && WindSpeedAverage() != -1 && WindSpeedMax() != -1){
      if(RainfallOneDay() != -1 && RainfallOneHour() != -1){
        return true; 
      }
    }
  }else{
    return false;
  }
}

// Print the weather data to serial port
void printWeather(){
  Serial.print("DataValid?: ");
  Serial.print(fdataValid);
  Serial.println("  ");
  Serial.print("Wind Direction: ");
  Serial.print(WindDirection());
  Serial.println("  ");
  Serial.print("Average Wind Speed (One Minute): ");
  Serial.print(WindSpeedAverage());
  Serial.println("m/s  ");
  Serial.print("Max Wind Speed (Five Minutes): ");
  Serial.print(WindSpeedMax());
  Serial.println("m/s");
  Serial.print("Rain Fall (One Hour): ");
  Serial.print(RainfallOneHour());
  Serial.println("mm  ");
  Serial.print("Rain Fall (24 Hour): ");
  Serial.print(RainfallOneDay());
  Serial.println("mm");
  Serial.print("Temperature: ");
  Serial.print(Temperature());
  Serial.println("C  ");
  Serial.print("Humidity: ");
  Serial.print(Humidity());
  Serial.println("%  ");
  Serial.print("Barometric Pressure: ");
  Serial.print(BarPressure());
  Serial.println("kPa");
  Serial.println("");
  Serial.println("");

}

void publishMQTT(){
  connect_MQTT();
    Serial.setTimeout(2000);

    // PUBLISH to the MQTT Broker (topic = Temperature, defined at the beginning)
    if (client.publish(temperature_topic, String(Temperature()).c_str())) {
      Serial.println("Temperature sent!");
    }
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
    else {
      Serial.println("Temperature failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(temperature_topic, String(Temperature()).c_str());
    }

    // PUBLISH to the MQTT Broker (topic = Humidity, defined at the beginning)
    if (client.publish(humidity_topic, String(Humidity()).c_str())) {
      Serial.println("Humidity sent!");
    }
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
    else {
      Serial.println("Humidity failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(humidity_topic, String(Humidity()).c_str());
    }

    if (client.publish(pressure_topic, String(BarPressure()).c_str())) {
      Serial.println("Pressure sent!");
    }else {
      Serial.println("Pressure failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(pressure_topic, String(BarPressure()).c_str());
    }

    if (client.publish(windspdavg1m_topic, String(WindSpeedAverage()).c_str())) {
      Serial.println("windspdavg1m sent!");
    }else {
      Serial.println("windspdavg1m failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(windspdavg1m_topic, String(WindSpeedAverage()).c_str());
    }

    if (client.publish(windspdmax5m_topic, String(WindSpeedMax()).c_str())) {
      Serial.println("windspdmax5m sent!");
    }else {
      Serial.println("windspdmax5m failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(windspdmax5m_topic, String(WindSpeedMax()).c_str());
    }

    if (client.publish(winddirection_topic, String(WindDirection()).c_str())) {
      Serial.println("winddirection sent!");
    }else {
      Serial.println("winddirection failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(winddirection_topic, String(WindDirection()).c_str());
    }

    if (client.publish(rain1hr_topic, String(RainfallOneHour()).c_str())) {
      Serial.println("rain1hr sent!");
    }else {
      Serial.println("rain1hr failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(rain1hr_topic, String(RainfallOneHour()).c_str());
    }

    if (client.publish(rain24hr_topic, String(RainfallOneDay()).c_str())) {
      Serial.println("rain24hr sent!");
    }else {
      Serial.println("rain24hr failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(rain24hr_topic, String(RainfallOneDay()).c_str());
    }
    client.disconnect();  // disconnect from the MQTT broker
}

void setup()
{
  Serial.begin(9600);
  ss.begin(2400);
  Serial.print("Hello World :) ");
  oldtime = millis();

}
void loop()
{

  // read weather data from serial (blocking!)
  getBuffer();
  // Check if the data is valid? 
  fdataValid = checkValid(); 
  // Print to serial
  printWeather(); 
  
  // Publish to MQTT
  if(millis() - oldtime > PUBLISH_PERIOD && fdataValid){
    publishMQTT(); 
    oldtime = millis(); 
  }
  
}