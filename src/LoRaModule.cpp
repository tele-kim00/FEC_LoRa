#include "LoRaModule.hpp"
#include <unistd.h>
#include <iostream>
#include <vector>

LoRaModule::LoRaModule(const std::string& port_name, speed_t baud_rate) : _port(port_name, baud_rate) { sleep(1); }
bool LoRaModule::checkConnection() {
    std::string cmd = "AT\r\n";
    std::vector<uint8_t> cmd_vec(cmd.begin(), cmd.end());
    _port.write(cmd_vec);
    usleep(100000);
    std::vector<uint8_t> buffer;
    _port.read(buffer);
    if (!buffer.empty()) {
        std::string response(buffer.begin(), buffer.end());
        if (response.find("+OK") != std::string::npos) return true;
    }
    return false;
}
bool LoRaModule::sendData(const std::string& data, int address) {
    std::string command_str = "AT+SEND=" + std::to_string(address) + "," + std::to_string(data.length()) + "," + data + "\r\n";
    std::vector<uint8_t> command_vec(command_str.begin(), command_str.end());
    _port.write(command_vec);
    return waitForOk();
}
bool LoRaModule::waitForOk() {
    usleep(600000);
    std::vector<uint8_t> buffer;
    _port.read(buffer);
    if (!buffer.empty()) {
        std::string response(buffer.begin(), buffer.end());
        if (response.find("+OK") != std::string::npos) return true;
    }
    std::cerr << "Error or no response from module." << std::endl;
    return false;
}
