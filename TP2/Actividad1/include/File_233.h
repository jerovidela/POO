#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace tp2 {

enum class Mode { Read, Write, Closed };

class File_233 {
public:
    // --- ctors ---
    File_233();
    File_233(const std::string& name, const std::string& datetime,
             const std::string& owner, std::size_t dimension = 0);
    File_233(const std::string& name, const std::string& datetime);

    // --- core API ---
    void open(Mode mode);                // throws on IO errors
    void close() noexcept;
    bool exist() const;

    void write(const std::string& line); // throws on IO errors

    // Reads entire file into cache when open(Read)
    std::string getLine(std::size_t index) const; // throws std::out_of_range

    std::string getCsv()  const;         // returns full content as CSV
    std::string getJson() const;         // converts CSV->JSON (first row as header)
    std::string getXml()  const;         // converts CSV->XML (first row as header)

    // --- metadata / getters ---
    std::string getNombre()     const { return name_; }
    std::string getFecha()      const { return datetime_; }
    std::string getPropietario()const { return owner_; }
    std::size_t getDimension()  const { return dimension_; }

    std::string getInfo() const; // "name: ..., datetime: ..., owner: ..., lines: ..."

    // --- configuration ---
    void setDirectory(const std::filesystem::path& dir);
    void setExtension(const std::string& ext);

private:
    // --- helpers ---
    std::filesystem::path fullPath() const;
    static std::vector<std::string> splitCsvLine(const std::string& line);
    static std::string escapeJson(const std::string& s);
    static std::string escapeXml(const std::string& s);

    void ensureOpenedFor(Mode expected) const;

private:
    // attributes
    std::string name_;
    std::string datetime_;
    std::string owner_;
    std::size_t dimension_{0};

    std::filesystem::path directory_{"./data"}; // fixed directory by default
    std::string extension_{".csv"};             // fixed extension by code

    mutable std::fstream stream_;
    Mode currentMode_{Mode::Closed};
    std::vector<std::string> cache_;            // file lines (including header)
};

} // namespace tp2
