#include "SerialReader.h"
#include <cstring>
#include <unistd.h>

#ifdef _WIN32
namespace tp2 {
// (igual que antes: stub con excepción/implementación vacía)
SerialReader::SerialReader(const std::string&, int) { throw std::runtime_error("SerialReader: Windows not implemented yet."); }
SerialReader::~SerialReader() {}
void SerialReader::writeRaw(const std::string&) {}
std::string SerialReader::readLine(int) { return {}; }
std::vector<std::string> SerialReader::readN(size_t, int) { return {}; }
void SerialReader::openAndConfigure() {}
int SerialReader::baudToFlag(int) { return 0; }
} // namespace tp2
#else
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

namespace tp2 {

static inline void trim_crlf(std::string& s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
}

int SerialReader::baudToFlag(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        default:     return 0;
    }
}

SerialReader::SerialReader(const std::string& device, int baud)
: device_(device), baud_(baud) {
    openAndConfigure();
}

SerialReader::~SerialReader() {
    if (fd_ >= 0) ::close(fd_);
}

void SerialReader::openAndConfigure() {
    // --- CAMBIO: abrir lectura y escritura ---
    fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) throw std::runtime_error("SerialReader: cannot open " + device_ + " (" + std::strerror(errno) + ")");

    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) throw std::runtime_error("SerialReader: tcgetattr failed");
    cfmakeraw(&tty);

    int speed = baudToFlag(baud_);
    if (speed == 0) throw std::runtime_error("SerialReader: unsupported baud rate");
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag &= ~PARENB; // 8N1
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) throw std::runtime_error("SerialReader: tcsetattr failed");
    tcflush(fd_, TCIFLUSH);
}

// --- NUEVO: escribir crudo al puerto serie ---
void SerialReader::writeRaw(const std::string& data) {
    if (fd_ < 0) throw std::runtime_error("SerialReader: device not open");
    ssize_t n = ::write(fd_, data.c_str(), data.size());
    if (n < 0) throw std::runtime_error(std::string("SerialReader: write error: ") + std::strerror(errno));
}

std::string SerialReader::readLine(int read_timeout_ms) {
    std::string line;
    char buf[256];
    int elapsed = 0;
    while (true) {
        ssize_t n = ::read(fd_, buf, sizeof(buf));
        if (n > 0) {
            for (ssize_t i = 0; i < n; ++i) {
                char c = buf[i];
                line.push_back(c);
                if (c == '\n') {
                    trim_crlf(line);
                    return line;
                }
            }
        } else if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error(std::string("SerialReader: read error: ") + std::strerror(errno));
        }
        usleep(10000);
        elapsed += 10;
        if (elapsed >= read_timeout_ms) {
            trim_crlf(line);
            return line; // "" si timeout sin datos
        }
    }
}

std::vector<std::string> SerialReader::readN(size_t n, int read_timeout_ms) {
    std::vector<std::string> out;
    out.reserve(n);
    while (out.size() < n) {
        auto s = readLine(read_timeout_ms);
        if (s.empty()) continue;
        out.push_back(std::move(s));
    }
    return out;
}

} // namespace tp2
#endif
