#pragma once

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>

// Small RAII helper: writes `content` to a unique file in the system temp
// directory and removes it on destruction.
class TempFile {
public:
    explicit TempFile(const std::string& content, const std::string& extension = ".map")
    {
        static std::mt19937_64 rng{std::random_device{}()};
        std::ostringstream name;
        name << "openmapf-test-" << rng() << extension;
        m_path = (std::filesystem::temp_directory_path() / name.str()).string();
        std::ofstream out(m_path, std::ios::binary);
        out << content;
    }

    ~TempFile()
    {
        std::error_code ec;
        std::filesystem::remove(m_path, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    const std::string& path() const { return m_path; }

private:
    std::string m_path;
};
