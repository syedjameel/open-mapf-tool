#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include "App.h"
#include "ConfigLocator.h"

#ifndef OPENMAPF_VERSION
#define OPENMAPF_VERSION "unknown"
#endif

namespace {

void printUsage(const char* argv0)
{
    std::cout
        << "OpenMAPF " << OPENMAPF_VERSION
        << " - interactive Multi-Agent Path Finding visualizer\n\n"
        << "Usage: " << argv0 << " [options]\n\n"
        << "Options:\n"
        << "  --config <path>   Use this config file (YAML)\n"
        << "  --version         Print version and exit\n"
        << "  -h, --help        Print this help and exit\n\n"
        << "Config discovery order (first match wins):\n"
        << "  1. --config <path>\n"
        << "  2. $OPENMAPF_CONFIG\n"
        << "  3. ${XDG_CONFIG_HOME:-$HOME/.config}/openmapf/env.yaml\n"
        << "  4. <exe dir>/../share/openmapf/env.yaml\n"
        << "  5. <exe dir>/../config/env.yaml\n"
        << "  6. <exe dir>/config/env.yaml\n"
        << "If no config is found, built-in defaults are used.\n";
}

} // namespace

int main(int argc, char** argv)
{
    std::string cliConfig;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--version") == 0) {
            std::cout << "openmapf " << OPENMAPF_VERSION << std::endl;
            return 0;
        }
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(arg, "--config") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: --config requires a path argument" << std::endl;
                return 2;
            }
            cliConfig = argv[++i];
            continue;
        }
        std::cerr << "Error: unknown argument '" << arg << "'\n" << std::endl;
        printUsage(argv[0]);
        return 2;
    }

    // An explicitly requested config that does not exist is a hard error;
    // silently falling back would hide typos.
    if (!cliConfig.empty() && !std::filesystem::is_regular_file(cliConfig)) {
        std::cerr << "Error: config file not found: " << cliConfig << std::endl;
        return 1;
    }

    auto configPath = ConfigLocator::resolveFromSystem(cliConfig, argv[0]);
    if (configPath) {
        std::cout << "Using config: " << *configPath << std::endl;
    } else {
        std::cerr << "Warning: no config file found; using built-in defaults "
                  << "(see --help for the search order)." << std::endl;
    }

    ImGuiMapfApp app(configPath.value_or(""));
    return app.run();
}
