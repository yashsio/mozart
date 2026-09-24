#include "ui.h"
#include "splash.h"
#include "player.h"
#include "util.h"
#include <ftxui/dom/elements.hpp>
#include <cstdint>
#include <algorithm>
#include <string>
#include <vector>
#include <set>

using namespace ftxui;

static std::string trunc(const std::string& s, int maxW) {
    if (maxW < 8) return s.substr(0, std::max(0, maxW));
    std::string r = s;
    if (colWidth(r) > maxW) {
        while (colWidth(r) > maxW - 3 && !r.empty()) r.pop_back();
        r += "...";
    }
    return r;
}

Element buildUI(
    Player& player,
    const std::vector<std::string>& files,
    const std::set<std::string>& starred,
    int sel, bool showStarredOnly,
    const std::string& searchQuery, bool searchMode,
    bool inputMode, const std::string& inputBuffer,
    const std::string& status, int statusLife,
    bool shuffle, int repeat,
    int starCount, int cols, int rows,
    bool showHelp, bool showVolumePopup)
{
    int ic = cols - 4;

    auto isVis = [&](int i) -> bool {
        if (i < 0 || i >= (int)files.size()) return false;
        if (showStarredOnly && !starred.count(files[i])) return false;
        if (!matchesQuery(files[i], searchQuery)) return false;
        return true;
    };

    std::vector<int> visible;
    int visPos = -1;
    for (int i = 0; i < (int)files.size(); i++) {
        if (!isVis(i)) continue;
        if (i == sel) visPos = (int)visible.size();
        visible.push_back(i);
    }

    int totalVis = (int)visible.size();
    bool hasSongs = player.songCount() > 0;
    int vol = player.volume();

    // ── Status/Info line ──
    std::string volStr;
    volStr += ICON_VOL_UP;
    volStr += " ";
    volStr += " " + std::to_string(vol) + "%";

    std::string shufStr = shuffle ? std::string(ICON_SHUFFLE) : std::string(1, ' ');
    std::string repStr;
    if (repeat == 1) repStr = "1";
    else if (repeat == 2) repStr = std::string(ICON_REPEAT);
    else repStr = std::string(1, ' ');

    Element infoLine = hbox({
        buildSplash(ic),
        filler(),
        text(volStr) | color(Color::Yellow),
        text("  "),
        text(shufStr) | (shuffle ? bold : dim) | size(WIDTH, EQUAL, 1),
        text(" "),
        text(repStr) | (repeat ? bold : dim) | size(WIDTH, EQUAL, 1),
        text("  "),
        text(std::string(ICON_STAR) + " " + std::to_string(starCount)) | color(Color::Yellow),
        text("  "),
    });

    // ── Now Playing ──
    Elements np;

    if (hasSongs) {
        std::string title = player.currentTitle();
        if (title.empty()) title = player.currentIndex() >= 0
            ? stripExt(files[player.currentIndex()]) : "(no title)";
        title = trunc(title, ic - 12);

        const char* si = player.isPlaying() ? ICON_PAUSE
                         : (player.isPaused() ? ICON_PLAY : ICON_PLAY);

        bool starredTrack = player.currentIndex() >= 0
            && player.currentIndex() < (int)files.size()
            && starred.count(files[player.currentIndex()]);

        int64_t pos = player.position();
        int64_t dur = player.duration();
        double ratio = 0.0;
        if (dur > 0)
            ratio = std::min(1.0, std::max(0.0, (double)pos / dur));

        int gw = std::clamp(ic - 4, 10, 60);
        std::string timeStr;
        if (dur > 0) {
            timeStr = fmtTime(pos) + "  /  " + fmtTime(dur);
            if ((int)timeStr.size() > ic) timeStr.clear();
        }

        np.push_back(text(""));
        np.push_back(hbox({
            filler(),
            text(std::string(si) + "  " + title) | bold | color(Color::Green),
            text(starredTrack ? std::string("  ") + ICON_STAR : std::string(4, ' ')) | color(Color::Yellow),
            filler(),
        }));
        np.push_back(text(""));
        np.push_back(hbox({filler(), gauge((float)ratio) | color(Color::Green) | size(WIDTH, EQUAL, gw), filler()}));
        np.push_back(hbox({filler(), text(timeStr) | dim, filler()}));
    } else {
        np.push_back(text(""));
        np.push_back(hbox({filler(), text(std::string(ICON_MUSIC) + "  No tracks in library") | color(Color::Cyan), filler()}));
        np.push_back(text(""));
        np.push_back(hbox({filler(), text("") | dim, filler()}));
        np.push_back(hbox({filler(), text("  Add with [a] or:  mozart  ~/Music") | dim, filler()}));
    }

    Element nowPlaying = vbox(std::move(np));

    // ── Library Header ──
    std::string libLabel = "  Library";
    if (totalVis > 0) libLabel += " (" + std::to_string(totalVis) + ")";
    std::string libRight;
    if (showStarredOnly) libRight += std::string(ICON_STAR) + " filtered";
    if (totalVis > 0 && visPos >= 0) {
        if (!libRight.empty()) libRight += "  ";
        libRight += std::to_string(visPos + 1) + "/" + std::to_string(totalVis);
    }

    Element libHeader = hbox({
        text(libLabel) | bold | color(Color::Cyan),
        filler(),
        text(libRight) | dim,
        text("  "),
    });

    // ── Playlist ──
    int innerRows = rows - 2;
    int fixed = 1 + 1 + (int)np.size() + 1 + 1 + 1 + 1 + 1;
    int avail = std::max(0, innerRows - fixed);

    int scrollOff = 0;
    if (visPos >= 0) {
        if (visPos >= scrollOff + avail) scrollOff = visPos - avail + 1;
        if (visPos < scrollOff) scrollOff = visPos;
    }
    if (scrollOff < 0) scrollOff = 0;

    bool hasAbove = scrollOff > 0;
    bool hasBelow = totalVis > scrollOff + avail;

    Elements pl;
    for (int vi = scrollOff; vi < totalVis && vi < scrollOff + avail; vi++) {
        int i = visible[vi];
        std::string name = stripExt(files[i]);
        name = trunc(name, ic - 8);

        bool starred_track = starred.count(files[i]);
        bool is_current = (i == player.currentIndex() && i >= 0);
        bool is_selected = (i == sel);

        std::string now_chr = is_current ? std::string(ICON_MUSIC) : " ";
        std::string star_chr = starred_track ? std::string(ICON_STAR) : " ";

        Element prefix = text("  " + now_chr + " ");
        if (is_current) prefix = prefix | color(Color::Green);

        Element starE = text(star_chr);
        if (starred_track) starE = starE | color(Color::Yellow);

        Element suffix = text("  " + name);

        Element item = hbox({prefix, starE, suffix});
        if (is_selected)
            item = item | inverted;
        else if (is_current)
            item = item | bold;
        pl.push_back(item);
    }

    if (hasAbove) {
        int n = scrollOff;
        pl.insert(pl.begin(),
            hbox({filler(), text("\u25B4 " + std::to_string(n) + " more \u25B4") | dim, filler()}));
    }
    if (hasBelow) {
        int n = totalVis - (scrollOff + avail);
        pl.push_back(
            hbox({filler(), text("\u25BE " + std::to_string(n) + " more \u25BE") | dim, filler()}));
    }
    if (pl.empty()) pl.push_back(text(""));

    Element playlist = vbox(std::move(pl)) | flex;

    // ── Status line (replaces old help bar) ──
    Element statusLine;
    if (inputMode) {
        statusLine = hbox({
            text(" "),
            text("Add directory: " + inputBuffer + "\u258C") | inverted | flex,
            text(" "),
        });
    } else if (searchMode) {
        statusLine = hbox({
            text(" "),
            text("/" + searchQuery + "\u258C") | inverted | flex,
            text(" "),
        });
    } else if (statusLife > 0) {
        statusLine = hbox({
            text("  "),
            text(status) | color(Color::Magenta) | flex,
        });
    } else {
        statusLine = hbox({
            text("  "),
            text("Press [?] for help") | dim | flex,
        });
    }

    // ── Main content assembly ──
    auto inner = vbox({
        infoLine,
        separatorHeavy(),
        nowPlaying,
        separatorHeavy(),
        libHeader,
        separator(),
        playlist,
        separatorHeavy(),
        statusLine,
    }) | yflex;

    Element mainContent = window(
        text(" " ICON_MUSIC " Mozart ") | bold | color(Color::Cyan),
        inner
    );

    // ── Popup overlays ──
    Element overlay;

    if (showHelp) {
        auto hl = [](const char* k, const char* d) {
            return hbox({
                text(std::string("  ") + k) | bold,
                text("  " + std::string(d)),
                filler(),
            });
        };
        auto hc = vbox({
            text(""),
            hl("Space",     "Play / Pause"),
            hl("Enter",     "Play selected track"),
            hl("n / p",     "Next / Previous track"),
            hl("j / k",     "Navigate playlist"),
            hl("+ / -",     "Volume up / down"),
            hl("Left/Right","Seek -5s / +5s"),
            hl("g / G",     "Jump to top / bottom"),
            hl("PgUp/Dn",   "Page up / down"),
            hl("s",         "Toggle shuffle"),
            hl("r",         "Cycle repeat mode"),
            hl("f / *",     "Toggle star"),
            hl("Tab",       "Filter starred only"),
            hl("/",         "Search library"),
            hl("a",         "Add directory"),
            hl("d",         "Delete track"),
            hl("?",         "Toggle this help"),
            hl("q",         "Quit"),
            text(""),
            hbox({filler(), text("[ ? / Esc to close ]") | dim, filler()}),
            text(""),
        });

        overlay = window(
            text(" \uf128 Help ") | bold | color(Color::Yellow),
            hc | vscroll_indicator | yframe
        ) | clear_under | center;
    } else if (showVolumePopup) {
        auto vc = vbox({
            text(""),
            hbox({filler(), text(std::string(ICON_VOL_UP) + "  " + std::to_string(vol) + "%") | bold, filler()}),
            hbox({filler(), gauge((float)vol / 100.0f) | color(Color::Green) | size(WIDTH, EQUAL, 20), filler()}),
            text(""),
        });

        overlay = window(
            text(" " ICON_VOL_UP " Volume ") | bold | color(Color::Yellow),
            vc | center
        ) | clear_under | center;
    } else {
        overlay = text("");
    }

    bool popupActive = showHelp || showVolumePopup;
    Elements dboxArgs;
    dboxArgs.push_back(popupActive ? mainContent | dim : mainContent);
    dboxArgs.push_back(overlay);
    return dbox(std::move(dboxArgs));
}
