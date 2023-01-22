// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <cassert>

using namespace std;

struct reaction {
  int produced;
  string product;
  list<pair<int, string>> inputs;

  reaction(string const &s);
};

reaction::reaction(string const &s) {
  stringstream ss(s);
  int quantity;
  string resource;
  bool more;
  do {
    ss >> quantity >> resource;
    more = resource.back() == ',';
    if (more)
      resource.pop_back();
    inputs.emplace_back(quantity, resource);
  } while (more);
  string arrow;
  ss >> arrow >> produced >> product;
}

// Topologically sorted FUEL => ORE by read()
vector<reaction> reactions;

void read() {
  string line;
  map<string, reaction> reacts;
  while (getline(cin, line)) {
    reaction react(line);
    reacts.emplace(react.product, react);
  }
  vector<string> topological;
  set<string> done{ "ORE" };
  auto make =
    [&](string const &needed, auto doit) -> void {
      if (done.count(needed))
        return;
      done.insert(needed);
      auto p = reacts.find(needed);
      assert(p != reacts.end());
      for (auto const & [_, input] : p->second.inputs)
        doit(input, doit);
      topological.push_back(needed);
    };
  make("FUEL", make);
  reverse(topological.begin(), topological.end());
  for (auto const &i : topological)
    reactions.push_back(reacts.find(i)->second);
}

void build(map<string, long> &needed) {
  for (auto const &react : reactions) {
    long amount = needed[react.product];
    long repeats = (amount + react.produced - 1) / react.produced;
    for (auto const & [input_amount, input] : react.inputs)
      needed[input] += repeats * input_amount;
  }
}

long ore_for(int fuel) {
  map<string, long> needed{ { "FUEL", fuel } };
  build(needed);
  return needed["ORE"];
}

void part1() {
  read();
  cout << ore_for(1) << '\n';
  map<string, long> needed{ { "FUEL", 1 } };
}

// I'm not sure if there's a clever way to account for the extra stuff
// available after producing a unit of fuel, and to go directly from
// available ore => max fuel.  Anyway, bisecting is obvious and
// reasonably fast.
void part2() {
  read();
  auto can_build = [](int fuel) { return ore_for(fuel) <= 1000000000000L; };
  int ok_amount = 0;
  int too_much = 1;
  while (can_build(too_much)) {
    ok_amount = too_much;
    too_much *= 2;
  }
  while (ok_amount + 1 < too_much) {
    int mid = (ok_amount + too_much) / 2;
    if (can_build(mid))
      ok_amount = mid;
    else
      too_much = mid;
  }
  cout << ok_amount << '\n';
}

int main(int argc, char **argv) {
  if (argc != 2) {
    cerr << "usage: " << argv[0] << " partnum < input\n";
    exit(1);
  }
  if (*argv[1] == '1')
    part1();
  else
    part2();
  return 0;
}
