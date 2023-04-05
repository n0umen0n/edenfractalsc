#include "edenfractal.hpp"
#include <limits>
#include <map>
#include <numeric>
// #include <ranges>
#include <algorithm>
#include <string>

namespace {

// Some compile-time configuration
const vector<name> admins{"dan"_n,      "jseymour.gm"_n, "chkmacdonald"_n,
                          "james.vr"_n, "vladislav.x"_n, "ironscimitar"_n};

constexpr int64_t max_supply = static_cast<int64_t>(1'000'000'000e4);

constexpr auto min_groups = size_t{2};
constexpr auto min_group_size = size_t{5};
constexpr auto max_group_size = size_t{6};
// Instead of that declare inside the table
// const auto defaultRewardConfig =
// rewardconfig{.zeos_reward_amt = (int64_t)100e4, .fib_offset = 5};

const auto eleclimit = seconds(7200);

constexpr std::string_view edenTransferMemo =
    "Eden fractal respect distribution";
constexpr std::string_view eosTransferMemo =
    "Eden fractal participation $EOS reward";

// Coefficients of 6th order poly where p is phi (ratio between adjacent
// fibonacci numbers) xp^0 + xp^1 ...
constexpr std::array<double, max_group_size> polyCoeffs{
    1, 1.618, 2.617924, 4.235801032, 6.85352607, 11.08900518};

// Other helpers
auto fib(uint8_t index) -> decltype(index) { //
  return (index <= 1) ? index : fib(index - 1) + fib(index - 2);
};

} // namespace

edenfractal::edenfractal(name self, name code, datastream<const char *> ds)
    : contract(self, code, ds) {}

void edenfractal::electdeleg(const name &elector, const name &delegate,
                             const uint64_t &groupnr) {
  require_auth(elector);

  check(is_account(elector), "Elector's account does not exist.");
  check(is_account(delegate), "Delegate's account does not exist.");

  electinf_t singleton(_self, _self.value);

  electioninfx serks;

  serks = singleton.get();
  // check(false, serks.electionNr);

  check(serks.starttime + eleclimit > current_time_point(),
        "Election is over. You can't vote anymore.");

  delegates_t table(_self, serks.electionnr);

  if (table.find(elector.value) == table.end()) {
    table.emplace(elector, [&](auto &row) {
      row.elector = elector;
      row.delegate = delegate;
      row.groupNr = groupnr;
    });
  } else {
    check(false, "You can only pick one delegate per election my friend.");
  }
}

void edenfractal::setagreement(const std::string &agreementv) {

  require_auth(_self);

  agreement_t agrtab(_self, _self.value);
  agreementx newagr;

  if (!agrtab.exists()) {
    agrtab.set(newagr, _self);
  } else {
    newagr = agrtab.get();
    check(newagr.versionNr !=
              std::numeric_limits<decltype(newagr.versionNr)>::max(),
          "version nr overflow");
  }
  newagr.agreement = agreementv;
  newagr.versionNr += 1;

  agrtab.set(newagr, _self);
}

void edenfractal::sign(const name &signer) {
  require_auth(signer);

  agreement_t agrtab(_self, _self.value);
  agreementx newagr;
  check(agrtab.exists(), "No agreement to sign");

  signature_t table(_self, _self.value);

  if (table.find(signer.value) == table.end()) {
    table.emplace(signer, [&](auto &row) { row.signer = signer; });
  } else {
    check(false, "You already signed the agreement");
  }
}

void edenfractal::unsign(const name &signer) {
  require_auth(signer);
  signature_t table(_self, _self.value);

  table.erase(*table.require_find(signer.value, "Signer not found"));
}

void edenfractal::retire(const asset &quantity, const string &memo) {
  require_auth(get_self());

  validate_symbol(quantity.symbol);
  validate_quantity(quantity);
  validate_memo(memo);

  auto sym = quantity.symbol.code();
  stats statstable(get_self(), sym.raw());
  const auto &st = statstable.get(sym.raw());

  statstable.modify(st, same_payer, [&](auto &s) { s.supply -= quantity; });

  sub_balance(st.issuer, quantity);
}

void edenfractal::transfer(const name &from, const name &to,
                           const asset &quantity, const string &memo) {
  check(from == get_self(), "we bend the knee to corrupted power");
  require_auth(from);

  validate_symbol(quantity.symbol);
  validate_quantity(quantity);
  validate_memo(memo);

  check(from != to, "cannot transfer to self");
  check(is_account(to), "to account does not exist");

  require_recipient(from);
  require_recipient(to);

  auto payer = has_auth(to) ? to : from;

  sub_balance(from, quantity);
  add_balance(to, quantity, payer);
}

void edenfractal::open(const name &owner, const symbol &symbol,
                       const name &ram_payer) {
  require_auth(ram_payer);

  validate_symbol(symbol);

  check(is_account(owner), "owner account does not exist");
  accounts acnts(get_self(), owner.value);
  check(acnts.find(symbol.code().raw()) == acnts.end(),
        "specified owner already holds a balance");

  acnts.emplace(ram_payer, [&](auto &a) { a.balance = asset{0, symbol}; });
}

void edenfractal::close(const name &owner, const symbol &symbol) {
  require_auth(owner);

  accounts acnts(get_self(), owner.value);
  auto it = acnts.find(symbol.code().raw());
  check(it != acnts.end(), "Balance row already deleted or never existed. "
                           "Action won't have any effect.");
  check(it->balance.amount == 0,
        "Cannot close because the balance is not zero.");
  acnts.erase(it);
}

void edenfractal::eosrewardamt(const asset &quantity, const uint8_t &offset) {

  require_auth(_self);

  eosrew_t rewtab(_self, _self.value);
  rewardconfgx newrew;

  if (!rewtab.exists()) {
    rewtab.set(newrew, _self);
  } else {
    newrew = rewtab.get();
  }
  newrew.eos_reward_amt = quantity.amount;
  newrew.fib_offset = offset;

  rewtab.set(newrew, _self);
}
/*
void edenfractal::fiboffset(uint8_t offset) {
  require_auth(get_self());

  RewardConfigSingleton rewardConfigTable(_self,
                                          _self.value);
  auto record = rewardConfigTable.get_or_default(defaultRewardConfig);

  record.fib_offset = offset;
  rewardConfigTable.set(record, get_self());
}
*/

void edenfractal::startelect() {
  require_admin_auth();

  /*
    eosrew_t rewtab(_self, _self.value);
    rewardconfig newrew;

    if (!rewtab.exists()) {
      rewtab.set(newrew, _self);
    } else {
      newrew = rewtab.get();
    }
    newrew.eos_reward_amt = quantity.amount;
    newrew.fib_offset = offset;

    rewtab.set(newrew, _self);
  */

  electinf_t singleton(_self, _self.value);

  electioninfx pede;

  if (!singleton.exists()) {
    singleton.set(pede, _self);
  } else {
    pede = singleton.get();
  }

  // exliza = singleton.get();

  check(pede.electionnr !=
            std::numeric_limits<decltype(pede.electionnr)>::max(),
        "election nr overflow");

  // check(false, pede.electionnr);

  pede.starttime = current_time_point();
  pede.electionnr += 1;

  singleton.set(pede, get_self());

  memberz_t members(_self, _self.value);

  // GET AVG REZ OF EACH USER AND INCREMENT THEIR MEETING NR
  for (auto iter = members.begin(); iter != members.end(); ++iter)

  {

    uint64_t sum_of_elems = std::accumulate(iter->period_rezpect.begin(),
                                            iter->period_rezpect.end(), 0);

    uint64_t nr_of_weeks = 12;

    uint64_t avgrez = sum_of_elems / nr_of_weeks;

    avgbalance_t to_acnts(_self, _self.value);
    auto to = to_acnts.find(iter->user.value);
    if (to == to_acnts.end()) {
      to_acnts.emplace(_self, [&](auto &a) {
        a.user = iter->user;
        a.balance = avgrez;
      });
    } else {
      to_acnts.modify(to, _self, [&](auto &a) { a.balance = avgrez; });
    }

    incrmetcount(iter->user);
  }

  avgbalance_t new_acnts(_self, _self.value);

  auto row = new_acnts.get_index<name("avgbalance")>();

  vector<name> allmembers;

  for (auto it = row.begin(); it != row.end(); ++it) {

    allmembers.push_back(it->user);
  }

  uint8_t size = allmembers.size();

  vector<name> no_order_leaders = {allmembers[size - 1], allmembers[size - 2],
                                   allmembers[size - 3], allmembers[size - 4],
                                   allmembers[size - 5], allmembers[size - 6]};

  leaders_t leadtbdel(_self, _self.value);
  for (auto iterdel = leadtbdel.begin(); iterdel != leadtbdel.end();) {
    leadtbdel.erase(iterdel++);
  }

  for (int i = 0; i < no_order_leaders.size(); i++) {
    leaders_t leadtb(_self, _self.value);
    leadtb.emplace(_self, [&](auto &a) { a.leader = no_order_leaders[i]; });
  }

  vector<name> order_leaders;

  leaders_t leadtbord(_self, _self.value);
  for (auto iter = leadtbord.begin(); iter != leadtbord.end(); iter++) {
    order_leaders.push_back(iter->leader);
  }

  accountauth(order_leaders[0], order_leaders[1], order_leaders[2],
              order_leaders[3], order_leaders[4], order_leaders[5]);
}

void edenfractal::sub_balance(const name &owner, const asset &value) {
  accounts from_acnts(get_self(), owner.value);

  const auto &from =
      from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  from_acnts.modify(from, owner, [&](auto &a) { a.balance -= value; });
}

void edenfractal::add_balance(const name &owner, const asset &value,
                              const name &ram_payer) {
  accounts to_acnts(get_self(), owner.value);
  auto to = to_acnts.find(value.symbol.code().raw());
  if (to == to_acnts.end()) {
    to_acnts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
  } else {
    to_acnts.modify(to, same_payer, [&](auto &a) { a.balance += value; });
  }
}

void edenfractal::require_admin_auth() {
  bool hasAuth = std::any_of(admins.begin(), admins.end(),
                             [](auto &admin) { return has_auth(admin); });
  check(hasAuth, "missing required authority of admin account");
}

void edenfractal::submitcons(const uint64_t &groupnr,
                             const std::vector<name> &rankings,
                             const name &submitter) {

  // siia peaks lisama et saaks ainult submittida kui on 6ige period

  require_auth(submitter);

  // Check whether user is a part of a group he is submitting consensus for.

  // groups_t _groups(_self, _self.value);

  // const auto &groupiter = _groups.get(groupnr, "No group with such ID.");
  /*
    bool exists = false;
    for (int i = 0; i < rankings.size(); i++) {
      if (groupiter.users[i] == submitter) {
        exists = true;
        break;
      }
    }
    check(exists, "Account name not found in group");
  */
  size_t group_size = rankings.size();
  check(group_size >= min_group_size, "too small group");
  check(group_size <= max_group_size, "too big group");

  for (size_t i = 0; i < rankings.size(); i++) {
    std::string rankname = rankings[i].to_string();

    check(is_account(rankings[i]), rankname + " account does not exist.");
  }

  // Getting current election nr
  electinf_t electab(_self, _self.value);
  electioninfx elecitr;

  elecitr = electab.get();

  consensus_t constable(_self, elecitr.electionnr);

  if (constable.find(submitter.value) == constable.end()) {
    constable.emplace(submitter, [&](auto &row) {
      row.rankings = rankings;
      row.submitter = submitter;
      row.groupnr = groupnr;
    });
  } else {
    check(false, "You can vote only once my friend.");
  }
}

void edenfractal::issuerez(const name &to, const asset &quantity,
                           const string &memo) {
  action(permission_level{get_self(), "owner"_n}, get_self(), "issue"_n,
         std::make_tuple(to, quantity, memo))
      .send();
};

void edenfractal::send(const name &from, const name &to, const asset &quantity,
                       const std::string &memo, const name &contract) {
  action(permission_level{get_self(), "owner"_n}, contract, "transfer"_n,
         std::make_tuple(from, to, quantity, memo))
      .send();
};

void edenfractal::validate_symbol(const symbol &symbol) {
  // check(symbol.value == eden_symbol.value, "invalid symbol");
  check(symbol == eden_symbol, "symbol precision mismatch");
}

void edenfractal::validate_quantity(const asset &quantity) {
  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "quantity must be positive");
}

void edenfractal::validate_memo(const string &memo) {
  check(memo.size() <= 256, "memo has more than 256 bytes");
}

void edenfractal::addavgrezp(const asset &value, const name &user) {
  memberz_t members(_self, _self.value);

  const auto &countiter =
      members.get(user.value, "No such user in members table.");

  uint8_t meeting_counter_real;

  if (countiter.meeting_counter == 12) {
    meeting_counter_real = 0;
  } else {
    meeting_counter_real = countiter.meeting_counter;
  }

  auto iter = members.find(user.value);
  if (iter == members.end()) {
    check(false, "Should not happenx.");
  } else {
    members.modify(iter, _self, [&](auto &a) {
      a.period_rezpect[meeting_counter_real] = value.amount;
    });
  }
}

void edenfractal::incrmetcount(const name &user) {

  memberz_t members(_self, _self.value);

  const auto &countiter =
      members.get(user.value, "No such user in members table.");

  uint8_t newcounter;

  if (countiter.meeting_counter == 12)

  {
    newcounter = 0;
  }

  else {

    newcounter = countiter.meeting_counter;
  }

  auto iter = members.find(user.value);
  if (iter == members.end()) {
    check(false, "Should not happen.");
  } else {
    members.modify(iter, _self,
                   [&](auto &a) { a.meeting_counter = newcounter + 1; });
  }
}

void edenfractal::issue(const name &to, const asset &quantity,
                        const string &memo) {
  // Only able to issue tokens to self
  check(to == get_self(), "tokens can only be issued to issuer account");
  // Only this contract can issue tokens
  require_auth(get_self());
  /*
      check(quantity.symbol.value == eden_symbol.value, "invalid symbol");
      check(quantity.symbol == eden_symbol, "symbol precision mismatch");
  */
  validate_symbol(quantity.symbol);
  validate_quantity(quantity);
  validate_memo(memo);

  auto sym = quantity.symbol.code();
  stats statstable(get_self(), sym.raw());
  const auto &st = statstable.get(sym.raw());
  check(quantity.amount <= st.max_supply.amount - st.supply.amount,
        "quantity exceeds available supply");

  statstable.modify(st, same_payer, [&](auto &s) { s.supply += quantity; });

  add_balance(st.issuer, quantity, st.issuer);
}

void edenfractal::submitranks(const AllRankings &ranks) {
  // This action calculates both types of rewards: EOS rewards, and the new
  // token rewards.
  require_auth(get_self());

  eosrew_t rewardConfigTable(_self, _self.value);
  // auto rewardConfig = rewardConfigTable.get_or_default(defaultRewardConfig);
  rewardconfgx newrew;

  newrew = rewardConfigTable.get();

  auto numGroups = ranks.allRankings.size();
  check(numGroups >= min_groups, "Number of groups is too small");

  auto coeffSum =
      std::accumulate(std::begin(polyCoeffs), std::end(polyCoeffs), 0.0);

  // Calculation how much EOS per coefficient.
  auto multiplier = (double)newrew.eos_reward_amt / (numGroups * coeffSum);

  std::vector<int64_t> eosRewards;
  std::transform(std::begin(polyCoeffs), std::end(polyCoeffs),
                 std::back_inserter(eosRewards), [&](const auto &c) {
                   auto finalEosQuant = static_cast<int64_t>(multiplier * c);
                   check(finalEosQuant > 0,
                         "Total configured EOS distribution is too small to "
                         "distibute any reward to rank 1s");
                   return finalEosQuant;
                 });

  std::map<name, uint8_t> accounts;

  for (const auto &rank : ranks.allRankings) {
    size_t group_size = rank.ranking.size();
    check(group_size >= min_group_size, "group size too small");
    check(group_size <= max_group_size, "group size too large");

    auto rankIndex = max_group_size - group_size;
    for (const auto &acc : rank.ranking) {
      check(is_account(acc), "account " + acc.to_string() + " DNE");
      check(0 == accounts[acc]++,
            "account " + acc.to_string() + " listed more than once");

      auto fibAmount = static_cast<int64_t>(fib(rankIndex + newrew.fib_offset));
      auto edenAmt = static_cast<int64_t>(
          fibAmount * std::pow(10, eden_symbol.precision()));
      auto edenQuantity = asset{edenAmt, eden_symbol};

      // TODO: To better scale this contract, any distributions should not use
      // require_recipient.
      //       (Otherwise other user contracts could fail this action)
      // Therefore,
      //   Eden tokens should be added/subbed from balances directly (without
      //   calling transfer) and EOS distribution should be stored, and then
      //   accounts can claim the EOS themselves.

      // issuerez(get_self(), rezpectQuantity, "Mint new REZPECT tokens");

      // Distribute EDEN
      issuerez(get_self(), edenQuantity, "Mint new EDEN tokens");
      // send(get_self(), acc, rezpectQuantity,
      // rezpectTransferMemo.data(),get_self());
      send(get_self(), acc, edenQuantity, "Distribution of EDEN tokens",
           get_self());

      // JUST FOR TESTING
      memberz_t members(_self, _self.value);
      //"user is not signed up yet!")
      vector<uint64_t> period_rezpect = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      if (members.find(acc.value) == members.end()) {
        members.emplace(_self, [&](auto &row) {
          row.user = acc;
          row.period_rezpect = period_rezpect;
          row.meeting_counter = 0;
        });
      }
      addavgrezp(edenQuantity, acc);

      // Distribute EOS
      check(eosRewards.size() > rankIndex,
            "Shouldn't happen."); // Indicates that the group is too large, but
                                  // we already check for that?

      auto eosQuantity = asset{eosRewards[rankIndex], eos_symbol};

      send(get_self(), acc, eosQuantity, "testing", "eosio.token"_n);

      ++rankIndex;
    }
  }
}
/*
void edenfractal::eosrewardamt(const asset &quantity, const uint8_t &offset) {

  require_auth(_self);

  zeosrew_t rewtab(_self, _self.value);
  rewardconfig newrew;

  if (!rewtab.exists()) {
    rewtab.set(newrew, _self);
  } else {
    newrew = rewtab.get();
  }
  newrew.zeos_reward_amt = quantity.amount;
  newrew.fib_offset = offset;

  rewtab.set(newrew, _self);
}
*/
void edenfractal::accountauth(const name &change1, const name &change2,
                              const name &change3, const name &change4,
                              const name &change5, const name &change6) {

  // Setup authority for contract. Choose either a new key, or account, or
  // both.
  authority contract_authority;

  // TEST WHETHER THIS ONE HAS TO BE DUPLICATED TO ADD ANOTHER ACC.
  //  Account to take over permission changeto@perm_child
  permission_level_weight account1{.permission =
                                       permission_level{change1, "active"_n},
                                   .weight = (uint16_t)1};
  permission_level_weight account2{.permission =
                                       permission_level{change2, "active"_n},
                                   .weight = (uint16_t)1};
  permission_level_weight account3{.permission =
                                       permission_level{change3, "active"_n},
                                   .weight = (uint16_t)1};
  permission_level_weight account4{.permission =
                                       permission_level{change4, "active"_n},
                                   .weight = (uint16_t)1};
  permission_level_weight account5{.permission =
                                       permission_level{change5, "active"_n},
                                   .weight = (uint16_t)1};
  permission_level_weight account6{.permission =
                                       permission_level{change6, "active"_n},
                                   .weight = (uint16_t)1};

  // Key is not supplied
  contract_authority.threshold = 4;
  contract_authority.keys = {};
  contract_authority.accounts = {account1, account2, account3,
                                 account4, account5, account6};
  contract_authority.waits = {};

  action(
      permission_level{_self, name("owner")}, name("eosio"), name("updateauth"),
      // keep empty instead of owner
      std::make_tuple(_self, name("active"), name("owner"), contract_authority))
      .send();

} // namespace eosio

/*
void edenfractal::setevent(const uint64_t &block_height) {

  memberz_t members(_self, _self.value);

  // GET AVG REZ OF EACH USER AND INCREMENT THEIR MEETING NR
  for (auto iter = members.begin(); iter != members.end(); ++iter)

  {

    // sum of vector

    uint64_t sum_of_elems = std::accumulate(iter->period_rezpect.begin(),
                                            iter->period_rezpect.end(), 0);

    // Create singleton for it to be adjustable
    uint64_t nr_of_weeks = 12;

    uint64_t avgrez = sum_of_elems / nr_of_weeks;

    avgbalance_t to_acnts(_self, _self.value);
    auto to = to_acnts.find(iter->user.value);
    if (to == to_acnts.end()) {
      to_acnts.emplace(_self, [&](auto &a) {
        a.user = iter->user;
        a.balance = avgrez;
      });
    } else {
      to_acnts.modify(to, _self, [&](auto &a) { a.balance = avgrez; });
    }

    incrmetcount(iter->user);
  }

  //
  avgbalance_t new_acnts(_self, _self.value);

  auto row = new_acnts.get_index<name("avgbalance")>();

  vector<name> allmembers;

  for (auto it = row.begin(); it != row.end(); ++it) {

    allmembers.push_back(it->user);
  }

  uint8_t size = allmembers.size();

  vector<name> no_order_leaders = {allmembers[size - 1], allmembers[size - 2],
                                   allmembers[size - 3], allmembers[size - 4],
                                   allmembers[size - 5], allmembers[size - 6]};

  leaders_t leadtbdel(_self, _self.value);
  for (auto iterdel = leadtbdel.begin(); iterdel != leadtbdel.end();) {
    leadtbdel.erase(iterdel++);
  }

  // ADD into lead table
  for (int i = 0; i < no_order_leaders.size(); i++) {
    // somewhere should be clear tables
    leaders_t leadtb(_self, _self.value);
    leadtb.emplace(_self, [&](auto &a) { a.leader = no_order_leaders[i]; });
  }

  vector<name> order_leaders;

  leaders_t leadtbord(_self, _self.value);
  for (auto iter = leadtbord.begin(); iter != leadtbord.end(); iter++) {
    order_leaders.push_back(iter->leader);
  }

  accountauth(order_leaders[0], order_leaders[1], order_leaders[2],
              order_leaders[3], order_leaders[4], order_leaders[5]);
}
*/