#include "File_233.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstdlib>

using namespace std;

namespace tp2 {

static std::string nowString() {
    using clock = std::chrono::system_clock;
    auto t = clock::to_time_t(clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

File_233::File_233()
    : name_("unnamed"), datetime_(nowString()),
      owner_(std::getenv("USERNAME") ? std::getenv("USERNAME")
                                     : (std::getenv("USER") ? std::getenv("USER") : "unknown")) {}

File_233::File_233(const std::string& name, const std::string& datetime,
                   const std::string& owner, std::size_t dimension)
    : name_(name), datetime_(datetime), owner_(owner), dimension_(dimension) {}

File_233::File_233(const std::string& name, const std::string& datetime)
    : name_(name), datetime_(datetime),
      owner_(std::getenv("USERNAME") ? std::getenv("USERNAME")
                                     : (std::getenv("USER") ? std::getenv("USER") : "unknown")) {}

void File_233::setDirectory(const std::filesystem::path& dir) {
    directory_ = dir;
}

void File_233::setExtension(const std::string& ext) {
    if (!ext.empty() && ext.front() != '.')
        extension_ = "." + ext;
    else
        extension_ = ext;
}

std::filesystem::path File_233::fullPath() const {
    std::filesystem::path p = directory_ / (name_ + extension_);
    return p;
}

bool File_233::exist() const {
    return std::filesystem::exists(fullPath());
}

void File_233::open(Mode mode) {
    close();
    std::filesystem::create_directories(directory_);
    ios::openmode flags = ios::in;
    if (mode == Mode::Write) flags = ios::out | ios::trunc;
    stream_.open(fullPath(), flags);
    if (!stream_.is_open()) {
        throw std::runtime_error("Failed to open file: " + fullPath().string());
    }
    currentMode_ = mode;

    cache_.clear();
    if (mode == Mode::Read) {
        std::string line;
        while (std::getline(stream_, line)) {
            cache_.push_back(line);
        }
        dimension_ = cache_.size();
        // rewind for potential re-reads (not strictly necessary)
        stream_.clear();
        stream_.seekg(0);
    } else {
        dimension_ = 0;
    }
}

void File_233::close() noexcept {
    if (stream_.is_open()) stream_.close();
    currentMode_ = Mode::Closed;
}

void File_233::ensureOpenedFor(Mode expected) const {
    if (currentMode_ != expected)
        throw std::runtime_error("Invalid file mode for this operation.");
}

void File_233::write(const std::string& line) {
    ensureOpenedFor(Mode::Write);
    stream_ << line << '\n';
    if (!stream_) throw std::runtime_error("Write failed.");
    ++dimension_;
}

std::string File_233::getLine(std::size_t index) const {
    ensureOpenedFor(Mode::Read);
    if (index >= cache_.size()) throw std::out_of_range("Line index out of range.");
    return cache_[index];
}

std::string File_233::getCsv() const {
    ensureOpenedFor(Mode::Read);
    std::ostringstream oss;
    for (std::size_t i = 0; i < cache_.size(); ++i) {
        oss << cache_[i];
        if (i + 1 < cache_.size()) oss << '\n';
    }
    return oss.str();
}

std::vector<std::string> File_233::splitCsvLine(const std::string& line) {
    // Simple split by comma (no quotes/escapes handling).
    std::vector<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == ',') { out.push_back(cur); cur.clear(); }
        else { cur.push_back(c); }
    }
    out.push_back(cur);
    return out;
}

std::string File_233::escapeJson(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
        case '\"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:   o << c; break;
        }
    }
    return o.str();
}

std::string File_233::escapeXml(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
        case '&':  o << "&amp;"; break;
        case '<':  o << "&lt;";  break;
        case '>':  o << "&gt;";  break;
        case '\"': o << "&quot;";break;
        case '\'': o << "&apos;";break;
        default:   o << c;       break;
        }
    }
    return o.str();
}

std::string File_233::getJson() const {
    ensureOpenedFor(Mode::Read);
    if (cache_.empty()) return "[]";
    auto header = splitCsvLine(cache_.front());

    std::ostringstream oss;
    oss << "[";
    bool firstObj = true;
    for (std::size_t i = 1; i < cache_.size(); ++i) {
        auto cols = splitCsvLine(cache_[i]);
        if (!firstObj) oss << ",";
        firstObj = false;
        oss << "\n  {";
        for (std::size_t c = 0; c < header.size(); ++c) {
            std::string key = (c < header.size()) ? header[c] : ("col" + std::to_string(c));
            std::string val = (c < cols.size())   ? cols[c]   : "";
            oss << (c == 0 ? "\n    " : ",\n    ")
                << "\"" << escapeJson(key) << "\": "
                << "\"" << escapeJson(val) << "\"";
        }
        oss << "\n  }";
    }
    oss << (cache_.size() > 1 ? "\n" : "") << "]";
    return oss.str();
}

std::string File_233::getXml() const {
    ensureOpenedFor(Mode::Read);
    if (cache_.empty()) return "<rows/>\n";
    auto header = splitCsvLine(cache_.front());

    std::ostringstream oss;
    oss << "<rows>\n";
    for (std::size_t i = 1; i < cache_.size(); ++i) {
        auto cols = splitCsvLine(cache_[i]);
        oss << "  <row index=\"" << (i-1) << "\">\n";
        for (std::size_t c = 0; c < header.size(); ++c) {
            std::string key = (c < header.size()) ? header[c] : ("col" + std::to_string(c));
            std::string val = (c < cols.size())   ? cols[c]   : "";
            oss << "    <" << key << ">"
                << escapeXml(val)
                << "</" << key << ">\n";
        }
        oss << "  </row>\n";
    }
    oss << "</rows>\n";
    return oss.str();
}

std::string File_233::getInfo() const {
    std::ostringstream oss;
    oss << "name: "      << name_
        << ", datetime: "<< datetime_
        << ", owner: "   << owner_
        << ", lines: "   << dimension_;
    return oss.str();
}

} // namespace tp2
