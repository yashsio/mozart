#include "config.h"
#include "util.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <algorithm>

namespace fs = std::filesystem;

static std::string cfgDir() {
    const char* h = getenv("HOME");
    return std::string(h ? h : "/tmp") + "/.config/mozart";
}

static std::string cfgFile(const char* name) {
    return cfgDir() + "/" + name;
}

static void ensureDir() {
    fs::create_directories(cfgDir());
}

std::vector<std::string> loadLib() {
    std::vector<std::string> files;
    std::ifstream in(cfgFile("library"));
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && fs::exists(line))
            files.push_back(line);
    }
    return files;
}

void saveLib(const std::vector<std::string>& files) {
    ensureDir();
    std::ofstream out(cfgFile("library"));
    for (auto& f : files) out << f << '\n';
}

std::set<std::string> loadStars() {
    std::set<std::string> stars;
    std::ifstream in(cfgFile("starred"));
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) stars.insert(line);
    }
    return stars;
}

void saveStars(const std::set<std::string>& stars) {
    ensureDir();
    std::ofstream out(cfgFile("starred"));
    for (auto& s : stars) out << s << '\n';
}

void loadCfg(int& vol, bool& shuf, int& rep, int& lastTrack, int64_t& lastPos, bool& starredOnly) {
    std::ifstream in(cfgFile("config"));
    std::string line;
    while (std::getline(in, line)) {
        try {
            if (line.rfind("volume=", 0) == 0) vol = std::stoi(line.substr(7));
            else if (line.rfind("shuffle=", 0) == 0) shuf = (line.substr(8) == "true");
            else if (line.rfind("repeat=", 0) == 0) rep = std::stoi(line.substr(7));
            else if (line.rfind("last_track=", 0) == 0) lastTrack = std::stoi(line.substr(11));
            else if (line.rfind("last_position=", 0) == 0) lastPos = std::stoll(line.substr(14));
            else if (line.rfind("starred_only=", 0) == 0) starredOnly = (line.substr(13) == "true");
        } catch (...) {}
    }
}

void saveCfg(int vol, bool shuf, int rep, bool starredOnly, int lastTrack, int64_t lastPos) {
    ensureDir();
    std::ofstream out(cfgFile("config"));
    out << "volume=" << vol << '\n';
    out << "shuffle=" << (shuf ? "true" : "false") << '\n';
    out << "repeat=" << rep << '\n';
    out << "starred_only=" << (starredOnly ? "true" : "false") << '\n';
    out << "last_track=" << lastTrack << '\n';
    out << "last_position=" << lastPos << '\n';
}

void scanDir(const std::string& path, std::vector<std::string>& out) {
    try {
        for (auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && isAudio(entry.path().extension().string())) {
                auto p = entry.path().string();
                if (std::find(out.begin(), out.end(), p) == out.end())
                    out.push_back(p);
            }
        }
    } catch (...) {}
}
