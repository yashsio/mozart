#include "splash.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace ftxui;

static const std::vector<std::string> quotes = {
    "Can you hear the music?",
    "Why is this world so silent?  ── Beethoven",
    "Music is the shorthand of emotion.",
    "Feeling the rhythm.",
    "Music can change the world.",
    "Listener is Listened.",
    "Music is a great way of relaxing.",
    "That's music to my ears.",
    "There is no genius without a touch of madness.",
    "Its time for the staccato rhythm.",
};

static const std::string& pickQuote() {
    static bool seeded = false;
    if (!seeded) {
        std::srand(std::time(nullptr));
        seeded = true;
    }
    static int idx = std::rand() % quotes.size();
    return quotes[idx];
}

Element buildSplash(int width) {
    std::string line = pickQuote();
    int maxW = std::max(10, width - 34);
    if ((int)line.size() > maxW) {
        while ((int)line.size() > maxW - 3) line.pop_back();
        line += "...";
    }
    return hbox({text("  "), text(line) | dim});
}
