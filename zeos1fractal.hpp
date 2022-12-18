#pragma once

#include <map>
#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

CONTRACT zeos1fractal
{
    TABLE member
    {
        name user;
        map<string, string> links;
        uint64_t zeos_earned;
        uint64_t respect_earned;
        bool accept_moderator;
        bool accept_delegate;
        bool has_been_delegate;

        uint64_t primary_key() const { return user.value; }
    };
    typedef eosio::multi_index<"members"_n, member> members_t;

    // member profile/account related actions
    ACTION signup(const name& user);
    ACTION addlink(const name& user, const string& key, const string& value);
    ACTION setacceptmod(const name& user, const bool& value);
    ACTION setacceptdel(const name& user, const bool& value);
};