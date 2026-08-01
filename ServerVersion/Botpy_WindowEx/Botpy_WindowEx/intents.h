#pragma once

#include <cstdint>

class Intents {
public:
    bool guilds;
    bool guild_members;
    bool guild_messages;
    bool guild_message_reactions;
    bool direct_message;
    bool open_forum_event;
    bool audio_or_live_channel_member;
    bool public_messages;
    bool interaction;
    bool message_audit;
    bool forums;
    bool audio_action;
    bool public_guild_messages;

    Intents() :
        guilds(false), guild_members(false),
        guild_messages(false), guild_message_reactions(false),
        direct_message(false),
        open_forum_event(false), audio_or_live_channel_member(false),
        public_messages(false),
        interaction(false), message_audit(false),
        forums(false), audio_action(false),
        public_guild_messages(false) {}

    Intents(bool pm, bool dm) :
        guilds(false), guild_members(false),
        guild_messages(false), guild_message_reactions(false),
        direct_message(dm),
        open_forum_event(false), audio_or_live_channel_member(false),
        public_messages(pm),
        interaction(false), message_audit(false),
        forums(false), audio_action(false),
        public_guild_messages(false) {}

    uint32_t to_bits() const;
};
