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

using namespace std;
using namespace eosio;
/*
constexpr std::string_view rezpect_ticker{"REZPECT"};
constexpr symbol zeos_symbol{"ZEOS", 4};
constexpr symbol eden_symbol{rezpect_ticker, 4};

*/

// inline constexpr auto _self = "edenfractal"_n;

constexpr std::string_view eden_ticker{"EDEN"};
constexpr symbol eos_symbol{"EOS", 4};
constexpr symbol eden_symbol{eden_ticker, 4};

constexpr symbol pede_symbol{eden_ticker, 4};

CONTRACT edenfractal : contract {
public:
  TABLE delegates {
    uint64_t groupNr;
    eosio::name elector;
    eosio::name delegate;

    uint64_t primary_key() const { return elector.value; }
  };
  typedef eosio::multi_index<"delegates"_n, delegates> delegates_t;

  TABLE agreementx {
    std::string agreement;
    uint8_t versionNr;
  };
  typedef eosio::singleton<"agreementx"_n, agreementx> agreement_t;

  TABLE signaturex {
    eosio::name signer;
    uint64_t primary_key() const { return signer.value; }
  };

  typedef eosio::multi_index<"signaturesx"_n, signaturex> signature_t;

  TABLE rewardconfgx {
    int64_t eos_reward_amt;
    uint8_t fib_offset;
  };

  typedef eosio::singleton<"eosrewx"_n, rewardconfgx> eosrew_t;
  /*
    TABLE group {
      uint64_t id;
      std::vector<name> users;

      uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<"groups"_n, group> groups_t;
  */
  TABLE orderlead {
    name leader;

    uint64_t primary_key() const { return leader.value; }
  };
  typedef eosio::multi_index<"ordleaders"_n, orderlead> leaders_t;

  struct permission_level_weight {
    permission_level permission;
    uint16_t weight;

    // explicit serialization macro is not necessary, used here only to improve
    // compilation time
    EOSLIB_SERIALIZE(permission_level_weight, (permission)(weight))
  };

  struct wait_weight {
    uint32_t wait_sec;
    uint16_t weight;

    EOSLIB_SERIALIZE(wait_weight, (wait_sec)(weight))
  };

  struct key_weight {
    public_key key;
    uint16_t weight;

    // explicit serialization macro is not necessary, used here only to improve
    // compilation time
    EOSLIB_SERIALIZE(key_weight, (key)(weight))
  };

  struct authority {
    uint32_t threshold;
    std::vector<key_weight> keys;
    std::vector<permission_level_weight> accounts;
    std::vector<wait_weight> waits;

    EOSLIB_SERIALIZE(authority, (threshold)(keys)(accounts)(waits))
  };

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

    uint64_t by_secondary() const { return groupnr; }
  };
  typedef eosio::multi_index<
      "conensus"_n, consensus,
      eosio::indexed_by<
          "bygroupnr"_n,
          eosio::const_mem_fun<consensus, uint64_t, &consensus::by_secondary>>>
      consensus_t;

  TABLE electioninfx {
    uint64_t electionnr = 0;
    eosio::time_point_sec starttime;
  };
  typedef eosio::singleton<"electinfx"_n, electioninfx> electinf_t;

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

  TABLE avgbalance {

    name user;
    uint64_t balance;

    uint64_t primary_key() const { return user.value; }

    uint64_t by_secondary() const { return balance; }
  };

  typedef eosio::multi_index<
      "avgbalance"_n, avgbalance,
      eosio::indexed_by<"avgbalance"_n,
                        eosio::const_mem_fun<avgbalance, uint64_t,
                                             &avgbalance::by_secondary>>>
      avgbalance_t;

  TABLE memberz {
    name user; // EOS account name
    vector<uint64_t>
        period_rezpect;      // each element contains weekly respect earned
    uint8_t meeting_counter; // shows which element in the vector to adjust.

    uint64_t primary_key() const { return user.value; }
  };
  typedef eosio::multi_index<"memberz"_n, memberz> memberz_t;

  edenfractal(name self, name code, datastream<const char *> ds);

  ACTION startelect();

  ACTION submitcons(const uint64_t &groupnr, const std::vector<name> &rankings,
                    const name &submitter);

  ACTION electdeleg(const name &elector, const name &delegate,
                    const uint64_t &groupnr);

  ACTION transfer(const name &from, const name &to, const asset &quantity,
                  const string &memo);

  // standard eosio.token action to issue tokens
  ACTION issue(const name &to, const asset &quantity, const string &memo);

  // distributes rezpect and zeos
  ACTION submitranks(const AllRankings &ranks);

  ACTION setagreement(const std::string &agreement);

  ACTION sign(const name &signer);

  ACTION unsign(const name &signer);

  ACTION retire(const asset &quantity, const string &memo);

  ACTION open(const name &owner, const symbol &symbol, const name &ram_payer);

  ACTION close(const name &owner, const symbol &symbol);

  ACTION eosrewardamt(const asset &quantity, const uint8_t &offset);

private:
  void accountauth(const name &change1, const name &change2,
                   const name &change3, const name &change4,
                   const name &change5, const name &change6);

  void validate_symbol(const symbol &symbol);

  void validate_quantity(const asset &quantity);

  void validate_memo(const string &memo);

  void sub_balance(const name &owner, const asset &value);

  void add_balance(const name &owner, const asset &value,
                   const name &ram_payer);

  void send(const name &from, const name &to, const asset &quantity,
            const std::string &memo, const name &contract);

  void issuerez(const name &to, const asset &quantity, const string &memo);

  void addavgrezp(const asset &value, const name &user);

  void incrmetcount(const name &user);

  void require_admin_auth();
};
