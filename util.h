#pragma once
#include <string>
#include <vector>
#include <cstdint>

std::string stripExt(const std::string& path);
std::string stripAudioExt(const std::string& name);
int playlistBodyRows(int rows, bool hasSongs);
std::string fmtTime(int64_t ms);
bool isAudio(const std::string& ext);
int colWidth(const std::string& s);
bool matchesQuery(const std::string& path, const std::string& query);
