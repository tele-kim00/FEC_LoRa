#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <termios.h>

class SerialPort {
public:
    SerialPort(const std::string& port_name, speed_t baud_rate);
    ~SerialPort();
    ssize_t write(const std::vector<uint8_t>& data);
    ssize_t read(std::vector<uint8_t>& buffer);
private:
    int _fd = -1;
};
