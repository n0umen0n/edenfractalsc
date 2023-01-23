#include "zeos1fractal.hpp"
#include <limits>
#include <map>
#include <numeric>
#include <string>

zeos1fractal::zeos1fractal(name self, name code, datastream<const char *> ds)
    : contract(self, code, ds), _global(_self, _self.value) {}

namespace {

// Some compile-time configuration
const vector<name> admins{"dan"_n, "mschoenebeck"_n, "vladislav.x"_n};

constexpr int64_t max_supply = static_cast<int64_t>(1'000'000'000e4);

constexpr auto min_groups = size_t{2};
constexpr auto min_group_size = size_t{5};
constexpr auto max_group_size = size_t{6};
// Instead of that declare inside the table
// const auto defaultRewardConfig =
// rewardconfig{.zeos_reward_amt = (int64_t)100e4, .fib_offset = 5};

constexpr std::string_view rezpectTransferMemo =
    "Zeos fractal REZPECT distribution.";
constexpr std::string_view zeosTransferMemo =
    "Zeos fractal participation $ZEOS reward.";

// Coefficients of 6th order poly where p is phi (ratio between adjacent
// fibonacci numbers) xp^0 + xp^1 ...
constexpr std::array<double, max_group_size> polyCoeffs{
    1, 1.618, 2.617924, 4.235801032, 6.85352607, 11.08900518};

// Other helpers
auto fib(uint8_t index) -> decltype(index) { //
  return (index <= 1) ? index : fib(index - 1) + fib(index - 2);
};

} // namespace
/*

NOT IN THE BEGINNING.

void zeos1fractal::electdeleg(const name &elector, const name &delegate,
                              const uint64_t &groupnr) {
  require_auth(elector);

  check(is_account(elector), "Elector's account does not exist.");
  check(is_account(delegate), "Delegate's account does not exist.");

  electinf_t singleton(_self, _self.value);

  delegates_t table(_self, 1);

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
*/

void zeos1fractal::submitcons(const uint64_t &groupnr,
                              const std::vector<name> &rankings,
                              const name &submitter) {

  // siia peaks lisama et saaks ainult submittida kui on 6ige period

  require_auth(submitter);

  // Check whether user is a part of a group he is submitting consensus for.

  groups_t _groups(_self, _self.value);

  const auto &groupiter = _groups.get(groupnr, "No group with such ID.");

  bool exists = false;
  for (int i = 0; i < rankings.size(); i++) {
    if (groupiter.users[i] == submitter) {
      exists = true;
      break;
    }
  }
  check(exists, "Account name not found in group");

  size_t group_size = rankings.size();
  check(group_size >= min_group_size, "too small group");
  check(group_size <= max_group_size, "too big group");

  for (size_t i = 0; i < rankings.size(); i++) {
    std::string rankname = rankings[i].to_string();

    check(is_account(rankings[i]), rankname + " account does not exist.");
  }

  // Getting current election nr
  electinf_t electab(_self, _self.value);
  electioninf elecitr;

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

void zeos1fractal::dogroups() {

  joins_t _joined(_self, _self.value);

  // this can be replaced by counter in separate table, that tracks how many
  // have joined.
  auto usersize = std::distance(_joined.cbegin(), _joined.cend());

  while (usersize > 0) {
    std::vector<name> group_users;
    // Create a group of size 6 if there are at least 6 remaining users,
    // or a group of size 5 if there are at least 5 but fewer than 6 remaining
    // users, or a group of size equal to the number of remaining users if there
    // are fewer than 5
    size_t group_size = (usersize >= 6) ? 6 : (usersize >= 5) ? 5 : usersize;
    for (size_t i = 0; i < group_size; i++) {
      auto itr = _joined.begin();

      uint32_t block_num = current_block_number();
      char *seed = reinterpret_cast<char *>(&block_num);
      checksum256 randomness = sha256(seed, sizeof(uint32_t));

      uint32_t randomint = *(uint32_t *)&randomness;

      std::advance(itr, randomint % usersize);
      group_users.push_back(itr->user);
      _joined.erase(itr);
    }

    groups_t _groups(_self, _self.value);
    // Add the group to the `groups` table
    _groups.emplace(get_self(), [&](auto &row) {
      row.id = _groups.available_primary_key();
      row.users = group_users;
    });

    dogroups();
  }
};

void zeos1fractal::issuerez(const name &to, const asset &quantity,
                            const string &memo) {
  action(permission_level{get_self(), "active"_n}, get_self(), "issue"_n,
         std::make_tuple(to, quantity, memo))
      .send();
};

void zeos1fractal::send(const name &from, const name &to, const asset &quantity,
                        const std::string &memo, const name &contract) {
  action(permission_level{get_self(), "active"_n}, contract, "transfer"_n,
         std::make_tuple(from, to, quantity, memo))
      .send();
};

void zeos1fractal::validate_symbol(const symbol &symbol) {
  // check(symbol.value == eden_symbol.value, "invalid symbol");
  check(symbol == rezpect_symbol, "symbol precision mismatch");
}

void zeos1fractal::validate_quantity(const asset &quantity) {
  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "quantity must be positive");
}

void zeos1fractal::validate_memo(const string &memo) {
  check(memo.size() <= 256, "memo has more than 256 bytes");
}

void zeos1fractal::sub_balance(const name &owner, const asset &value) {
  accounts from_acnts(get_self(), owner.value);

  const auto &from =
      from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  from_acnts.modify(from, owner, [&](auto &a) { a.balance -= value; });
}

void zeos1fractal::add_balance(const name &owner, const asset &value,
                               const name &ram_payer) {
  accounts to_acnts(get_self(), owner.value);
  auto to = to_acnts.find(value.symbol.code().raw());
  if (to == to_acnts.end()) {
    to_acnts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
  } else {
    to_acnts.modify(to, same_payer, [&](auto &a) { a.balance += value; });
  }
}

void zeos1fractal::transfer(const name &from, const name &to,
                            const asset &quantity, const string &memo) {
  check(from == get_self(), "Transfer impossibru");
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

void zeos1fractal::issue(const name &to, const asset &quantity,
                         const string &memo) {
  // Only able to issue tokens to self
  check(to == get_self(), "tokens can only be issued to issuer account");
  // Only this contract can issue tokens
  require_auth(get_self());
  /*
      check(quantity.symbol.value == rezpect_symbol.value, "invalid symbol");
      check(quantity.symbol == rezpect_symbol, "symbol precision mismatch");
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

void zeos1fractal::distribute(const AllRankings &ranks) {
  require_auth(_self);

  zeosrew_t rewardConfigTable(_self, _self.value);
  // auto rewardConfig = rewardConfigTable.get_or_default(defaultRewardConfig);
  rewardconfig newrew;

  newrew = rewardConfigTable.get();

  auto numGroups = ranks.allRankings.size();
  check(numGroups >= min_groups, "Too few groups.");

  auto coeffSum =
      std::accumulate(std::begin(polyCoeffs), std::end(polyCoeffs), 0.0);

  // Calculation how much EOS per coefficient.
  auto multiplier = (double)newrew.zeos_reward_amt / (numGroups * coeffSum);

  std::vector<int64_t> zeosRewards;
  std::transform(std::begin(polyCoeffs), std::end(polyCoeffs),
                 std::back_inserter(zeosRewards), [&](const auto &c) {
                   auto finalzeosQuant = static_cast<int64_t>(multiplier * c);
                   check(finalzeosQuant > 0,
                         "Total configured ZEOS distribution is too small to "
                         "distibute any reward to rank 1s");
                   return finalzeosQuant;
                 });

  std::map<name, uint8_t> accounts;

  for (const auto &rank : ranks.allRankings) {
    size_t group_size = rank.ranking.size();
    check(group_size >= min_group_size, "too small group");
    check(group_size <= max_group_size, "too big group");

    auto rankIndex = max_group_size - group_size;
    for (const auto &acc : rank.ranking) {
      check(is_account(acc), "account " + acc.to_string() + " DNE");
      check(0 == accounts[acc]++,
            "account " + acc.to_string() + " listed more than once");

      auto fibAmount = static_cast<int64_t>(fib(rankIndex + newrew.fib_offset));
      auto rezpectAmt = static_cast<int64_t>(
          fibAmount * std::pow(10, rezpect_symbol.precision()));
      auto rezpectQuantity = asset{rezpectAmt, rezpect_symbol};

      // TODO: To better scale this contract, any distributions should not use
      // require_recipient.
      //       (Otherwise other user contracts could fail this action)
      // Therefore,
      //   Eden tokens should be added/subbed from balances directly (without
      //   calling transfer) and EOS distribution should be stored, and then
      //   accounts can claim the EOS themselves.

      // Distribute EDEN

      issuerez(get_self(), rezpectQuantity, "Mint new REZPECT tokens");
      send(get_self(), acc, rezpectQuantity, rezpectTransferMemo.data(),
           get_self());

      // Distribute EOS
      check(zeosRewards.size() > rankIndex,
            "Shouldn't happen."); // Indicates that the group is too large, but
                                  // we already check for that?
      auto zeosQuantity = asset{zeosRewards[rankIndex], zeos_symbol};
      send(get_self(), acc, rezpectQuantity, rezpectTransferMemo.data(),
           "thezeostoken"_n);
      /*
            token::actions::transfer{"thezeostoken"_n, {get_self(),
         "active"_n}}.send( get_self(), acc, zeosQuantity,
         zeosTransferMemo.data());
      */
      ++rankIndex;
    }
  }
}

void zeos1fractal::zeosreward(const asset &quantity, const uint8_t &offset) {

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

// also includes picking yes or no.
void zeos1fractal::voteprop(const name &user, const vector<uint8_t> &option,
                            const vector<uint64_t> &ids) {
  require_auth(user);

  symbol sym = symbol("REZPECT", 4);

  accounts baltable(_self, user.value);

  const auto &baliter =
      baltable.get(sym.code().raw(), "You have no REZPECT tokens brother.");

  propvote_t usrvote(_self, _self.value);
  const auto &voteiter = usrvote.find(user.value);

  proposals_t proptab(_self, _self.value);

  // USER HAS VOTED BEFORE, SO ADJUSTING BY HIS PREVIOUS VOTE, AND DELETING HIS
  // VOTE TABLE
  if (voteiter != usrvote.end())

  {

    for (size_t i = 0; i < voteiter->ids.size(); i++) {

      // go to proposals to proposals table
      // proposals_t proptab(_self, _self.value);
      const auto &proprow = proptab.find(voteiter->ids[i]);

      check(proprow == proptab.end(), "Proposal with such ID does not exist.");

      uint64_t adjustedbaldec = baliter.balance.amount / (i + 1);

      proptab.modify(proprow, user, [&](auto &contract) {
        contract.totaltokens -= adjustedbaldec;
      });

      if (voteiter->option[i] == 0)

      {

        proptab.modify(proprow, user, [&](auto &contract) {
          contract.votedforoption[0] -= baliter.balance.amount;
        });
      }

      if (voteiter->option[i] == 1)

      {

        proptab.modify(proprow, user, [&](auto &contract) {
          contract.votedforoption[1] -= baliter.balance.amount;
        });
      }
    }
  }

  if (voteiter != usrvote.end())

  {
    usrvote.erase(voteiter);
  }

  // SAVING NEW USERS VOTE AND THEN ADJUSTING PROPOSAL BY TOKEN WEIGHTS
  usrvote.emplace(_self, [&](auto &contract) {
    contract.user = user;
    contract.ids = ids;
    contract.option = option;
  });

  for (size_t i = 0; i < ids.size(); i++) {

    // proposals_t proptab(_self, _self.value);
    const auto &proprow = proptab.find(ids[i]);

    check(proprow == proptab.end(),
          "Proposal with such ID does not exist (1).");

    uint64_t adjustedbalinc = baliter.balance.amount / (i + 1);

    proptab.modify(proprow, user, [&](auto &contract) {
      contract.totaltokens += adjustedbalinc;
    });
    // 0=no
    if (option[i] == 0)

    {

      proptab.modify(proprow, user, [&](auto &contract) {
        contract.votedforoption[0] += baliter.balance.amount;
      });
    }
    // 1=yes
    if (option[i] == 1)

    {

      proptab.modify(proprow, user, [&](auto &contract) {
        contract.votedforoption[1] += baliter.balance.amount;
      });
    }
  }

  // get users balance
  // check if has anything if not error no balance

  // If exists users vote table. Scope self, key by name. Then go to proposals
  // table loop number of times the vector ids size. Substract from option and
  // from total voted for proposal. then delete the vote table then emplace
  // the table and add new votes.
}

void zeos1fractal::voteintro(const name &user, const vector<uint64_t> &ids) {

  require_auth(user);

  symbol sym = symbol("REZPECT", 4);

  accounts baltable(_self, user.value);

  const auto &baliter =
      baltable.get(sym.code().raw(), "You have no REZPECT tokens brother.");

  intrvote_t usrvote(_self, _self.value);
  const auto &voteiter = usrvote.find(user.value);

  intros_t introtab(_self, _self.value);

  if (voteiter != usrvote.end())

  {

    for (size_t i = 0; i < voteiter->ids.size(); i++) {

      // go to proposals to proposals table
      // proposals_t proptab(_self, _self.value);
      const auto &introrow = introtab.find(voteiter->ids[i]);

      check(introrow == introtab.end(), "Intro with such ID does not exist.");

      uint64_t adjustedbaldec = baliter.balance.amount / (i + 1);

      introtab.modify(introrow, user, [&](auto &contract) {
        contract.totaltokens -= adjustedbaldec;
      });
    }
  }

  if (voteiter != usrvote.end())

  {
    usrvote.erase(voteiter);
  }

  // SAVING NEW USERS VOTE AND THEN ADJUSTING PROPOSAL BY TOKEN WEIGHTS
  usrvote.emplace(_self, [&](auto &contract) {
    contract.user = user;
    contract.ids = ids;
  });

  for (size_t i = 0; i < ids.size(); i++) {

    // proposals_t proptab(_self, _self.value);
    const auto &introrow = introtab.find(ids[i]);

    check(introrow == introtab.end(),
          "Proposal with such ID does not exist (1).");

    uint64_t adjustedbalinc = baliter.balance.amount / (i + 1);

    introtab.modify(introrow, user, [&](auto &contract) {
      contract.totaltokens += adjustedbalinc;
    });
  }
}

void zeos1fractal::addintro(const uint64_t &id, const name &user,
                            const string &topic, const string &description) {

  intros_t introtab(_self, _self.value);
  auto existing = introtab.find(id);
  check(existing == introtab.end(), "Intro with such ID exists");

  introtab.emplace(_self, [&](auto &contract) {
    contract.id = id;
    contract.user = user;
    contract.topic = topic;
    contract.description = description;
    // contract.status = 0;
    contract.totaltokens = 0;
  });
}

void zeos1fractal::addprop(const uint64_t &id, const name &user,
                           const string &question, const string &description,
                           const vector<uint64_t> &votedforoption) {

  proposals_t proptab(_self, _self.value);
  auto existing = proptab.find(id);
  check(existing == proptab.end(), "Proposal with such ID exists");

  proptab.emplace(_self, [&](auto &contract) {
    contract.id = id;
    contract.user = user;
    contract.question = question;
    contract.description = description;
    contract.status = 0;
    contract.totaltokens = 0;
  });
}

void zeos1fractal::cleartables() {
  require_auth(_self);

  if (_global.exists())
    _global.remove();
}

/*
void zeos1fractal::cleartables() {
  require_auth(_self);
  members_t members(_self, _self.value);
  intros_t introductions(_self, _self.value);
  modranks_t modranks(_self, _self.value);
  introranks_t introranks(_self, _self.value);
  propranks_t propranks(_self, _self.value);
  joins_t joins(_self, _self.value);
  for (auto it = members.begin(); it != members.end(); ++it)
    members.erase(it);
  for (auto it = introductions.begin(); it != introductions.end(); ++it)
    introductions.erase(it);
  for (auto it = modranks.begin(); it != modranks.end(); ++it)
    modranks.erase(it);
  for (auto it = introranks.begin(); it != introranks.end(); ++it)
    introranks.erase(it);
  for (auto it = propranks.begin(); it != propranks.end(); ++it)
    propranks.erase(it);
  for (auto it = joins.begin(); it != joins.end(); ++it)
    joins.erase(it);
  if (_global.exists())
    _global.remove();
}
*/

void zeos1fractal::init() {
  require_auth(_self);
  cleartables();

  // Incrementing election number, so that correct intros, proposals could be
  // queried and new scope for consensus table.
  electinf_t electab(_self, _self.value);
  electioninf newevent;

  newevent = electab.get();
  newevent.electionnr += 1;
  electab.set(newevent, _self);

  _global.set(
      {
          CS_IDLE, 0, 0, vector<name>(),

          0, vector<name>(), vector<uint64_t>(),

          map<name, uint64_t>({
              {"moderator"_n,
               10}, // respect required to be able to become a moderator
              {"delegate"_n,
               100}, // respect required to be able to become a delegate
              {"modvote"_n,
               200}, // respect required to be able to vote for moderators
              {"propvote"_n,
               300} // respect required to be able to vote for proposals
                    // TODO ...
          }),
          3600, // 30 min
          5400, // 45 min
          5400, // 45 min
          1200  // 10 min
      },
      _self);
}

/*
void zeos1fractal::signup(const name &user) {
  require_auth(user);
  members_t members(_self, _self.value);
  check(members.find(user.value) == members.end(),
        "user is already signed up!");

  members.emplace(user, [&](auto &row) {
    row.user = user;
    row.links = map<string, string>();
    row.zeos_earned = 0;
    row.respect_earned = 0;
    row.accept_moderator = false;
    row.accept_delegate = false;
    row.has_been_delegate = false;
    row.is_banned = false;
  });

  // TODO: create balance for Respect token to allocate RAM at cost of user
  // for later respect token issuance
}

*/

void zeos1fractal::join(const name &user) {
  require_auth(user);
  members_t members(_self, _self.value);
  //"user is not signed up yet!")
  if (members.find(user.value) != members.end()) {
    members.emplace(user, [&](auto &row) {
      row.user = user;
      row.links = map<string, string>();
      row.zeos_earned = 0;
      row.respect_earned = 0;
      row.accept_moderator = false;
      row.accept_delegate = false;
      row.has_been_delegate = false;
      row.is_banned = false;
    });
  }

  joins_t joins(_self, _self.value);
  check(joins.find(user.value) == joins.end(), "user has joined already!");
  check(_global.exists(), "'global' not initialized! call 'init' first");
  auto g = _global.get();
  check(g.next_event_block_height > 0, "no event upcoming yet!");
  check(static_cast<uint64_t>(current_block_number()) >=
            g.next_event_block_height - g.early_join_duration,
        "too early to join the event!");

  joins.emplace(user, [&](auto &row) { row.user = user; });
}

void zeos1fractal::addlink(const name &user, const string &key,
                           const string &value) {
  require_auth(user);
  members_t members(_self, _self.value);
  auto u = members.find(user.value);
  check(u != members.end(), "user is not signed up yet!");

  // check if the key/value pair already exists for (some other) user
  for (auto it = members.begin(); it != members.end(); ++it) {
    auto entry = it->links.find(key);
    if (entry != it->links.end()) {
      check(entry->second != value, "key/value pair already exists!");
    }
  }

  members.modify(u, user, [&](auto &row) { row.links[key] = value; });
}

void zeos1fractal::setacceptmod(const name &user, const bool &value) {
  require_auth(user);
  members_t members(_self, _self.value);
  auto u = members.find(user.value);
  check(u != members.end(), "user is not signed up yet!");

  members.modify(u, user, [&](auto &row) { row.accept_moderator = value; });
}

/*
void zeos1fractal::setacceptdel(const name &user, const bool &value) {
  require_auth(user);
  members_t members(_self, _self.value);
  auto u = members.find(user.value);
  check(u != members.end(), "user is not signed up yet!");

  members.modify(u, user, [&](auto &row) { row.accept_delegate = value; });
}
*/
/*
void zeos1fractal::setintro(const name &user, const uint64_t &num_blocks) {
  require_auth(user);
  members_t members(_self, _self.value);
  check(members.find(user.value) != members.end(),
        "user is not signed up yet!");

  // TODO
}
*/

// At the end of the event set new event, initially can be done through bloks.
void zeos1fractal::setevent(const uint64_t &block_height) {
  require_auth(_self);
  check(_global.exists(), "'global' not initialized! call 'init' first");
  auto g = _global.get();

  if (block_height == 0) {
    g.next_event_block_height = 0;
    _global.set(g, _self);
  } else {
    check(block_height > static_cast<uint64_t>(current_block_number()),
          "block_height too low: event must start in the future!");
    g.next_event_block_height = block_height;
    _global.set(g, _self);
  }
}

void zeos1fractal::changestate() {
  // anyone can execute this action
  check(_global.exists(), "'global' not initialized! call 'init' first");
  auto g = _global.get();
  uint64_t cur_bh = static_cast<uint64_t>(current_block_number());

  switch (g.state) {
  case CS_IDLE: {
    check(cur_bh > g.next_event_block_height,
          "too early to move into INTRODUCTION state!");
    g.state = CS_INTRODUCTION;
    _global.set(g, _self);

    dogroups();

  } break;

  case CS_INTRODUCTION: {
    check(cur_bh > g.next_event_block_height + g.introduction_duration,
          "too early to move into BREAKOUT_ROOMS state!");
    g.state = CS_BREAKOUT_ROOMS;
    _global.set(g, _self);
  } break;

  case CS_BREAKOUT_ROOMS: {
    check(cur_bh > g.next_event_block_height + g.introduction_duration +
                       g.breakout_room_duration,
          "too early to move into COUNCIL_MEETING state!");
    g.state = CS_COUNCIL_MEETING;
    _global.set(g, _self);
  } break;

  case CS_COUNCIL_MEETING: {
    check(cur_bh > g.next_event_block_height + g.introduction_duration +
                       g.breakout_room_duration + g.council_meeting_duration,
          "too early to move into IDLE state!");
    check(g.next_event_block_height == 0 || g.next_event_block_height > cur_bh,
          "next event is not yet set by council! call 'setevent' to creat a "
          "new event.");
    g.state = CS_IDLE;
    _global.set(g, _self);
  } break;
  }
}