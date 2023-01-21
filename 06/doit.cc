// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <cassert>

using namespace std;

map<string, set<string>> orbiting;

void read() {
  string line;
  while (getline(cin, line)) {
    auto pos = line.find(')');
    assert(pos != string::npos);
    string parent = line.substr(0, pos);
    string child = line.substr(pos + 1);
    orbiting[parent].insert(child);
  }
}

using obj_info = tuple<int, bool, bool>;

obj_info const &search(string const &obj, int depth,
                       map<string, obj_info> &info) {
  bool has_you = obj == "YOU";
  bool has_santa = obj == "SAN";
  auto p = orbiting.find(obj);
  if (p != orbiting.end())
    for (auto const &child : p->second) {
      auto [_, child_you, child_santa] =
        search(child, depth + 1, info);
      has_you = has_you || child_you;
      has_santa = has_santa || child_santa;
    }
  auto q = info.emplace(obj, obj_info{ depth, has_you, has_santa }).first;
  return q->second;
}

map<string, obj_info> search() {
  read();
  map<string, obj_info> info;
  search("COM", 0, info);
  return info;
}

void part1() {
  auto info = search();
  int total = 0;
  for (auto const &i : info)
    total += get<0>(i.second);
  cout << total << '\n';
}

void part2() {
  auto info = search();
  int depth_both = 0;
  int depth_you = 0;
  int depth_santa = 0;
  for (auto const &i : info) {
    auto [depth, has_you, has_santa] = i.second;
    if (has_you)
      depth_you = max(depth_you, depth);
    if (has_santa)
      depth_santa = max(depth_santa, depth);
    if (has_you && has_santa)
      depth_both = max(depth_both, depth);
  }
  cout << depth_you + depth_santa - 2 * depth_both - 2 << '\n';
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
