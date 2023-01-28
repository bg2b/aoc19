// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <vector>
#include <array>
#include <list>
#include <set>
#include <functional>
#include <cassert>

using namespace std;

int const n = 5;

using coords = pair<int, int>;

coords mid{ n / 2, n / 2 };

coords operator+(coords const &c1, coords const &c2) {
  return { c1.first + c2.first, c1.second + c2.second };
}

coords operator-(coords const &c1, coords const &c2) {
  return { c1.first - c2.first, c1.second - c2.second };
}

vector<coords> const dirs{
  { 0, +1 }, { 0, -1 }, { -1, 0 }, { +1, 0 }
};

struct level {
  array<array<char, n>, n> state;

  // Read from stdin
  level();
  // All of one character type
  level(char init);

  // Is c within bounds?
  bool in_bounds(coords const &c) const;

  // What's at c, or '.' for things out of bounds
  char at(coords const &c) const;

  // Number of neighbors in direction dir from c.  If up and down are
  // null, just the one level, otherwise looks up and down for
  // additional neighbors
  int nbrs(coords const &c, coords const &dir,
           level const *up, level const *down) const;

  // Return the next state of this level.  If up and down are null,
  // looks at just the one level, otherwise handles nesting
  level next_state(level const *up, level const *down) const;

  // Count something about bugs, function receives (i, j)
  int count(function<int(int, int)> fn) const;
  // Biodiversity for part 1
  int biodiversity() const;
  // Number of bugs for part 2
  int num_bugs() const;
};

level::level() {
  for (int i = 0; i < n; ++i) {
    string row;
    getline(cin, row);
    assert(int(row.length()) == n);
    for (int j = 0; j < n; ++j)
      state[i][j] = row[j];
  }
}

level::level(char init) {
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      state[i][j] = init;
}

bool level::in_bounds(coords const &c) const {
  return c.first >= 0 && c.first < n && c.second >= 0 && c.second < n;
}

char level::at(coords const &c) const {
  return in_bounds(c) ? state[c.first][c.second] : '.';
}

int level::nbrs(coords const &c, coords const &dir,
                level const *up, level const *down) const {
  auto c1 = c + dir;
  if (!up || !down)
    // Single level
    return at(c1) == '#' ? 1 : 0;
  if (!in_bounds(c1))
    // Going off the edge, look up
    return up->at(mid + dir) == '#' ? 1 : 0;
  if (c1 != mid)
    // Non midpoint, normal state
    return at(c1) == '#' ? 1 : 0;
  int result = 0;
  auto check_down = [&](coords const &cedge) {
                      if (!down->in_bounds(cedge - dir))
                        result += down->at(cedge) == '#' ? 1 : 0;
                    };
  // Look around the border
  for (int i = 0; i < n - 1; ++i) {
    check_down({ 0, i });
    check_down({ n - 1, i + 1 });
    check_down({ i + 1, 0 });
    check_down({ i, n - 1 });
  }
  return result;
}

level level::next_state(level const *up, level const *down) const {
  bool nested = up && down;
  auto result(*this);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      coords c{ i, j };
      if (nested && c == mid)
        continue;
      int total_nbrs = 0;
      for (auto const &dir : dirs)
        total_nbrs += nbrs(c, dir, up, down);
      if (state[i][j] == '#' && total_nbrs != 1)
        result.state[i][j] = '.';
      if (state[i][j] == '.' && (total_nbrs == 1 || total_nbrs == 2))
        result.state[i][j] = '#';
    }
  return result;
}

int level::count(function<int(int, int)> fn) const {
  int result = 0;
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (state[i][j] == '#')
        result += fn(i, j);
  return result;
}

int level::biodiversity() const {
  return count([](int i, int j) { return 1 << (5 * i + j); });
}

int level::num_bugs() const {
  return count([](int, int) { return 1; });
}

bool operator<(level const &b1, level const &b2) {
  return b1.state < b2.state;
}

bool operator==(level const &b1, level const &b2) {
  return b1.state == b2.state;
}

struct bugs {
  // Levels; front is up (outer), back is down (inner)
  list<level> lvls;

  // Construct from stdin
  bugs();

  // Compute next state for all levels, dynamically expanding if
  // necessary
  void next_state();

  // Number of bugs in all levels
  int total_bugs() const;
};

bugs::bugs() {
  lvls.emplace_back();
}

void bugs::next_state() {
  level empty('.');
  lvls.push_front(empty);
  lvls.push_back(empty);
  list<level> nxt;
  for (auto i = lvls.begin(); i != lvls.end(); ++i)
    nxt.push_back(i->next_state(i == lvls.begin() ? &empty : &*prev(i),
                                next(i) == lvls.end() ? &empty : &*next(i)));
  lvls = nxt;
  while (!lvls.empty() && lvls.front() == empty)
    lvls.pop_front();
  while (!lvls.empty() && lvls.back() == empty)
    lvls.pop_back();
}

int bugs::total_bugs() const {
  int result = 0;
  for (auto const &lvl : lvls)
    result += lvl.num_bugs();
  return result;
}

void part1() {
  level lvl;
  set<level> seen;
  while (!seen.count(lvl)) {
    seen.insert(lvl);
    lvl = lvl.next_state(nullptr, nullptr);
  }
  cout << lvl.biodiversity() << '\n';
}

void part2() {
  bugs b;
  for (int i = 0; i < 200; ++i)
    b.next_state();
  cout << b.total_bugs() << '\n';
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
