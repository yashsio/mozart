#include "util.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

std::string stripExt(const std::string& path) {
    auto pos = path.find_last_of('/');
    std::string name = (pos == std::string::npos) ? path : path.substr(pos + 1);
    pos = name.find_last_of('.');
    if (pos != std::string::npos) name = name.substr(0, pos);
    return name;
}

std::string stripAudioExt(const std::string& name) {
    auto pos = name.find_last_of('.');
    if (pos == std::string::npos || pos == 0) return name;
    if (!isAudio(name.substr(pos))) return name;
    return name.substr(0, pos);
}

std::string fmtTime(int64_t ms) {
    if (ms < 0) ms = 0;
    int s = (int)(ms / 1000);
    int m = s / 60; s %= 60;
    int h = m / 60; m %= 60;
    char buf[32];
    if (h > 0)
        snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else
        snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

bool isAudio(const std::string& ext) {
    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    return e == ".mp3" || e == ".wav" || e == ".flac" || e == ".ogg"
        || e == ".aac" || e == ".m4a" || e == ".wma" || e == ".opus";
}

int playlistBodyRows(int rows, bool hasSongs) {
    // Rows the playlist may use: terminal height minus the window border (2),
    // the splash line, the three separators, the library header, the status
    // line and the now-playing block.
    int innerRows = rows - 2;
    int fixed = 7 + (hasSongs ? 5 : 4);
    return std::max(0, innerRows - fixed);
}

int colWidth(const std::string& s) {
    int w = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { w++; i++; }
        else if (c < 0xC0) { i++; }
        else if (c < 0xE0) { w++; i += 2; }
        else if (c < 0xF0) { w++; i += 3; }
        else { w++; i += 4; }
    }
    return w;
}

bool matchesQuery(const std::string& path, const std::string& query) {
    if (query.empty()) return true;
    std::string name = path;
    auto pos = name.find_last_of('/');
    if (pos != std::string::npos) name = name.substr(pos + 1);
    pos = name.find_last_of('.');
    if (pos != std::string::npos) name = name.substr(0, pos);
    auto it = std::search(
        name.begin(), name.end(),
        query.begin(), query.end(),
        [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }
    );
    return it != name.end();
}
