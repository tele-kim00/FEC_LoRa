#include "SerialPort.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>

SerialPort::SerialPort(const std::string& port_name, speed_t baud_rate) {
    _fd = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (_fd < 0) throw std::runtime_error("Serial port open error: " + port_name);

    struct termios tty;
    if (tcgetattr(_fd, &tty) != 0) throw std::runtime_error("tcgetattr error on:" + port_name);

    cfsetispeed(&tty, baud_rate);
    cfsetospeed(&tty, baud_rate);

    tty.c_cflag |= (CLOCAL | CREAD); tty.c_cflag &= ~PARENB; tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8; tty.c_lflag = 0; tty.c_iflag = 0;
    tty.c_oflag = 0; tty.c_cc[VMIN] = 0; tty.c_cc[VTIME] = 5;

    if (tcsetattr(_fd, TCSANOW, &tty) != 0) throw std::runtime_error("tcsetattr error on: " + port_name);
}
SerialPort::~SerialPort() { if (_fd >= 0) close(_fd); }
ssize_t SerialPort::write(const std::vector<uint8_t>& data) {
    if (_fd < 0) return -1;
    return ::write(_fd, data.data(), data.size());
}
ssize_t SerialPort::read(std::vector<uint8_t>& buffer) {
    if (_fd < 0) return -1;
    char temp_buf[256];
    ssize_t bytes_read = ::read(_fd, temp_buf, sizeof(temp_buf));
    if (bytes_read > 0) buffer.assign(temp_buf, temp_buf + bytes_read);
    return bytes_read;
}
