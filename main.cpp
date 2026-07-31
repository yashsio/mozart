#include "player.h"
#include "ui.h"
#include "config.h"
#include "util.h"
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <locale.h>
#include <csignal>
#include <thread>
#include <atomic>
#include <algorithm>
#include <set>
#include <vector>
#include <string>
#include <filesystem>
#include <sys/ioctl.h>
#include <unistd.h>

namespace fs = std::filesystem;

static std::atomic<bool> g_quitSig{false};
extern "C" void onSignal(int) { g_quitSig = true; }

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);

    int volume = 70;
    bool shuffle = false;
    int repeat = 0;
    int lastTrack = -1;
    int64_t lastPos = 0;
    bool showStarredOnly = false;

    auto files = loadLib();
    auto starred = loadStars();
    loadCfg(volume, shuffle, repeat, lastTrack, lastPos, showStarredOnly);

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (fs::is_directory(arg)) {
            scanDir(arg, files);
        } else if (fs::is_regular_file(arg) && isAudio(fs::path(arg).extension().string())) {
            if (std::find(files.begin(), files.end(), arg) == files.end())
                files.push_back(arg);
        }
    }

    Player player;
    if (!files.empty()) {
        player.load(files);
        player.setVolume(volume);
        player.setShuffle(shuffle);
        player.setRepeatMode(repeat);
        if (lastTrack >= 0 && lastTrack < (int)files.size()) {
            player.playIndex(lastTrack);
            if (lastPos > 0) player.seekTo(lastPos);
        } else if (player.songCount() > 0) {
            player.playIndex(0);
        }
    }

    int sel = std::clamp(lastTrack >= 0 ? lastTrack : 0, 0, std::max(0, (int)files.size() - 1));
    bool searchMode = false;
    std::string searchQuery;
    bool inputMode = false;
    std::string inputBuffer;
    std::string status;
    int statusLife = 0;
    int prevTrack = -2;
    int autoSaveTicker = 0;
    bool showHelp = false;
    int volumePopupLife = 0;

    auto isVis = [&](int i) -> bool {
        if (i < 0 || i >= (int)files.size()) return false;
        if (showStarredOnly && !starred.count(files[i])) return false;
        if (!matchesQuery(files[i], searchQuery)) return false;
        return true;
    };

    auto nextVis = [&](int from, int dir) -> int {
        if (files.empty()) return 0;
        if (dir == 0) {
            if (isVis(from)) return from;
            for (int i = from + 1; i < (int)files.size(); i++)
                if (isVis(i)) return i;
            for (int i = from - 1; i >= 0; i--)
                if (isVis(i)) return i;
            return from;
        }
        int i = from + dir;
        while (i >= 0 && i < (int)files.size()) {
            if (isVis(i)) return i;
            i += dir;
        }
        return from;
    };

    auto clampSel = [&]() {
        if (files.empty()) { sel = 0; return; }
        int next = nextVis(sel, 0);
        if (!isVis(next)) {
            int first = nextVis(0, 1);
            sel = isVis(first) ? first : 0;
        } else {
            sel = next;
        }
    };

    auto screen = ftxui::ScreenInteractive::Fullscreen();

    auto renderer = ftxui::Renderer([&] {
        player.pollAdvance();

        int curTrack = player.currentIndex();
        if (curTrack != prevTrack && curTrack >= 0 && prevTrack >= -1) {
            std::string name = stripExt(files[curTrack]);
            int maxW = 50;
            if ((int)name.size() > maxW) name = name.substr(0, maxW - 3) + "...";
            status = std::string(ICON_MUSIC) + " " + name;
            statusLife = 25;
        }
        prevTrack = curTrack;

        if (statusLife > 0) statusLife--;
        if (volumePopupLife > 0) volumePopupLife--;

        autoSaveTicker++;
        if (autoSaveTicker % 300 == 0) {
            int64_t pos = (player.isPlaying() || player.isPaused()) ? player.position() : 0;
            saveCfg(volume, shuffle, repeat, showStarredOnly, curTrack, pos);
            saveStars(starred);
            saveLib(files);
        }

        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        int cols = ws.ws_col > 0 ? ws.ws_col : 80;
        int rows = ws.ws_row > 0 ? ws.ws_row : 24;

        int starCount = (int)starred.size();

        return buildUI(player, files, starred, sel, showStarredOnly,
                       searchQuery, searchMode, inputMode, inputBuffer,
                       status, statusLife, shuffle, repeat,
                       starCount, cols, rows,
                       showHelp, volumePopupLife > 0);
    });

    renderer |= ftxui::CatchEvent([&](ftxui::Event e) {
        if (g_quitSig) {
            screen.ExitLoopClosure()();
            return true;
        }

        if (showHelp) {
            if (e == ftxui::Event::Escape || e == ftxui::Event::Character('?')) {
                showHelp = false;
            }
            return true;
        }

        if (inputMode) {
            if (e == ftxui::Event::Escape) {
                inputMode = false;
                inputBuffer.clear();
            } else if (e == ftxui::Event::Return) {
                inputMode = false;
                std::string p = inputBuffer;
                inputBuffer.clear();
                if (!p.empty()) {
                    if (p[0] == '~') {
                        const char* h = getenv("HOME");
                        if (h) p = std::string(h) + p.substr(1);
                    }
                    {
                        std::error_code ec;
                        auto abs = fs::absolute(p, ec);
                        if (!ec) p = abs.string();
                    }
                    if (fs::is_directory(p)) {
                        int before = (int)files.size();
                        scanDir(p, files);
                        if ((int)files.size() > before) {
                            player.setFiles(files);
                            clampSel();
                            status = std::string(ICON_FOLDER) + " Added";
                            statusLife = 30;
                        } else {
                            status = "No new audio files";
                            statusLife = 20;
                        }
                    } else {
                        status = "Not a directory";
                        statusLife = 20;
                    }
                }
            } else if (e == ftxui::Event::Backspace && !inputBuffer.empty()) {
                inputBuffer.pop_back();
            } else if (e.is_character()) {
                inputBuffer += e.character();
            }
            return true;
        }

        if (searchMode) {
            if (e == ftxui::Event::Escape) {
                searchMode = false;
                searchQuery.clear();
            } else if (e == ftxui::Event::Return) {
                searchMode = false;
            } else if (e == ftxui::Event::Backspace && !searchQuery.empty()) {
                searchQuery.pop_back();
            } else if (e.is_character()) {
                searchQuery += e.character();
            }
            sel = nextVis(sel, 0);
            if (!isVis(sel)) sel = nextVis(0, 1);
            if (!isVis(sel)) sel = 0;
            return true;
        }

        if (e == ftxui::Event::Character('?')) {
            showHelp = !showHelp;
            return true;
        }
        if (e == ftxui::Event::q || e == ftxui::Event::CtrlC) {
            screen.ExitLoopClosure()();
            return true;
        }
        if (e == ftxui::Event::Escape) {
            if (statusLife > 0) statusLife = 0;
            return true;
        }
        if (e == ftxui::Event::Character(' ')) {
            if (player.songCount() > 0) player.toggle();
            return true;
        }
        if (e == ftxui::Event::ArrowDown || e == ftxui::Event::j) {
            sel = nextVis(sel, 1);
            return true;
        }
        if (e == ftxui::Event::ArrowUp || e == ftxui::Event::k) {
            sel = nextVis(sel, -1);
            return true;
        }
        if (e == ftxui::Event::n) {
            if (player.songCount() > 0) { player.next(); status = std::string(ICON_NEXT) + " Next"; statusLife = 30; }
            return true;
        }
        if (e == ftxui::Event::p) {
            if (player.songCount() > 0) { player.prev(); status = std::string(ICON_PREV) + " Prev"; statusLife = 30; }
            return true;
        }
        if (e == ftxui::Event::s) {
            shuffle = !shuffle;
            player.setShuffle(shuffle);
            status = std::string(ICON_SHUFFLE) + " " + (shuffle ? "ON" : "OFF");
            statusLife = 30;
            return true;
        }
        if (e == ftxui::Event::r) {
            repeat = (repeat + 1) % 3;
            player.setRepeatMode(repeat);
            status = std::string(ICON_REPEAT) + " "
                + (repeat == 0 ? "OFF" : (repeat == 1 ? "ONE" : "ALL"));
            statusLife = 30;
            return true;
        }
        if (e == ftxui::Event::Character('+') || e == ftxui::Event::Character('=')) {
            volume = std::min(100, volume + 5);
            player.setVolume(volume);
            volumePopupLife = 25;
            statusLife = 0;
            return true;
        }
        if (e == ftxui::Event::Character('-')) {
            volume = std::max(0, volume - 5);
            player.setVolume(volume);
            volumePopupLife = 25;
            statusLife = 0;
            return true;
        }
        if (e == ftxui::Event::a) {
            inputMode = true;
            inputBuffer.clear();
            statusLife = 0;
            return true;
        }
        if (e == ftxui::Event::d) {
            if (sel >= 0 && sel < (int)files.size()) {
                bool wasCurrent = (sel == player.currentIndex());
                starred.erase(files[sel]);
                files.erase(files.begin() + sel);
                player.setFiles(files);
                if (wasCurrent && !files.empty()) {
                    int nextIdx = std::min(sel, (int)files.size() - 1);
                    player.playIndex(nextIdx);
                    sel = nextIdx;
                } else {
                    clampSel();
                }
                status = "Deleted";
                statusLife = 30;
            }
            return true;
        }
        if (e == ftxui::Event::Character('*') || e == ftxui::Event::f) {
            if (sel >= 0 && sel < (int)files.size()) {
                auto& path = files[sel];
                if (starred.count(path)) {
                    starred.erase(path);
                    status = "Unstarred";
                } else {
                    starred.insert(path);
                    status = std::string(ICON_STAR) + " Starred";
                }
                saveStars(starred);
                if (!isVis(sel)) clampSel();
                statusLife = 30;
            }
            return true;
        }
        if (e == ftxui::Event::Tab) {
            showStarredOnly = !showStarredOnly;
            clampSel();
            status = showStarredOnly ? "Starred only" : "All songs";
            statusLife = 25;
            return true;
        }
        if (e == ftxui::Event::Character('/')) {
            searchMode = true;
            searchQuery.clear();
            statusLife = 0;
            return true;
        }
        if (e == ftxui::Event::g) {
            sel = nextVis(0, 0);
            if (!isVis(sel)) sel = 0;
            return true;
        }
        if (e == ftxui::Event::G) {
            sel = nextVis((int)files.size() - 1, 0);
            if (!isVis(sel)) sel = std::max(0, (int)files.size() - 1);
            return true;
        }
        if (e == ftxui::Event::Return) {
            if (sel >= 0 && sel < (int)files.size()) {
                player.playIndex(sel);
                status = std::string(ICON_PLAY) + " Now playing";
                statusLife = 30;
            }
            return true;
        }
        if (e == ftxui::Event::ArrowLeft) {
            if (player.isPlaying())
                player.seekTo(std::max<int64_t>(0, player.position() - 5000));
            return true;
        }
        if (e == ftxui::Event::ArrowRight) {
            if (player.isPlaying())
                player.seekTo(player.position() + 5000);
            return true;
        }
        if (e == ftxui::Event::PageDown || e == ftxui::Event::CtrlD) {
            struct winsize ws;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
            int half = std::max(1, (ws.ws_row - 12) / 2);
            for (int i = 0; i < half; i++) sel = nextVis(sel, 1);
            return true;
        }
        if (e == ftxui::Event::PageUp || e == ftxui::Event::CtrlU) {
            struct winsize ws;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
            int half = std::max(1, (ws.ws_row - 12) / 2);
            for (int i = 0; i < half; i++) sel = nextVis(sel, -1);
            return true;
        }
        if (e == ftxui::Event::Home) {
            sel = nextVis(0, 0);
            if (!isVis(sel)) sel = 0;
            return true;
        }
        if (e == ftxui::Event::End) {
            sel = nextVis((int)files.size() - 1, 0);
            if (!isVis(sel)) sel = std::max(0, (int)files.size() - 1);
            return true;
        }

        return false;
    });

    std::atomic<bool> done{false};
    std::thread refreshThread([&] {
        while (!done && !g_quitSig) {
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
            if (done || g_quitSig) break;
            screen.PostEvent(ftxui::Event::Custom);
        }
    });

    screen.Loop(renderer);

    done = true;
    refreshThread.join();

    int64_t finalPos = (player.isPlaying() || player.isPaused()) ? player.position() : 0;
    saveCfg(volume, shuffle, repeat, showStarredOnly, player.currentIndex(), finalPos);
    saveStars(starred);
    saveLib(files);

    return 0;
}
