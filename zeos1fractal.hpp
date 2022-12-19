#pragma once

#include <map>
#include <vector>
#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

enum proposal_status
{
    PS_PROPOSED = 0,
    PS_UNDER_REVIEW = 1,
    PS_APPROVED = 2,
    PS_DENIED = 3,
};

enum contract_state
{
    CS_IDLE = 0,
    // Event states
    CS_INTRODUCTION = 1,
    CS_BREAKOUT_ROOMS = 2,
    CS_COUNCIL_MEETING = 3,
};

CONTRACT zeos1fractal
{
    TABLE member
    {
        name user;                  // EOS account name
        map<string, string> links;  // for instance: twitter.com => @mschoenebeck1 or SSH => ssh-rsa AAAAB3NzaC1yc2E...
        uint64_t zeos_earned;       // total amount of ZEOS received
        uint64_t respect_earned;    // total amount of ZEOS Respect received
        bool accept_moderator;      // accept moderator role?
        bool accept_delegate;       // accept delegate vote?
        bool has_been_delegate;     // has ever been delegate?
        bool is_banned;             // is user banned from fractal?

        uint64_t primary_key() const { return user.value; }
    };
    typedef eosio::multi_index<"members"_n, member> members_t;

    // resets with every weekly event
    TABLE introduction
    {
        name user;
        uint64_t seconds;
    };

    // VRAM - ever expanding (can only add/modify items)
    TABLE proposal
    {
        uint64_t id;
        name user;
        string caption;
        string description;
        string ipfs;
        proposal_status status;
    };

    // ongoing vote (like EOS BP voting)
    TABLE moderator_ranking
    {
        name user;
        vector<name> ranking;
    };

    // resets with every event
    TABLE introduction_ranking
    {
        name user;
        vector<name> ranking;
    };

    // resets with every event
    TABLE proposal_ranking
    {
        name user;
        vector<uint64_t> ranking;
    };

    // resets with every event
    TABLE joined
    {
        name user;
    };

    // Singleton - Global stats
    TABLE global
    {
        contract_state state;
        uint64_t proposal_count;
        uint64_t event_count;
        vector<name> moderator_ranking;
        
        // Event information
        // Resets every time when a new event is created by 'admin'
        uint64_t next_event_block_height;
        vector<name> introduction_ranking;
        vector<uint64_t> proposal_ranking;

        // Settings/Configuration
        map<name, uint64_t> respect_level;
        uint64_t introduction_duration;
        uint64_t breakout_room_duration;
        uint64_t council_meeting_duration;
    };

    /// Creates a new member account for this fractal (allocates RAM)
    ACTION signup(const name& user);

    /// Joins the current event
    ACTION join(const name& user);
    
    /// Adds link to (social media) profile on other platform. Remains forever in chain state.
    ACTION addlink(
        const name& user,
        const string& key,
        const string& value
    );

    /// Set to 'true' to accept moderator role
    ACTION setacceptmod(
        const name& user,
        const bool& value
    );

    // Set to 'true' to accept delegate vote
    ACTION setacceptdel(
        const name& user,
        const bool& value
    );

    ACTION setintro(
        const name& user,
        const uint64_t& seconds
    );

    // TODO: add/update proposal actions

};