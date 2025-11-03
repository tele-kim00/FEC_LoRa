#pragma once
#include "SerialPort.hpp"
#include <string>

class LoRaModule {
public:
    LoRaModule(const std::string& port_name, speed_t baud_rate);
    bool checkConnection();
    bool sendData(const std::string& data, int address);
private:
    SerialPort _port;
    bool waitForOk();
};
