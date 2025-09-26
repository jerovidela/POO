#include "File_233.h"      // cambia a File_NNN.h si corresponde
#include "SerialReader.h"

#include <iostream>
#include <string>
#include <cctype>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <sstream>

struct Args {
    std::string filename;
    bool readMode{false};
    bool writeMode{false};
    char inFormat{'c'};   // c|j|x (para -w: lo que pedimos al Arduino o lo que llega por stdin)
    char outFormat{'c'};  // c|j|x (para -r: cómo mostrar)
    std::size_t n{0};     // lecturas a capturar
    std::string dir{"./data"};

    // puerto serie
    bool useSerial{false};
    std::string serialDev{"/dev/ttyACM0"};
    int baud{19200};      // tu sketch usa 19200
};

static void printUsage(const char* prog) {
    std::cerr <<
    "Usage:\n"
    "  " << prog << " <filename> (-r|-w) [-f c|j|x] [-o c|j|x] [-n N] [--dir PATH]\n"
    "                [--serial [/dev/ttyACM0]] [--baud 19200]\n\n"
    "Examples:\n"
    "  " << prog << " data -w -f c -n 200 --serial /dev/ttyUSB0 --baud 19200\n"
    "  " << prog << " data -w -f j -n 50 --serial /dev/ttyUSB0 --baud 19200\n"
    "  " << prog << " data -r -o j\n";
}

static bool parseArgs(int argc, char** argv, Args& a) {
    if (argc < 3) return false;
    a.filename = argv[1];
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "-r") a.readMode = true;
        else if (arg == "-w") a.writeMode = true;
        else if (arg == "-f" && i + 1 < argc) { a.inFormat  = std::tolower(argv[++i][0]); }
        else if (arg == "-o" && i + 1 < argc) { a.outFormat = std::tolower(argv[++i][0]); }
        else if (arg == "-n" && i + 1 < argc) { a.n = static_cast<std::size_t>(std::stoul(argv[++i])); }
        else if (arg == "--dir" && i + 1 < argc) { a.dir = argv[++i]; }
        else if (arg == "--serial") {
            a.useSerial = true;
            if (i + 1 < argc && argv[i+1][0] != '-') a.serialDev = argv[++i];
        }
        else if (arg == "--baud" && i + 1 < argc) { a.baud = std::stoi(argv[++i]); }
        else return false;
    }
    if (a.readMode == a.writeMode) return false;       // elegir exactamente uno
    if (a.writeMode && a.n == 0) a.n = 10;             // default razonable
    return true;
}

static inline void strip_cr(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

// --- util simple para CSV de 4 columnas ---
static bool splitCsv4(const std::string& line, std::vector<std::string>& out) {
    out.clear();
    std::string cur;
    for (char c : line) {
        if (c == ',') { out.push_back(cur); cur.clear(); }
        else if (c != '\r' && c != '\n') { cur.push_back(c); }
    }
    out.push_back(cur);
    return out.size() == 4;
}

// --- parseo mínimo del JSON del sketch a CSV ---
// Asume claves: dispositivo_id (int), porcentaje_valvula (int),
// estado_nivel (string), caudal (float). Tolera coma colgante antes de '}'.
static bool jsonBlockToCsvLine(std::string block, std::string& outCsv) {
    // quitar CR
    block.erase(std::remove(block.begin(), block.end(), '\r'), block.end());
    // fix coma colgante ",\n}" -> "\n}"
    std::string::size_type pos = block.rfind(",\n}");
    if (pos != std::string::npos) block.replace(pos, 3, "\n}");

    auto findValInt = [&](const char* key, long& v)->bool{
        std::string k = std::string("\"") + key + "\":";
        auto p = block.find(k);
        if (p == std::string::npos) return false;
        p += k.size();
        while (p < block.size() && std::isspace(static_cast<unsigned char>(block[p]))) ++p;
        std::size_t endp{};
        try { v = std::stol(block.substr(p), &endp); } catch (...) { return false; }
        return true;
    };
    auto findValFloat = [&](const char* key, double& v)->bool{
        std::string k = std::string("\"") + key + "\":";
        auto p = block.find(k);
        if (p == std::string::npos) return false;
        p += k.size();
        while (p < block.size() && std::isspace(static_cast<unsigned char>(block[p]))) ++p;
        std::size_t endp{};
        try { v = std::stod(block.substr(p), &endp); } catch (...) { return false; }
        return true;
    };
    auto findValString = [&](const char* key, std::string& v)->bool{
        std::string k = std::string("\"") + key + "\":";
        auto p = block.find(k);
        if (p == std::string::npos) return false;
        p += k.size();
        while (p < block.size() && std::isspace(static_cast<unsigned char>(block[p]))) ++p;
        if (p >= block.size() || block[p] != '\"') return false;
        ++p;
        std::string s;
        while (p < block.size() && block[p] != '\"') { s.push_back(block[p]); ++p; }
        if (p >= block.size()) return false;
        v = s; return true;
    };

    long dispositivo_id = 0;
    long porcentaje_valvula = 0;
    std::string estado_nivel;
    double caudal = 0.0;

    if (!findValInt("dispositivo_id", dispositivo_id)) return false;
    if (!findValInt("porcentaje_valvula", porcentaje_valvula)) return false;
    if (!findValString("estado_nivel", estado_nivel)) return false;
    if (!findValFloat("caudal", caudal)) return false;

    std::ostringstream oss;
    oss << dispositivo_id << "," << porcentaje_valvula << "," << estado_nivel << "," << caudal;
    outCsv = oss.str();
    return true;
}

// --- parseo mínimo de XML del sketch a CSV ---
static bool xmlBlockToCsvLine(std::string block, std::string& outCsv) {
    auto getTag = [&](const char* tag, std::string& v)->bool{
        std::string open = std::string("<") + tag + ">";
        std::string close = std::string("</") + tag + ">";
        auto p1 = block.find(open);
        auto p2 = block.find(close);
        if (p1 == std::string::npos || p2 == std::string::npos || p2 <= p1) return false;
        p1 += open.size();
        v = block.substr(p1, p2 - p1);
        // trim básico
        while (!v.empty() && (v.back()=='\r' || v.back()=='\n' || v.back()==' ' || v.back()=='\t')) v.pop_back();
        while (!v.empty() && (v.front()=='\r' || v.front()=='\n' || v.front()==' ' || v.front()=='\t')) v.erase(v.begin());
        return true;
    };

    std::string s_id, s_pv, s_niv, s_caud;
    if (!getTag("dispositivo_id", s_id)) return false;
    if (!getTag("porcentaje_valvula", s_pv)) return false;
    if (!getTag("estado_nivel", s_niv)) return false;
    if (!getTag("caudal", s_caud)) return false;

    std::ostringstream oss;
    oss << s_id << "," << s_pv << "," << s_niv << "," << s_caud;
    outCsv = oss.str();
    return true;
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    Args args;
    if (!parseArgs(argc, argv, args)) {
        printUsage(argv[0]);
        return 1;
    }

    try {
        tp2::File_233 f{args.filename, "created-at-runtime"};  // cambia si tu clase se llama distinto
        f.setDirectory(args.dir);
    // SIEMPRE CSV (lo pide la consigna que elegiste)
        f.setExtension(".csv");

        // ---------- ESCRITURA ----------
        if (args.writeMode) {
            if (args.n == 0) throw std::runtime_error("Number of readings (-n) must be > 0");
            if (!args.useSerial && (args.inFormat != 'c' && args.inFormat != 'j' && args.inFormat != 'x'))
                throw std::runtime_error("Invalid input format (-f): use c|j|x");

            f.open(tp2::Mode::Write);

            std::size_t count = 0;
            // Encabezado CSV una sola vez
            f.write("dispositivo_id,porcentaje_valvula,estado_nivel,caudal");

            if (args.useSerial) {
                tp2::SerialReader sr(args.serialDev, args.baud);

                // La UNO suele resetear al abrir → pequeña espera y drenado corto
                std::this_thread::sleep_for(std::chrono::milliseconds(600));
                auto t0 = std::chrono::steady_clock::now();
                while (std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - t0).count() < 300) {
                    (void)sr.readLine(30); // ignorar todo (banner u otros restos)
                }

                std::vector<std::string> cols;

                while (count < args.n) {
                    if (args.inFormat == 'c') {
                        // --- CSV: sincronizar con línea de 4 columnas ---
                        sr.writeRaw("c\n");
                        std::string line;
                        auto start = std::chrono::steady_clock::now();
                        while (true) {
                            line = sr.readLine(250);
                            if (!line.empty()) {
                                strip_cr(line);
                                if (std::count(line.begin(), line.end(), ',') == 3) {
                                    f.write(line);
                                    ++count;
                                    break;
                                }
                            }
                            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - start).count() > 2000) {
                                // no llegó CSV válido en 2s → abandonamos esta muestra
                                break;
                            }
                        }
                    }
                    else if (args.inFormat == 'j') {
                        // --- JSON: buscar '{' y terminar en '}' ---
                        sr.writeRaw("j\n");

                        std::string block;
                        auto start = std::chrono::steady_clock::now();

                        // 1) buscar inicio '{'
                        while (true) {
                            auto ln = sr.readLine(250);
                            if (!ln.empty()) {
                                strip_cr(ln);
                                if (ln.find('{') != std::string::npos) {
                                    block = ln + "\n";
                                    break;
                                }
                            }
                            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - start).count() > 2000) {
                                block.clear(); break;
                            }
                        }

                        // 2) acumular hasta '}'
                        if (!block.empty()) {
                            auto start2 = std::chrono::steady_clock::now();
                            while (true) {
                                auto ln = sr.readLine(250);
                                if (!ln.empty()) {
                                    strip_cr(ln);
                                    block += ln + "\n";
                                    if (ln.find('}') != std::string::npos) break;
                                }
                                if (std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now() - start2).count() > 2000) {
                                    block.clear(); break;
                                }
                            }
                        }

                        if (!block.empty()) {
                            std::string csvLine;
                            if (jsonBlockToCsvLine(block, csvLine)) {
                                f.write(csvLine);
                                ++count;
                            }
                        }
                    }
                    else { // args.inFormat == 'x'
                        // --- XML: buscar inicio plausible y terminar en </registro> ---
                        sr.writeRaw("x\n");

                        std::string block;
                        auto start = std::chrono::steady_clock::now();

                        // 1) buscar inicio (<?xml o <registro>)
                        while (true) {
                            auto ln = sr.readLine(250);
                            if (!ln.empty()) {
                                strip_cr(ln);
                                if (ln.rfind("Inicio de sketch", 0) == 0) continue; // ignorar banner
                                if (ln.find("<registro>") != std::string::npos || ln.find("<?xml") != std::string::npos) {
                                    block = ln + "\n";
                                    break;
                                }
                            }
                            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - start).count() > 2000) {
                                block.clear(); break;
                            }
                        }

                        // 2) acumular hasta </registro>
                        if (!block.empty()) {
                            auto start2 = std::chrono::steady_clock::now();
                            while (true) {
                                auto ln = sr.readLine(250);
                                if (!ln.empty()) {
                                    strip_cr(ln);
                                    block += ln + "\n";
                                    if (ln.find("</registro>") != std::string::npos) break;
                                }
                                if (std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now() - start2).count() > 2000) {
                                    block.clear(); break;
                                }
                            }
                        }

                        if (!block.empty()) {
                            std::string csvLine;
                            if (xmlBlockToCsvLine(block, csvLine)) {
                                f.write(csvLine);
                                ++count;
                            }
                        }
                    }
                }
            } else {
                // STDIN: escribir N líneas tal como llegan
                std::string line;
                while (count < args.n && std::getline(std::cin, line)) {
                    strip_cr(line);
                    if (line.empty()) continue;
                    f.write(line);
                    ++count;
                }
            }

            f.close();
            std::cout << "OK: wrote " << f.getDimension() << " lines to "
                      << args.dir << "/" << args.filename << ".csv\n";
            return 0;
        }

        // ---------- LECTURA ----------
        if (args.readMode) {
            if (!f.exist()) throw std::runtime_error("File does not exist.");
            f.open(tp2::Mode::Read);

            std::cout << "#info," << f.getNombre() << "," << f.getFecha()
                      << "," << f.getPropietario() << "," << f.getDimension() << "\n";

            switch (args.outFormat) {
                case 'c': std::cout << f.getCsv()  << "\n"; break;
                case 'j': std::cout << f.getJson() << "\n"; break;
                case 'x': std::cout << f.getXml();          break;
                default: throw std::runtime_error("Invalid output format. Use c|j|x.");
            }

            f.close();
            return 0;
        }

        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << "\n";
        return 2;
    }
}
