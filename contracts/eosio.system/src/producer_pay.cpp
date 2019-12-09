#include <chrono>
#include <ctime>
#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

// need to exclude const int64_t min_pervote_daily_pay = 100'0000;

//const int64_t min_pervote_daily_pay = 100'0000;
const int64_t    min_activated_stake            = 150'000'000'0000;
double     continuous_rate                = 0.04879; // 5% annual rate
//const double perblock_rate = 0.0025; // 0.25%
//const double standby_rate = 0.0075; // 0.75%
const double     perblock_rate                  = 0.015; // 1.5%

// ....................dummy rates.....................................................................................
const int64_t    network_start_time             = 1575884246;
const int64_t    one_year_to_network_start      = 1575885700;
const int64_t    two_year_to_network_start      = 1575885750;
const int64_t    three_year_to_network_start    = 1575885800;
//const int64_t    network_start_time             = 1575892800;
//const int64_t    one_year_to_network_start      = 1639051200;
//const int64_t    two_year_to_network_start      = 1670587200;
//const int64_t    three_year_to_network_start    = 1702123200;
//const double top_to_producers_percent = 0.3;
//const double witness_reward_percent = 0.075;
// const double standby_rate = 0.00375; // 0.375%
const double     network_percent                = 0.5; // 50% to network 
const double     network_to_producer            = 0.6; // 60% of network to witnessess
const double     network_to_witness             = 0.15; // 15% to witnessess
const uint32_t   blocks_per_year                = 52*7*24*2*3600; // half seconds per year
const uint32_t   seconds_per_year               = 52*7*24*3600;
const uint32_t   blocks_per_day                 = 2 * 24 * 3600;
const uint32_t   blocks_per_hour                = 2 * 3600;
const int64_t    useconds_per_day               = 24 * 3600 * int64_t(1000000);
const int64_t    useconds_per_year              = seconds_per_year*1000000ll;



void system_contract::onblock( ignore<block_header> ) {
using namespace eosio;
require_auth(_self);
block_timestamp timestamp;
name producer;
_ds >> timestamp >> producer;

// _gstate2.last_block_num is not used anywhere in the system contract code anymore.
// Although this field is deprecated, we will continue updating it for now until the last_block_num field
// is eventually completely removed, at which point this line can be removed.
_gstate2.last_block_num = timestamp;

/** until activated stake crosses this threshold no new rewards are paid */
if( _gstate.total_activated_stake < min_activated_stake )
return;

if( _gstate.last_pervote_bucket_fill == time_point() ) /// start the presses
_gstate.last_pervote_bucket_fill = current_time_point();

/**
* At startup the initial producer may not be one that is registered / elected
* and therefore there may be no producer object for them.
*/
auto prod = _producers.find( producer.value );
if ( prod != _producers.end() ) {
_gstate.total_unpaid_blocks++;
_producers.modify( prod, same_payer, [&](auto& p ) {
p.unpaid_blocks++;
});
}

/// only update block producers once every minute, block_timestamp is in half seconds
if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
update_elected_producers( timestamp );

if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
name_bid_table bids(_self, _self.value);
auto idx = bids.get_index<"highbid"_n>();
auto highest = idx.lower_bound( std::numeric_limits<uint64_t>::max()/2 );
if( highest != idx.end() &&
highest->high_bid > 0 &&
(current_time_point() - highest->last_bid_time) > microseconds(useconds_per_day) &&
_gstate.thresh_activated_stake_time > time_point() &&
(current_time_point() - _gstate.thresh_activated_stake_time) > microseconds(14 * useconds_per_day)
) {
_gstate.last_name_close = timestamp;
channel_namebid_to_rex( highest->high_bid );
idx.modify( highest, same_payer, [&]( auto& b ){
b.high_bid = -b.high_bid;
});
}
}
}
}

using namespace eosio;
void system_contract::claimrewards( const name owner ) {
require_auth( owner );
print("authorized owner");
print(owner);
//time_t current = time(0);
//print("current time epoch is = ", current);
print("        .............................................................. ");
auto vote_restriction = _gstate.total_producer_vote_weight / 200;
//print("time comes from here for inflation ::::::::::::::::::::::::::::::::::::");
//std::chrono::time_point<std::chrono::system_clock> start;
//start = std::chrono::system_clock::now();
//print("thw current time is ========", start);
//_gstate.last_producer_schedule_update = block_time;
// code to get the counter for number of the producer that have .5% votes of the total votes for all 50 producer
      auto idx = _producers.get_index<"prototalvote"_n>();
//auto counter = _gstate.counter;
      std::vector< std::pair<eosio::producer_key,uint16_t> > top_fifty_witnesses_producers;
      top_fifty_witnesses_producers.reserve(50);       
if(_gstate.counter == 0){
//for ( auto it = idx.cbegin(); it != idx.cend() && top_fifty_witnesses_producers.size() < 2 && vote_restriction <= it->total_votes && it->active(); ++it ) {
//         _gstate.counter ++;
//counter++;
//	}
//}

for ( auto it = idx.cbegin(); it != idx.cend() && top_fifty_witnesses_producers.size() < 50 && vote_restriction <= it->total_votes && it->active(); ++it ) {
     if(vote_restriction <= it->total_votes){
        top_fifty_witnesses_producers.emplace_back( std::pair<eosio::producer_key,uint16_t>({{it->owner, it->producer_key}, it->location}) );	
}
//counter++;
      }


auto witness_size = top_fifty_witnesses_producers.size();

_gstate.counter = witness_size;

print("    ...........................................     ");
print("total witnesses we got = ",  witness_size);



}

const auto& prod = _producers.get( owner.value );
//auto vitr = _producers.find( owner.value );
check( prod.active(), "producer does not have an active key" );
print("     .....................................................................    ");
print("number of the producer that has passed the vote restriction    ", _gstate.counter);
check( _gstate.total_activated_stake >= min_activated_stake,"cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );
const auto ct = current_time_point();
//auto vote_restriction = _gstate.total_producer_vote_weight / 200;
print("    ........................................................................      ");
print("     vote restrictions are  =    ", vote_restriction);
//print("current time is = ", prod.last_claim_time);
print("    ........................................................................      ");
print("     owner producers votes =     ", prod.total_votes);
print("    ........................................................................      ");
print("    ........................................................................      ");
print("    all producers votes    =     ", _gstate.total_producer_vote_weight             );
// to claim rewards anytime 
// comment this for the testing purpose ----check( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );
//check( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

const asset token_supply = eosio::token::get_supply(token_account, core_symbol().code() );

// vote count over time
const auto usecs_since_last_fill = (ct - _gstate.last_pervote_bucket_fill).count();
if( usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > time_point() ) {
//auto new_tokens = 40000000;  

//............................................inflation code here ...............................................//
//...............................................................................................................//


if(now() >= network_start_time && now() < one_year_to_network_start){
//const int64_t claimable_continuous_rate = int64_t(double(now()-base_inflated) / (10*seconds_per_year) );
continuous_rate = 0.04879;
print("inflation continuous rate is =============================================================================");
print("inflation continuous rate is =============================================================================");
print("inflation continuous rate is =====  ", continuous_rate);
}

if(now() >= one_year_to_network_start && now() < two_year_to_network_start ){
//const int64_t claimable_continuous_rate = int64_t(double(now()-base_inflated) / (10*seconds_per_year) );
continuous_rate  =  0.03922;
print("inflation continuous rate is =====  ", continuous_rate);
}

if(now() >= two_year_to_network_start  && now() < three_year_to_network_start){
//const int64_t claimable_continuous_rate = int64_t(double(now()-base_inflated) / (10*seconds_per_year) );
continuous_rate  =  0.02955;
print("inflation continuous rate is =====  ", continuous_rate);
}

if(now() >= three_year_to_network_start){
//const int64_t claimable_continuous_rate = int64_t(double(now()-base_inflated) / (10*seconds_per_year) );
continuous_rate  =  0.01980;
print("inflation continuous rate is =====  ", continuous_rate);
}


//...............................................................................................................//
//............................................ends here .........................................................//




auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill)) / double(useconds_per_year) );

//auto inf_tokens = (continuous_rate * token_supply.amount);
//print("inflated tokens for first year = ",inf_tokens); 
// // value changed for the inflation added
// auto to_producers = new_tokens / 5;
// auto to_savings = new_tokens - to_producers;
// // changed auto to_per_block_pay = to_producers/4; 
// // to auto to_per_block_pay = to_producers *1.5;
// auto to_per_block_pay = to_producers / 4;
print("-----------------*****----------------------");
print("token supply amount --------------",token_supply.amount);
print("-----------------*****----------------------");
print("usecs since last fill--------------", usecs_since_last_fill);
print("-----------------*****----------------------");
print("useconds per year-------------", useconds_per_year);
print("-----------------*****----------------------");
//.................................................................
//.................................................................
//auto counter = _producers.count(prod.total_votes >= vote_restriction);
//auto counter = 0;
//if(prod.total_votes >= vote_restriction){
//counter ++;
//print("producer counter", counter);
//}
//print("total number of the producers that has passed the vote restriction", counter);
// Aladin 5% inflation and distribution 
auto to_network           = new_tokens * network_percent;
auto to_producers = to_network * network_to_producer;

//to be excluded for no saving but infra

auto to_per_block_pay = to_producers ;
// for 15 % vote reward
auto to_per_vote_pay = to_network * network_to_witness;
auto total_taken_tokens = to_producers + to_per_vote_pay;
auto to_savings = new_tokens - total_taken_tokens;

print("----------------------------*******-----------------");
print("to_producers = ", to_producers);
print("----------------------------*******-----------------");
print("to_networks = ", to_network);
print("--------------------------*****---------------------");
print("new_tokens = ", new_tokens);
print("--------------------------*****---------------------");
print("to_per_block_pay = ", to_per_block_pay);
print("to per vote pay = ",to_per_vote_pay);

INLINE_ACTION_SENDER(eosio::token, issue)(
token_account, { {_self, active_permission} },
{ _self, asset(new_tokens, core_symbol()), std::string("issue tokens for producer pay and savings") }
);

INLINE_ACTION_SENDER(eosio::token, transfer)(
token_account, { {_self, active_permission} },{ _self, saving_account, asset(to_savings, core_symbol()), "unallocated inflation" }
);

INLINE_ACTION_SENDER(eosio::token, transfer)(
token_account, { {_self, active_permission} },
{ _self, bpay_account, asset(to_per_block_pay, core_symbol()), "fund per-block bucket" }
);

if( prod.total_votes >=  vote_restriction){
INLINE_ACTION_SENDER(eosio::token, transfer)(
token_account, { {_self, active_permission} },
{ _self, vpay_account, asset(to_per_vote_pay, core_symbol()), "fund per-vote bucket" }
);
}
_gstate.perblock_bucket = 0;
_gstate.pervote_bucket  = 0;
_gstate.pervote_bucket += to_per_vote_pay;
_gstate.perblock_bucket += to_per_block_pay;
_gstate.last_pervote_bucket_fill = ct;
//auto ccounter = _gstate.counter ;
}

auto prod2 = _producers2.find( owner.value );

/// New metric to be used in pervote pay calculation. Instead of vote weight ratio, we combine vote weight and
/// time duration the vote weight has been held into one metric.
const auto last_claim_plus_3days = prod.last_claim_time + microseconds(3 * useconds_per_day);

bool crossed_threshold = (last_claim_plus_3days <= ct);
bool updated_after_threshold = true;

if ( prod2 != _producers2.end() ) {
updated_after_threshold = (last_claim_plus_3days <= prod2->last_votepay_share_update);
} else {
prod2 = _producers2.emplace( owner, [&]( producer_info2& info ) {
info.owner = owner;
info.last_votepay_share_update = ct;
});
}

// Note: updated_after_threshold implies cross_threshold (except if claiming rewards when the producers2 table row did not exist).
// The exception leads to updated_after_threshold to be treated as true regardless of whether the threshold was crossed.
// This is okay because in this case the producer will not get paid anything either way.
// In fact it is desired behavior because the producers votes need to be counted in the global total_producer_votepay_share for the first time.

int64_t producer_per_block_pay = 0;
if( _gstate.total_unpaid_blocks > 0 ) {
producer_per_block_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
print("----------------------------*******-----------------");
print("producer per block pay = ", producer_per_block_pay);
print("----------------------------****----------------");
print("per block bucket", _gstate.perblock_bucket);
print("----------------------------****----------------");
print("prod unpaid blocks", prod.unpaid_blocks);
print("----------------------------****----------------");
print("per total unpaid blocks", _gstate.total_unpaid_blocks);
}

// counts the number of votes 
double new_votepay_share = update_producer_votepay_share( prod2,
ct,
updated_after_threshold ? 0.0 : prod.total_votes,
true // reset votepay_share to zero after updating
);
//initializese the vote_pay
int64_t producer_per_vote_pay = 0;
if( _gstate2.revision > 0 ) {
double total_votepay_share = update_total_votepay_share( ct );
if( total_votepay_share > 0 && !crossed_threshold ) {
producer_per_vote_pay = int64_t((new_votepay_share * _gstate.pervote_bucket) / total_votepay_share);
if( producer_per_vote_pay > _gstate.pervote_bucket )
producer_per_vote_pay = _gstate.pervote_bucket;
}
} else {
//if( _gstate.total_producer_vote_weight > 0 && prod.total_votes >= vote_restriction && 50 >= vitr->producers.size() ) {


//auto itr = top_fifty_witnesses_producers.find(owner.value);
 

//bool witness_found = false;
// Iterate over all elements in Vector


/*
for (auto & present : top_fifty_witnesses_producers)
{
        if (present == owner.value)
        {
                witness_found = true;
                break;
        }
}
*/
//auto witr = _producers.find( owner.value );
//if( witr->top_fifty_witnesses_producers.size()){
//if(witness_found){
if( _gstate.total_producer_vote_weight > 0 && prod.total_votes >= vote_restriction && _gstate.counter >0) {
//check(top_fifty_witnesses_producers.find(owner.value), "witness reward conditions are not setisfied that is why no witness rewards ");
producer_per_vote_pay = int64_t(_gstate.pervote_bucket / _gstate.counter);
_gstate.counter--;
//counter--;
print("----------------------------*******----------------------------");
print("producer per vote pay =                  ",producer_per_vote_pay);
print("----------------------****-------------------------------------");
print("per vote bucket =                     " , _gstate.pervote_bucket);
print("-------------------****----------------------------------------");
print("total producer vote weight" , _gstate.total_producer_vote_weight);
print("-------------------****----------------------------------------");
print("counter value aftetr reward taken =           " ,_gstate.counter);
}
}

//if( producer_per_vote_pay < min_pervote_daily_pay ) {
//producer_per_vote_pay = 0;
//}

_gstate.pervote_bucket -= producer_per_vote_pay;
_gstate.perblock_bucket -= producer_per_block_pay;
_gstate.total_unpaid_blocks -= prod.unpaid_blocks;
//_gstate.counter = ccounter;

print("..................................................................................");
print("value of block bucket after the value transfered to the repective producer", _gstate.perblock_bucket);
print("..................................................................................");
print("value of the vote bucket after the value tranfered to the respective producer for votes caluculation = " ,_gstate.pervote_bucket );


update_total_votepay_share( ct, -new_votepay_share, (updated_after_threshold ? prod.total_votes : 0.0) );

_producers.modify( prod, same_payer, [&](auto& p) {
p.last_claim_time = ct;
p.unpaid_blocks = 0;
});

if( producer_per_block_pay > 0 ) {
INLINE_ACTION_SENDER(eosio::token, transfer)(
token_account, { {bpay_account, active_permission}, {owner, active_permission} },
{ bpay_account, owner, asset(producer_per_block_pay, core_symbol()), std::string("producer block pay") }
);
}
if( producer_per_vote_pay > 0 ) {
INLINE_ACTION_SENDER(eosio::token, transfer)(
token_account, { {vpay_account, active_permission}, {owner, active_permission} },
{ vpay_account, owner, asset(producer_per_vote_pay, core_symbol()), std::string("producer vote pay") }
);
}
}
} //namespace eosiosystem

