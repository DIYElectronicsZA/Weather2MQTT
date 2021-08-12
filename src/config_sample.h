/* config_sample.h
*!! rename this file config.h
*and enter your login credentials below 
*/

// Your WiFi credentials.
// Set password to "" for open networks.
const char* ssid = "";                 // Your personal network SSID
const char* pass = "";                  // Your personal network password

//MQTT
const char* mqtt_server = "192.168.88.200";  // IP of the MQTT broker
const char* mqtt_username = ""; // MQTT username
const char* mqtt_password = ""; // MQTT password
const char* clientID = "weatherstation"; // MQTT client ID