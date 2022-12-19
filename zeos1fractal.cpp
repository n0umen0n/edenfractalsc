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

}

void zeos1fractal::signup(const name& user)
{

}

void zeos1fractal::join(const name& user)
{

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