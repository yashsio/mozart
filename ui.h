#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include <set>
#include <cstdint>

class Player;

#define ICON_MUSIC     "\uf001"
#define ICON_STAR      "\uf005"
#define ICON_PLAY      "\uf04b"
#define ICON_PAUSE     "\uf04c"
#define ICON_STOP      "\uf04d"
#define ICON_PREV      "\uf048"
#define ICON_NEXT      "\uf051"
#define ICON_SHUFFLE   "\uf074"
#define ICON_REPEAT    "\uf01e"
#define ICON_VOL_UP    "\uf028"
#define ICON_FOLDER    "\uf07c"
#define ICON_PLUS      "\uf067"
#define ICON_SEARCH    "\uf002"

ftxui::Element buildUI(
    Player& player,
    const std::vector<std::string>& files,
    const std::set<std::string>& starred,
    int sel, bool showStarredOnly,
    const std::string& searchQuery, bool searchMode,
    bool inputMode, const std::string& inputBuffer,
    const std::string& status, int statusLife,
    bool shuffle, int repeat,
    int starCount, int cols, int rows,
    bool showHelp, bool showVolumePopup);
