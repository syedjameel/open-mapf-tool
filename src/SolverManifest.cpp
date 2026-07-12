#include "SolverManifest.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include "Environment.h"

SolverManifest SolverManifest::withDefaults()
{
    const std::vector<std::string> allGrids = {
        "square", "hex-oddr", "hex-evenr", "hex-oddq", "hex-evenq"};

    SolverManifest m;
    m.m_entries = {
        {"cbs", "CBSH2-RTC-CHBP-HEX", "", allGrids},
        {"lacam", "LaCAM (hex fork)", "", allGrids},
        {"lacam2", "LaCAM2 (hex fork)", "", allGrids},
        {"lacam3", "LaCAM3 (hex fork)", "", allGrids},
    };
    return m;
}

bool SolverManifest::loadFromFile(const std::string& path)
{
    try {
        YAML::Node root = YAML::LoadFile(path);
        const YAML::Node& solvers = root["solvers"];
        if (!solvers || !solvers.IsSequence()) {
            std::cerr << "Warning: solver manifest '" << path
                      << "' has no 'solvers' sequence; ignoring it." << std::endl;
            return false;
        }

        for (const auto& node : solvers) {
            const std::string key = node["key"].as<std::string>("");
            if (key.empty()) {
                continue; // entries without a key are meaningless
            }

            SolverEntry* entry = find(key);
            if (entry == nullptr) {
                m_entries.push_back(SolverEntry{key, key, "", {}});
                entry = &m_entries.back();
            }

            entry->name = node["name"].as<std::string>(entry->name);
            const std::string exec = node["executable"].as<std::string>("");
            if (!exec.empty()) {
                entry->executable = exec;
            }
            if (node["grids"] && node["grids"].IsSequence()) {
                entry->grids.clear();
                for (const auto& g : node["grids"]) {
                    entry->grids.push_back(g.as<std::string>());
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Warning: failed to load solver manifest '" << path << "': " << e.what()
                  << "; continuing with current solver configuration." << std::endl;
        return false;
    }
}

bool SolverManifest::saveToFile(const std::string& path) const
{
    try {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        YAML::Emitter out;
        out << YAML::BeginMap << YAML::Key << "solvers" << YAML::Value << YAML::BeginSeq;
        for (const auto& e : m_entries) {
            out << YAML::BeginMap;
            out << YAML::Key << "key" << YAML::Value << e.key;
            out << YAML::Key << "name" << YAML::Value << e.name;
            out << YAML::Key << "executable" << YAML::Value << e.executable;
            out << YAML::Key << "grids" << YAML::Value << YAML::Flow << e.grids;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq << YAML::EndMap;

        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        file << "# OpenMAPF solver manifest -- edited via the GUI 'Solvers' tab\n"
             << out.c_str() << "\n";
        return file.good();
    } catch (const std::exception& e) {
        std::cerr << "Warning: failed to save solver manifest '" << path << "': " << e.what()
                  << std::endl;
        return false;
    }
}

void SolverManifest::applyLegacyEnvironment(const Environment& env)
{
    auto adopt = [this](const char* key, const std::string& exec) {
        if (exec.empty()) {
            return;
        }
        SolverEntry* entry = find(key);
        if (entry != nullptr && entry->executable.empty()) {
            entry->executable = exec;
        }
    };
    adopt("cbs", env.m_solver_cbs);
    adopt("lacam", env.m_solver_lacam);
    adopt("lacam2", env.m_solver_lacam2);
    adopt("lacam3", env.m_solver_lacam3);
}

const SolverEntry* SolverManifest::find(const std::string& key) const
{
    for (const auto& e : m_entries) {
        if (e.key == key) {
            return &e;
        }
    }
    return nullptr;
}

SolverEntry* SolverManifest::find(const std::string& key)
{
    for (auto& e : m_entries) {
        if (e.key == key) {
            return &e;
        }
    }
    return nullptr;
}

std::string SolverManifest::executableFor(const std::string& key) const
{
    const SolverEntry* e = find(key);
    return e != nullptr ? e->executable : "";
}
