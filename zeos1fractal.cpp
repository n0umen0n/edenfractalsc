#include "zeos1fractal.hpp"

zeos1fractal::zeos1fractal(
    name self,
    name code, 
    datastream<const char *> ds
) :
    contract(self, code, ds),
    _global(_self, _self.value)
{

}

void zeos1fractal::init()
{
    // empty all tables
    members_t members(_self, _self.value);
    intros_t introductions(_self, _self.value);
    modranks_t modranks(_self, _self.value);
    introranks_t introranks(_self, _self.value);
    propranks_t propranks(_self, _self.value);
    joins_t joins(_self, _self.value);
    for(auto it = members.begin(); it != members.end(); ++it) members.erase(it);
    for(auto it = introductions.begin(); it != introductions.end(); ++it) introductions.erase(it);
    for(auto it = modranks.begin(); it != modranks.end(); ++it) modranks.erase(it);
    for(auto it = introranks.begin(); it != introranks.end(); ++it) introranks.erase(it);
    for(auto it = propranks.begin(); it != propranks.end(); ++it) propranks.erase(it);
    for(auto it = joins.begin(); it != joins.end(); ++it) joins.erase(it);
    _global.set({
        CS_IDLE,
        0,
        0,
        vector<name>(),

        0,
        vector<name>(),
        vector<uint64_t>(),

        map<name, uint64_t>({
            {"moderator"_n, 10},    // respect required to be able to become a moderator
            {"delegate"_n, 100},    // respect required to be able to become a delegate
            {"modvote"_n, 200},     // respect required to be able to vote for moderators
            {"propvote"_n, 300}     // respect required to be able to vote for proposals
            // TODO ...
        }),
        3600,                       // 30 min
        5400,                       // 45 min
        5400,                       // 45 min
        1200                        // 10 min
    }, _self);
}

void zeos1fractal::signup(const name& user)
{
    require_auth(user);
    members_t members(_self, _self.value);
    check(members.find(user.value) == members.end(), "user is already signed up!");

    members.emplace(user, [&](auto& row){
        row.user = user;
        row.links = map<string, string>();
        row.zeos_earned = 0;
        row.respect_earned = 0;
        row.accept_moderator = false;
        row.accept_delegate = false;
        row.has_been_delegate = false;
        row.is_banned = false;
    });

    // TODO: create balance for Respect token to allocate RAM at cost of user for later respect token issuance
}

void zeos1fractal::join(const name& user)
{
    require_auth(user);
    members_t members(_self, _self.value);
    check(members.find(user.value) != members.end(), "user is not signed up yet!");
    joins_t joins(_self, _self.value);
    check(joins.find(user.value) == joins.end(), "user has joined already!");
    auto _g = _global.get();
    check(_g.next_event_block_height > 0, "no event upcoming yet!");
    check(static_cast<uint64_t>(current_block_number()) >= _g.next_event_block_height - _g.early_join_duration, "too early to join the event!");

    joins.emplace(user, [&](auto& row){
        row.user = user;
    });
}

void zeos1fractal::addlink(
    const name& user,
    const string& key,
    const string& value
)
{

}

void zeos1fractal::setacceptmod(
    const name& user,
    const bool& value
)
{

}

void zeos1fractal::setacceptdel(
    const name& user,
    const bool& value
)
{

}

void zeos1fractal::setintro(
    const name& user,
    const uint64_t& num_blocks
)
{

}

void zeos1fractal::setevent(const uint64_t& block_height)
{

}

void zeos1fractal::changestate()
{

}