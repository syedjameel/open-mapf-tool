#pragma once

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>

// Small RAII helper: writes `content` to a unique file in the system temp
// directory and removes it on destruction.
class TempFile
{
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

// RAII helper: writes an executable script to the system temp directory.
class TempScript
{
public:
    explicit TempScript(const std::string& content) : m_file(content, ".sh")
    {
        std::filesystem::permissions(m_file.path(),
                                     std::filesystem::perms::owner_all |
                                         std::filesystem::perms::group_read |
                                         std::filesystem::perms::group_exec,
                                     std::filesystem::perm_options::replace);
    }

    const std::string& path() const { return m_file.path(); }

private:
    TempFile m_file;
};

// RAII helper: creates a unique directory under the system temp directory
// and removes it (recursively) on destruction.
class TempDir
{
public:
    TempDir()
    {
        static std::mt19937_64 rng{std::random_device{}()};
        std::ostringstream name;
        name << "openmapf-testdir-" << rng();
        m_path = (std::filesystem::temp_directory_path() / name.str()).string();
        std::filesystem::create_directories(m_path);
    }

    ~TempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(m_path, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::string& path() const { return m_path; }

private:
    std::string m_path;
};
