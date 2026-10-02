#include "external_announce_media_player.h"

#include "esphome/core/log.h"

namespace esphome {
namespace external_announce {

static const char *const TAG = "external_announce";

void ExternalAnnounceMediaPlayer::setup() { this->state = media_player::MEDIA_PLAYER_STATE_IDLE; }

void ExternalAnnounceMediaPlayer::dump_config() { ESP_LOGCONFIG(TAG, "External Announce Media Player"); }

void ExternalAnnounceMediaPlayer::finish() {
  if (this->state != media_player::MEDIA_PLAYER_STATE_ANNOUNCING)
    return;
  ESP_LOGD(TAG, "Announcement finished");
  this->state = media_player::MEDIA_PLAYER_STATE_IDLE;
  // voice_assistant observes this transition and sends AnnounceFinished.
  this->publish_state();
}

void ExternalAnnounceMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (call.get_command().has_value() &&
      call.get_command().value() == media_player::MEDIA_PLAYER_COMMAND_STOP) {
    for (auto *trigger : this->stop_triggers_)
      trigger->trigger();
    this->finish();
    return;
  }

  if (!call.get_media_url().has_value())
    return;

  const std::string &url = call.get_media_url().value();
  const bool enqueue = call.get_command().has_value() &&
                       call.get_command().value() == media_player::MEDIA_PLAYER_COMMAND_ENQUEUE;
  // Streaming TTS and the final file are the same sentence. A second play
  // while this one is running would speak it again. ENQUEUE is the message
  // after a preannounce, so a different URL still starts.
  if (this->state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING && (url == this->url_ || !enqueue)) {
    ESP_LOGD(TAG, "Ignoring duplicate announcement");
    // The caller may have reset the response to URL_SENT before perform().
    // Publishing ANNOUNCING again moves that back to PLAYING, so the satellite
    // stays in Responding for the playback that is already running.
    this->publish_state();
    return;
  }

  // voice_assistant sets the response state to URL_SENT before perform().
  // Publishing ANNOUNCING here, before the playback script runs, is what moves
  // that state to PLAYING. finish() afterwards is then a real end, not a start
  // the component never saw.
  this->url_ = url;
  ESP_LOGD(TAG, "Announcement: %s", this->url_.c_str());
  this->state = media_player::MEDIA_PLAYER_STATE_ANNOUNCING;
  this->publish_state();
  for (auto *trigger : this->announcement_triggers_)
    trigger->trigger(this->url_);
}

}  // namespace external_announce
}  // namespace esphome
