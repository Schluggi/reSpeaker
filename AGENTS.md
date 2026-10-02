# AGENTS.md

Guidance for agents working in this repository. Keep this file current when
architecture, packages, components, playback, or commands change — update it in
the same change.

## What this repository is

ESPHome packages and external components that turn a **Respeaker XVF3800 USB
4-Mic Array** (ESP32-S3 + XMOS XVF3800 DSP) into a Home Assistant voice
satellite. There is no app to run here. Consumers pull packages over
`packages:` and components over `external_components`.

There is **no local speaker**. TTS and notification sounds play on a Home
Assistant `media_player` (`${external_speaker}`).

## Layout

- `packages/` — source of truth for device config:
  - `base.yaml` — core, network, api, external_components
  - `hardware.yaml` — I2S mic, DSP, DFU
  - `voice-assistant.yaml` — mWW, pipeline, external speaker playback
  - `leds.yaml` — LED effect scripts
  - `timers-alarm.yaml` — time, alarm, timers
- `esphome/components/respeaker_xvf3800/` — I2C hub for the XMOS DSP
- `esphome/components/external_announce/` — silent media player that forwards
  announcement URLs to Home Assistant
- `esphome/components/aic3104/` — codec DAC platform (packages leave it unloaded)
- `config/respeaker-xvf-satellite-example.yaml` — thin consumer example
- `ha/` — Home Assistant script snippet for `set_led_color`

## Commands

No tests, linters, or CI. Use the ESPHome CLI:

```bash
esphome config config/respeaker-xvf-satellite-example.yaml
esphome compile config/respeaker-xvf-satellite-example.yaml
esphome run config/respeaker-xvf-satellite-example.yaml
esphome logs config/respeaker-xvf-satellite-example.yaml
esphome clean config/respeaker-xvf-satellite-example.yaml
```

`esphome config` validates YAML and component schemas. `esphome compile` is the
only check for C++.

Secrets live next to the example config (`config/secrets.yaml.example`). Keep
local-only files in `local/` (gitignored).

### Testing before push

Packages and `external_components` resolve from GitHub `@main`, so a fresh
checkout builds published code, not the working tree.

**Package edits** — local harness under `local/` (gitignored):

```yaml
packages:
  base: !include ../packages/base.yaml
  hardware: !include ../packages/hardware.yaml
  voice_assistant: !include ../packages/voice-assistant.yaml
  leds: !include ../packages/leds.yaml
  timers_alarm: !include ../packages/timers-alarm.yaml
```

Then `esphome config local/test-merge.yaml`.

**C++ edits** — temporarily switch `external_components` in `packages/base.yaml`
to `source: {type: local, path: esphome/components}`. Do not commit that switch.

`${respeaker_ref}` drives packages, C++ components, and the DFU firmware URL.
Keep them in lockstep.

The config pins a fork of ESPHome `i2s_audio`
(`formatBCE/esphome@respeaker_microphone`). Stock `i2s_audio` does not work on
this board.

## External speaker playback

```
voice_assistant.media_player
  → external_announce_player (platform: external_announce)
  → on_announcement → play_on_external_speaker
  → homeassistant.action media_player.play_media on ${external_speaker}
```

While `external_announce_player` is `ANNOUNCING`, the Assist satellite stays
Responding. `finish()` (from HA player idle/off/paused/standby, or watchdogs)
moves it back to Idle and sends AnnounceFinished.

Hard constraints — do not break either without an intentional replacement:

1. **`announce: "true"`** on `play_on_external_speaker` (string, not YAML bool).
   ESPHome sends action data as strings. Without the flag, the player replaces
   current media and music stops. With it, players that support announcements
   duck music (Music Assistant: Sonos S2, Sendspin, Snapcast).
2. **URL dedupe** in `ExternalAnnounceMediaPlayer::control()`: while
   `ANNOUNCING`, ignore another play URL that is the same as the current one, or
   any non-`ENQUEUE` play. A different `ENQUEUE` URL (preannounce → announcement)
   still starts. Without dedupe, streaming TTS plus TTS_END (and announce) speak
   the reply more than once. `play_on_external_speaker` `mode: restart` does not
   cancel a Home Assistant call that already left the device.
3. **Wake start** uses script `start_voice_assistant_from_wake` with
   `mode: single`. `stop_after_detection: false` can fire twice; without single
   mode each detection sends the wake chime again.
4. **Same-URL debounce** in `play_on_external_speaker`: while
   `external_playback_active` and the URL equals `external_playback_url`, do not
   call Home Assistant again. Clear `external_playback_url` when playback ends
   or is stopped.

Wake sounds and timers share the playback scripts; `finish()` is a no-op unless
the player is announcing, so those paths do not clear Responding incorrectly.

## DSP hub and LEDs

`RespeakerXVF3800` is an I2C hub at `0x2C`. Optional children
(`MuteSwitch`, `DFUVersionTextSensor`, `LEDBeamSensor`) hold a back-pointer and
call hub methods from `update()`. Add entities the same way.

XMOS access is `(resid, cmd, payload)` over I2C. `CTRL_WAIT` and
`SERVICER_COMMAND_RETRY` (`0x40`) are normal and must be retried, not logged as
errors.

LED animation lives in `packages/leds.yaml` only. The `LED Ring` light drives
globals for the animation engine; it does not write the ring directly. Beam
lock is opt-in via a template switch; lock on wake, unlock after STT VAD end /
pipeline end.

## Related docs

- `README.md` — user-facing install and behaviour
- `CLAUDE.md` — Claude Code notes (may overlap; this file is the Cursor source of truth)
