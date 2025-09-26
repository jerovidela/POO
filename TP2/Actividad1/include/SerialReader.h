#pragma once
#include <string>
#include <vector>
#include <stdexcept>

namespace tp2 {

class SerialReader {
public:
    explicit SerialReader(const std::string& device = "/dev/ttyACM0", int baud = 115200);
    ~SerialReader();

    // --- NUEVO: escritura cruda al puerto serie ---
    void writeRaw(const std::string& data);

    // Lee una línea (sin \r/\n). Devuelve "" si hubo timeout.
    std::string readLine(int read_timeout_ms = 200);

    // Lee N líneas no vacías.
    std::vector<std::string> readN(size_t n, int read_timeout_ms = 200);

    SerialReader(const SerialReader&) = delete;
    SerialReader& operator=(const SerialReader&) = delete;

private:
    int fd_{-1};
    std::string device_;
    int baud_{115200};

    void openAndConfigure();
    static int baudToFlag(int baud);
};

} // namespace tp2
