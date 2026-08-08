#include "mpris.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>

Mpris::Mpris(Player& player, bool& shuffle, int& repeat, int& volume)
    : m_player(player)
    , m_shuffle(shuffle)
    , m_repeat(repeat)
    , m_volume(volume)
{
#ifdef MPRIS_ENABLED
    m_server = mpris::Server::make("mozart");
    if (!m_server)
        return;

    m_server->set_identity("Mozart");
    m_server->set_desktop_entry("mozart");
    m_server->set_supported_uri_schemes({"file"});
    m_server->set_supported_mime_types({
        "audio/mpeg", "audio/ogg", "audio/flac", "audio/x-flac",
        "audio/wav", "audio/x-wav", "audio/mp4", "audio/aac",
        "audio/x-m4a", "audio/opus", "audio/webm", "audio/x-matroska",
    });
    m_server->set_playback_status(mpris::PlaybackStatus::Stopped);

    m_server->on_next([this] { m_player.next(); });
    m_server->on_previous([this] { m_player.prev(); });
    m_server->on_pause([this] { m_player.pause(); });
    m_server->on_play([this] { m_player.play(); });
    m_server->on_play_pause([this] { m_player.toggle(); });
    m_server->on_stop([this] {
        m_player.pause();
        m_player.seekTo(0);
    });
    m_server->on_seek([this](int64_t us) {
        m_player.seekTo(m_player.position() + us / 1000);
        m_server->send_seeked_signal(m_player.position() * 1000);
    });
    m_server->on_set_position([this](int64_t us) {
        m_player.seekTo(us / 1000);
        m_server->send_seeked_signal(us);
    });
    m_server->on_loop_status_changed([this](mpris::LoopStatus s) {
        int r = s == mpris::LoopStatus::None   ? 0
              : s == mpris::LoopStatus::Track  ? 1
                                               : 2;
        m_player.setRepeatMode(r);
        m_repeat = r;
    });
    m_server->on_shuffle_changed([this](bool s) {
        m_player.setShuffle(s);
        m_shuffle = s;
    });
    m_server->on_volume_changed([this](double v) {
        int vol = std::clamp((int)std::lround(v * 100), 0, 100);
        m_player.setVolume(vol);
        m_volume = vol;
    });

    m_server->start_loop_async();
#endif
}

Mpris::~Mpris() = default;

void Mpris::pushMetadata() {
#ifdef MPRIS_ENABLED
    int idx = m_player.currentIndex();
    if (idx < 0 || idx >= (int)m_player.playlist().size()) {
        m_server->set_metadata({});
        return;
    }
    std::map<mpris::Field, sdbus::Variant> md;
    md[mpris::Field::TrackId] = sdbus::Variant(sdbus::ObjectPath{
        "/org/mpris/MediaPlayer2/Track/" + std::to_string(idx)});
    md[mpris::Field::Title] = sdbus::Variant(m_player.currentTitle());
    md[mpris::Field::Url] = sdbus::Variant("file://" + m_player.playlist()[idx]);
    int64_t len = m_player.duration();
    if (len > 0)
        md[mpris::Field::Length] = sdbus::Variant(len * 1000);
    m_server->set_metadata(md);
#endif
}

void Mpris::update() {
#ifdef MPRIS_ENABLED
    if (!m_server)
        return;

    mpris::PlaybackStatus status = mpris::PlaybackStatus::Stopped;
    if (m_player.isPlaying())
        status = mpris::PlaybackStatus::Playing;
    else if (m_player.isPaused())
        status = mpris::PlaybackStatus::Paused;
    if (status != m_status) {
        m_status = status;
        m_server->set_playback_status(status);
    }

    int track = m_player.currentIndex();
    if (track != m_track || m_player.currentTitle() != m_title) {
        m_track = track;
        m_title = m_player.currentTitle();
        pushMetadata();
    }

    if (m_player.volume() != m_vol) {
        m_vol = m_player.volume();
        m_server->set_volume(m_vol / 100.0);
    }

    if (m_repeat != m_rep) {
        m_rep = m_repeat;
        m_server->set_loop_status(m_rep == 1 ? mpris::LoopStatus::Track
                                  : m_rep == 2 ? mpris::LoopStatus::Playlist
                                               : mpris::LoopStatus::None);
    }

    if (m_shuffle != m_shuf) {
        m_shuf = m_shuffle;
        m_server->set_shuffle(m_shuffle);
    }

    m_server->set_position(m_player.position() * 1000);
#endif
}
