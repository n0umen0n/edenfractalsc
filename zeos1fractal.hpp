#pragma once

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <map>
#include <vector>

using namespace std;
using namespace eosio;

// proposal status
#define PS_PROPOSED 0
#define PS_UNDER_REVIEW 1
#define PS_APPROVED 2
#define PS_DENIED 3

// contract states
#define CS_IDLE 0
#define CS_INTRODUCTION 1
#define CS_BREAKOUT_ROOMS 2
#define CS_COUNCIL_MEETING 3

CONTRACT zeos1fractal : contract {
public:
  TABLE userspropvote {
    name user;
    vector<uint64_t> ids;
    vector<uint8_t> option;

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"usrspropvote"_n, usrspropvote> usrpropvote_t;

  TABLE currency_stats {
    asset supply;
    asset max_supply;
    name issuer;

    uint64_t primary_key() const { return supply.symbol.code().raw(); }
  };

  typedef eosio::multi_index<name("stat"), currency_stats> stats;

  TABLE account {
    asset balance;

    uint64_t primary_key() const { return balance.symbol.code().raw(); }
  };

  typedef eosio::multi_index<name("accounts"), account> accounts;

  TABLE member {
    name user;                 // EOS account name
    map<string, string> links; // for instance: twitter.com => @mschoenebeck1 or
                               // SSH => ssh-rsa AAAAB3NzaC1yc2E...
    uint64_t zeos_earned;      // total amount of ZEOS received
    uint64_t respect_earned;   // total amount of ZEOS Respect received
    bool accept_moderator;     // accept moderator role?
    bool accept_delegate;      // accept delegate vote?
    bool has_been_delegate;    // has ever been delegate?
    bool is_banned;            // is user banned from fractal?

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"members"_n, member> members_t;

  // resets with every weekly event
  TABLE introduction {
    name user;
    uint64_t num_blocks; // duration of the introduction

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"intros"_n, introduction> intros_t;

  // VRAM - ever expanding (can only add/modify items)
  TABLE proposal {
    uint64_t id;
    name user;
    string question;
    vector<uint64_t> votedforoption;
    vector<string> answers;
    uint64_t totaltokens;
    string description;
    string ipfs;
    uint64_t status;

    uint64_t primary_key() const { return id; }
  };
  // TODO: change to VRAM
  typedef eosio::multi_index<"proposals"_n, proposal> proposals_t;

  // ongoing vote (like EOS BP voting)
  TABLE moderator_ranking {
    name user;
    vector<name> ranking;

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"modranks"_n, moderator_ranking> modranks_t;

  // resets with every event
  TABLE introduction_ranking {
    name user;
    vector<name> ranking;

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"introranks"_n, introduction_ranking> introranks_t;

  // resets with every event
  TABLE proposal_ranking {
    name user;
    vector<uint64_t> ranking;

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"propranks"_n, proposal_ranking> propranks_t;

  // resets with every event
  TABLE joined {
    name user;

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"joins"_n, joined> joins_t;

  // Singleton - Global stats
  TABLE global {
    uint64_t state;
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
    uint64_t early_join_duration;
  };
  eosio::singleton<"global"_n, global> _global;

  zeos1fractal(name self, name code, datastream<const char *> ds);

  /// Clears all tables of the smart contract (useful if table schema needs
  /// update)
  ACTION cleartables();

  /// Initializes the contract and resets all tables
  ACTION init();

  /// Creates a new member account for this fractal (allocates RAM)
  ACTION signup(const name &user);

  /// Joins the current event
  ACTION join(const name &user);

  /// Adds link to (social media) profile on other platform. Remains forever in
  /// chain state.
  ACTION addlink(const name &user, const string &key, const string &value);

  /// Set to 'true' to accept moderator role
  ACTION setacceptmod(const name &user, const bool &value);

  // Set to 'true' to accept delegate vote
  ACTION setacceptdel(const name &user, const bool &value);

  ACTION setintro(const name &user, const uint64_t &seconds);

  // TODO: add/update proposal actions

  ACTION setevent(const uint64_t &block_height);

  ACTION changestate();
};