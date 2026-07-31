#pragma once
#include <string>
#include <vector>
#include <set>
#include <cstdint>

std::vector<std::string> loadLib();
void saveLib(const std::vector<std::string>& files);
std::set<std::string> loadStars();
void saveStars(const std::set<std::string>& stars);
void loadCfg(int& vol, bool& shuf, int& rep, int& lastTrack, int64_t& lastPos, bool& starredOnly);
void saveCfg(int vol, bool shuf, int rep, bool starredOnly, int lastTrack, int64_t lastPos);
void scanDir(const std::string& path, std::vector<std::string>& out);
