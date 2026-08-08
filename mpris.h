#pragma once
#include "player.h"
#include <memory>
#include <string>

#ifdef MPRIS_ENABLED
#include "mpris_server.hpp"
#else
#define MPRIS_SERVER_NO_IMPL
#include "mpris_server.hpp"
#endif

class Mpris {
public:
    Mpris(Player& player, bool& shuffle, int& repeat, int& volume);
    ~Mpris();
    void update();

private:
    void pushMetadata();

    Player& m_player;
    bool& m_shuffle;
    int& m_repeat;
    int& m_volume;
    std::unique_ptr<mpris::Server> m_server;
    mpris::PlaybackStatus m_status = mpris::PlaybackStatus::Stopped;
    int m_track = -2;
    std::string m_title;
    int m_rep = -1;
    bool m_shuf = false;
    int m_vol = -1;
};
