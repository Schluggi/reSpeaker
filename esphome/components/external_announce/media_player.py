from esphome import automation
import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID

CONF_ON_ANNOUNCEMENT = "on_announcement"
CONF_ON_STOP = "on_stop"

external_announce_ns = cg.esphome_ns.namespace("external_announce")
ExternalAnnounceMediaPlayer = external_announce_ns.class_(
    "ExternalAnnounceMediaPlayer", cg.Component, media_player.MediaPlayer
)
AnnouncementTrigger = external_announce_ns.class_(
    "AnnouncementTrigger", automation.Trigger.template(cg.std_string)
)
StopTrigger = external_announce_ns.class_("StopTrigger", automation.Trigger.template())

CONFIG_SCHEMA = (
    media_player.media_player_schema(ExternalAnnounceMediaPlayer)
    .extend(cv.COMPONENT_SCHEMA)
    .extend(
        {
            cv.Optional(CONF_ON_ANNOUNCEMENT): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(AnnouncementTrigger),
                }
            ),
            cv.Optional(CONF_ON_STOP): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StopTrigger),
                }
            ),
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    # The stock on_announcement callback has no URL argument. Playing from it
    # and from this trigger would start the same reply twice.
    announcement_automations = config.pop(CONF_ON_ANNOUNCEMENT, [])
    await media_player.register_media_player(var, config)
    for conf in announcement_automations:
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)
    for conf in config.get(CONF_ON_STOP, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
