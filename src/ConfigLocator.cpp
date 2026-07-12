#include "ConfigLocator.h"

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace ConfigLocator {

std::vector<std::string> candidatePaths(const Inputs& in, const std::string& fileName)
{
    std::vector<std::string> candidates;

    if (!in.cliPath.empty()) {
        candidates.push_back(in.cliPath);
    }
    if (!in.envPath.empty()) {
        candidates.push_back(in.envPath);
    }

    const std::string configHome = userConfigDir(in);
    if (!configHome.empty()) {
        candidates.push_back(configHome + "/" + fileName);
    }

    if (!in.exeDir.empty()) {
        candidates.push_back(in.exeDir + "/../share/openmapf/" + fileName);
        candidates.push_back(in.exeDir + "/../config/" + fileName);
        candidates.push_back(in.exeDir + "/config/" + fileName);
    }

    return candidates;
}

std::string userConfigDir(const Inputs& in)
{
    if (!in.xdgConfigHome.empty()) {
        return in.xdgConfigHome + "/openmapf";
    }
    if (!in.homeDir.empty()) {
        return in.homeDir + "/.config/openmapf";
    }
    return "";
}

std::optional<std::string> resolve(const Inputs& in,
                                   const std::function<bool(const std::string&)>& exists,
                                   const std::string& fileName)
{
    for (const auto& candidate : candidatePaths(in, fileName)) {
        if (exists(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::string executableDirectory(const char* argv0)
{
    std::error_code ec;

    // Linux: authoritative path of the running binary
    auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && exe.has_parent_path()) {
        return exe.parent_path().string();
    }

    // Fallback: derive from argv[0]
    if (argv0 != nullptr && argv0[0] != '\0') {
        auto p = std::filesystem::absolute(std::filesystem::path(argv0), ec);
        if (!ec && p.has_parent_path()) {
            return p.parent_path().string();
        }
    }

    return "";
}

Inputs systemInputs(const std::string& cliPath, const char* argv0)
{
    auto getEnv = [](const char* name) -> std::string {
        const char* v = std::getenv(name);
        return v != nullptr ? v : "";
    };

    Inputs in;
    in.cliPath = cliPath;
    in.envPath = getEnv("OPENMAPF_CONFIG");
    in.xdgConfigHome = getEnv("XDG_CONFIG_HOME");
    in.homeDir = getEnv("HOME");
    in.exeDir = executableDirectory(argv0);
    return in;
}

std::optional<std::string> resolveFromSystem(const std::string& cliPath, const char* argv0,
                                             const std::string& fileName)
{
    return resolve(systemInputs(cliPath, argv0),
                   [](const std::string& p) {
                       std::error_code ec;
                       return std::filesystem::is_regular_file(p, ec);
                   },
                   fileName);
}

} // namespace ConfigLocator
