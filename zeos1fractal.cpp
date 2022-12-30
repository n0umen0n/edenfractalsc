#include "zeos1fractal.hpp"

zeos1fractal::zeos1fractal(name self, name code, datastream<const char *> ds)
    : contract(self, code, ds), _global(_self, _self.value) {}

// also includes picking yes or no.
void zeos1fractal::voteprop(const name &user, const vector &<uint64_t> ids,
                            const vector &<uint8_t> option) {
  require_auth(user);

  symbol sym = symbol("REZPECT", 4);

  accounts baltable(_self, user.value);

  const auto &baliter =
      baltable.get(sym.code().raw(), "You have no REZPECT tokens brother.");

  usrpropvote_t usrvote(_self, _self.value);
  const auto &voteiter = usrvote.find(user.value);

  // meaning user has voted before
  if (voteiter != usrvote.end())

  {

    for (size_t i = 0; i < voteiter.ids.size(); i++) {

      // go to proposals to proposals table
      // proposals_t proptab(_self, _self.value);
      const auto &proprow = proptab.find(voteiter.ids[i]);

      check(proprow == proptab.end(), "Proposal with such ID does not exist.");

      // uint64_t adjustedbal = baliter.balance.amount / (i + 1);

      proptab.modify(proprow, user, [&](auto &contract) {
        contract.totaltokens -= adjustedbal;
      });

      if (voteiter.option[i] == 0)

      {

        proptab.modify(proprow, user, [&](auto &contract) {
          contract.votedforoption[0] -= baliter.balance.amount;
        });
      }

      if (voteiter.option[i] == 1)

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

    proptab.modify(proprow, user, [&](auto &contract) {
      contract.totaltokens -= adjustedbal;
    });
    // 0=no
    if (option[i] == 0)

    {

      proptab.modify(proprow, user, [&](auto &contract) {
        contract.votedforoption[0] -= baliter.balance.amount;
      });
    }
    // 1=yes
    if (option[i] == 1)

    {

      proptab.modify(proprow, user, [&](auto &contract) {
        contract.votedforoption[1] -= baliter.balance.amount;
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

void zeos1fractal::voteintro(const name &user, const vector<name &> ids) {
  // will see if separate actio needed
}

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

void zeos1fractal::init() {
  require_auth(_self);
  cleartables();

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

void zeos1fractal::join(const name &user) {
  require_auth(user);
  members_t members(_self, _self.value);
  check(members.find(user.value) != members.end(),
        "user is not signed up yet!");
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

void zeos1fractal::setacceptdel(const name &user, const bool &value) {
  require_auth(user);
  members_t members(_self, _self.value);
  auto u = members.find(user.value);
  check(u != members.end(), "user is not signed up yet!");

  members.modify(u, user, [&](auto &row) { row.accept_delegate = value; });
}

void zeos1fractal::setintro(const name &user, const uint64_t &num_blocks) {
  require_auth(user);
  members_t members(_self, _self.value);
  check(members.find(user.value) != members.end(),
        "user is not signed up yet!");

  // TODO
}

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