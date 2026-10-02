#pragma once

#include "esphome/components/media_player/media_player.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#include <string>
#include <vector>

namespace esphome {
namespace external_announce {

class ExternalAnnounceMediaPlayer : public Component, public media_player::MediaPlayer {
 public:
  void setup() override;
  void dump_config() override;

  media_player::MediaPlayerTraits get_traits() override { return {}; }

  // Ends an announcement. No effect unless one is in progress, so wake sounds
  // and timers can share the playback scripts without touching the satellite.
  void finish();

  void add_announcement_trigger(Trigger<std::string> *trigger) { this->announcement_triggers_.push_back(trigger); }
  void add_stop_trigger(Trigger<> *trigger) { this->stop_triggers_.push_back(trigger); }

 protected:
  void control(const media_player::MediaPlayerCall &call) override;

  std::string url_;
  std::vector<Trigger<std::string> *> announcement_triggers_;
  std::vector<Trigger<> *> stop_triggers_;
};

class AnnouncementTrigger : public Trigger<std::string> {
 public:
  explicit AnnouncementTrigger(ExternalAnnounceMediaPlayer *parent) { parent->add_announcement_trigger(this); }
};

class StopTrigger : public Trigger<> {
 public:
  explicit StopTrigger(ExternalAnnounceMediaPlayer *parent) { parent->add_stop_trigger(this); }
};

}  // namespace external_announce
}  // namespace esphome
