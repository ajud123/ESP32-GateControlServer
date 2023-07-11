#pragma once
#include <string>
#include <WiFi.h>
namespace GateSettings {

    const std::string WiFiSSID = "Wifi-SSID";
    const std::string WiFiPassword = "Wifi-Password";
    const std::string Hostname = "Gate";

    IPAddress local_IP(192, 168, 1, 164);
    IPAddress gateway(192, 168, 1, 254);
    IPAddress subnet(255, 255, 255, 0);

    const int ServerPort = 80;

    const std::string OTAPassword = "ChangeThisPassword";

    const bool LoggingEnabled = false;
    const std::string LogURL = "http://127.0.0.1/send_log";

    const std::string OpenGate = "/open";
    const std::string CloseGate = "/close";
    const std::string StopGate = "/stop";
    const std::string PedestrianGate = "/pedestrian";
    const std::string GetPosition = "/GetPOS";

    const uint8_t gpio_gate_open = 15;
    const uint8_t gpio_gate_close = 18;
    const uint8_t gpio_gate_stop = 4;
    const uint8_t gpio_gate_ped = 5;
}

/*

OTA UPDATE SETTINGS FOR platformio.ini
;upload_protocol = espota
;upload_port = 192.168.1.164
;upload_flags = 
;	--port=3232
;	--auth=OTAPassword

*/