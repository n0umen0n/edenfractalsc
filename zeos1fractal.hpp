#pragma once

#include <cmath>
// #include <crypto.h>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
// #include <eosiolib/crypto.h>
#include <map>
#include <string>
#include <vector>

/*
using namespace eosio;
using std::string;
using std::vector;
*/
using namespace std;
using namespace eosio;

constexpr std::string_view rezpect_ticker{"REZPECT"};
constexpr symbol zeos_symbol{"ZEOS", 4};
constexpr symbol rezpect_symbol{rezpect_ticker, 4};

// namespace zeos1fractal {
/*
constexpr std::string_view rezpect_ticker{"REZPECT"};
constexpr symbol zeos_symbol{"ZEOS", 4};
constexpr symbol rezpect_symbol{rezpect_ticker, 4};
*/
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
  struct GroupRanking {
    std::vector<eosio::name> ranking;
  };
  struct AllRankings {
    std::vector<GroupRanking> allRankings;
  };

  TABLE consensus {
    std::vector<eosio::name> rankings;
    uint64_t groupnr;
    eosio::name submitter;

    uint64_t primary_key() const { return submitter.value; }

    uint64_t by_secondary() const { return groupNr; }
  };

  typedef eosio::multi_index<
      "conensus"_n, consensus,
      eosio::indexed_by<
          "bygroupnr"_n,
          eosio::const_mem_fun<consensus, uint64_t, &consensus::by_secondary>>>
      consensus_t;

  TABLE delegates {
    uint64_t groupnr;
    eosio::name elector;
    eosio::name delegate;

    uint64_t primary_key() const { return elector.value; }
  };
  typedef eosio::multi_index<"delegates"_n, delegates> delegates_t;

  TABLE rewardconfig {
    int64_t zeos_reward_amt;
    uint8_t fib_offset;
  };
  typedef eosio::singleton<"zeosrew"_n, rewardconfig> zeosrew_t;

  TABLE electioninf { uint64_t electionnr; };
  typedef eosio::singleton<"electinf"_n, electioninf> electinf_t;

  TABLE usrspropvote {
    name user;
    vector<uint64_t> ids;
    vector<uint8_t> option;

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"usrspropvote"_n, usrspropvote> propvote_t;

  TABLE usrsintrvote {
    name user;
    vector<uint64_t> ids;

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"usrsintrvote"_n, usrsintrvote> intrvote_t;

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

  /*

    // resets with every weekly event
    TABLE introduction {
      name user;
      uint64_t num_blocks; // duration of the introduction

      uint64_t primary_key() const { return user.value; }
    };
    typedef eosio::multi_index<"intros"_n, introduction> intros_t;
  */
  // VRAM - ever expanding (can only add/modify items)
  TABLE proposal {
    uint64_t id;
    name user;
    string question;
    vector<uint64_t> votedforoption;
    // vector<string> answers; SINCE only YES No then no need.
    uint64_t totaltokens;
    string description;
    uint64_t status;

    uint64_t primary_key() const { return id; }
  };
  // TODO: change to VRAM
  typedef eosio::multi_index<"proposals"_n, proposal> proposals_t;

  TABLE intros {
    uint64_t id;
    name user;
    string topic;
    string description;
    uint64_t totaltokens;

    uint64_t primary_key() const { return id; }
  };
  // TODO: change to VRAM
  typedef eosio::multi_index<"intros"_n, intros> intros_t;

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

  TABLE group {
    uint64_t id;
    std::vector<name> users;

    uint64_t primary_key() const { return id; }
  };
  typedef eosio::multi_index<"groups"_n, group> groups_t;

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

  ACTION electdeleg(const name &elector, const name &delegate,
                    const uint64_t &groupnr);

  ACTION submitcons(const uint64_t &groupnr, const std::vector<name> &rankings,
                    const name &submitter);

  ACTION zeosreward(const asset &quantity, const uint8_t &offset);

  ACTION transfer(const name &from, const name &to, const asset &quantity,
                  const string &memo);

  // standard eosio.token action to issue tokens
  ACTION issue(const name &to, const asset &quantity, const string &memo);

  // distributes rezpect and zeos
  ACTION distribute(const AllRankings &ranks);

  // Ranks the proposals submitted by the users. User has option to simply order
  // the proposals or vote also.
  ACTION voteprop(const name &user, const vector<uint8_t> &option,
                  const vector<uint64_t> &ids);
  // Ranks the introductions submitted by the users
  ACTION voteintro(const name &user, const vector<uint64_t> &ids);

  // Enables users to submit introductions
  ACTION addintro(const uint64_t &id, const name &user, const string &topic,
                  const string &description);

  // Enables users to submit proposals
  ACTION addprop(const uint64_t &id, const name &user, const string &question,
                 const string &description,
                 const vector<uint64_t> &votedforoption);

  // Sets total amount to be distributed per one cycle
  ACTION zeosreward(const asset &quantity);

  /// Clears all tables of the smart contract (useful if table schema needs
  /// update)
  ACTION cleartables();

  /// Initializes the contract and resets all tables
  ACTION init();

  /// Creates a new member account for this fractal (allocates RAM)
  // ACTION signup(const name &user);

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

private:
  void validate_symbol(const symbol &symbol);

  void validate_quantity(const asset &quantity);

  void validate_memo(const string &memo);

  void sub_balance(const name &owner, const asset &value);

  void add_balance(const name &owner, const asset &value,
                   const name &ram_payer);

  void send(const name &from, const name &to, const asset &quantity,
            const std::string &memo, const name &contract);

  void issuerez(const name &to, const asset &quantity, const string &memo);

  void dogroups();
};

////}; // namespace zeos1fractal