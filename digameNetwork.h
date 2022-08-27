/* digameNetwork.h
 *
 *  Functions for logging into a wifi network and reporting the device's
 *  MAC address, and POSTING a JSON message to a server.
 *
 *  Copyright 2021, Digame Systems. All rights reserved.
 */

#ifndef __DIGAME_NETWORK_H__
#define __DIGAME_NETWORK_H__

#include <WiFi.h>             // WiFi stack
#include <HTTPClient.h>       // To post to the ParkData Server
#include <digameJSONConfig.h> // for Config struct that holds network credentials

#define debugUART Serial

// Globals
bool wifiConnected = false;
unsigned long msLastConnectionAttempt; // Timer value of the last time we tried to connect to the wifi.
HTTPClient http;                       // The class we use to POST messages
unsigned long msLastPostTime;          // Timer value of the last time we did an http POST.

//*****************************************************************************
// Return the device's MAC address as a String
String getMACAddress()
{

    byte mac[6];
    String retString;

    WiFi.macAddress(mac);

    char buffer[3];
    for (int i = 0; i<5; i++){
      sprintf(buffer, "%02x", mac[i]);
      retString = String(retString + buffer + ":");
    }
    sprintf(buffer, "%02x", mac[5]);
    retString = String(retString + buffer);

    /*
    retString = String(String(mac[0], HEX) + ":");
    retString = String(retString + String(mac[1], HEX) + ":");
    retString = String(retString + String(mac[2], HEX) + ":");
    retString = String(retString + String(mac[3], HEX) + ":");
    retString = String(retString + String(mac[4], HEX) + ":");
    retString = String(retString + String(mac[5], HEX));
    */

    return retString;
}

//*****************************************************************************
// Return the last four digits of the device's MAC address as a String
String getShortMACAddress()
{
    byte mac[6];
    String retString;

    WiFi.macAddress(mac);
    // DEBUG_PRINTLN(mac);
    char buffer[3];
    sprintf(buffer, "%02x", mac[4]);
    retString = String(retString + buffer);
    sprintf(buffer, "%02x", mac[5]);
    retString = String(retString + buffer);

    return retString;
}
//*****************************************************************************
// Enable WiFi and log into the network
bool enableWiFi(Config config)
{
    String ssid = config.ssid;
    String password = config.password;
    DEBUG_PRINTLN("  Starting WiFi");
    DEBUG_PRINTLN("    SSID: " + ssid);
    DEBUG_PRINTLN("    Password: " + password);
    DEBUG_PRINTLN("    Server URL: " + config.serverURL);
    
    WiFi.disconnect();   // Disconnect the network
    WiFi.mode(WIFI_OFF); // Switch WiFi off

    //setCpuFrequencyMhz(80); //TODO: Figure out a better place for this.
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    WiFi.mode(WIFI_STA); // Station mode
    vTaskDelay(250 / portTICK_PERIOD_MS);

    String hostName = config.deviceName;
    hostName.replace(" ", "_");
    WiFi.setHostname(hostName.c_str());         // define hostname
    WiFi.begin(ssid.c_str(), password.c_str()); // Log in

    bool timedout = false;
    unsigned long wifiTimeout = 30000;
    unsigned long tstart = millis();
    DEBUG_PRINT("    ")
    while (WiFi.status() != WL_CONNECTED)
    {
        //delay(1000);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        DEBUG_PRINT(".");
        if ((millis() - tstart) > wifiTimeout)
        {
            DEBUG_PRINTLN("    Timeout attempting WiFi connection.");
            timedout = true;
            break;
        }
    }

    msLastConnectionAttempt = millis(); // Timer value of the last time we tried to connect to the wifi.

    if (timedout)
    {
        wifiConnected = false;
        return false;
    }
    else
    {
        debugUART.println("");
        debugUART.println("    WiFi connected.");
        debugUART.print("    IP address: ");
        debugUART.println(WiFi.localIP());
        wifiConnected = true;

        // Your Domain name with URL path or IP address with path
        // http.begin(config.serverURL);

        return true;
    }
}

bool initWiFi(Config config)
{
    return enableWiFi(config);
}

//*****************************************************************************
void disableWiFi()
{
    WiFi.setSleep(true);
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    //setCpuFrequencyMhz(40); // Drop cpu down to conserve power
    debugUART.println("");
    debugUART.println("WiFi disconnected!");
    wifiConnected = false;
}

//*****************************************************************************
// Save a single JSON message to the server. TODO: Better option for retries.
//*****************************************************************************
bool postJSON(String jsonPayload, Config config)
{

    if (config.showDataStream == "false")
    {
        debugUART.print("postJSON Running on Core #: ");
        debugUART.println(xPortGetCoreID());
        // debugUART.print("Free Heap: ");
        // debugUART.println(ESP.getFreeHeap());
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        debugUART.println("WiFi not connected.");
        if (enableWiFi(config) == false)
        {
            return false;
        };
    }

    unsigned long t1 = millis();

    http.begin(config.serverURL);

    if (config.showDataStream == "false")
    {
        debugUART.print("JSON payload length: ");
        debugUART.println(jsonPayload.length());
        debugUART.print("HTTP begin Time: ");
        debugUART.println(millis() - t1);
    }

    // If you need an HTTP request with a content type: application/json, use the following:
    http.addHeader("Content-Type", "application/json");

    t1 = millis();
    int httpResponseCode = http.POST(jsonPayload);

    if (config.showDataStream == "false")
    {
        debugUART.print("POST Time: ");
        debugUART.println(millis() - t1);
        debugUART.println("POSTing to Server:");
        debugUART.println(jsonPayload);
        debugUART.print("HTTP response code: ");
        debugUART.println(httpResponseCode);
        if (!(httpResponseCode == 200))
        {
            debugUART.println("*****ERROR*****");
            debugUART.println(http.errorToString(httpResponseCode));
        }
        debugUART.println();
    }
    // Free resources
    http.end();

    if (httpResponseCode == 200)
    {
        msLastPostTime = millis(); // Log the time of the last successful post
        return true;
    }
    else
    {
        msLastPostTime = millis(); // Log the time of the last successful post
        // TODO: verify this is the right approach. if we have a bunch of network
        // errors, not setting msLastPostTime here, might cause us to put the wifi 
        // to sleep over and over...
        return false;
    }
}

#endif //__DIGAME_NETWORK_H__
