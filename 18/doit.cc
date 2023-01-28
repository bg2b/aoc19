// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <cctype>
#include <cassert>

using namespace std;

using coords = pair<int, int>;

coords operator+(coords const &c1, coords const &c2) {
  return { c1.first + c2.first, c1.second + c2.second };
}

vector<coords> const dirs{
  { 0, +1 }, { 0, -1 }, { -1, 0 }, { +1, 0 }
};

// An interesting location with an entrance, a key, or a door
struct location {
  // What's here
  char what;
  // Here's where
  coords pos;
  // Shortest paths to other locations; the search stops after
  // reaching another interesting thing
  vector<pair<int, location const *>> links;

  location(char what_, coords const &pos_) : what(what_), pos(pos_) {}

  bool is_key() const { return islower(what); }
  bool is_door() const { return isupper(what); }
  bool is_start() const { return isdigit(what); }

  // Numbering for keys and doors
  int index() const { return what - (is_key() ? 'a' : 'A'); }

  // Has a key been collected?  keys is a bitwise OR of 1 << index for
  // all collected keys
  bool collected(int keys) const {
    assert(is_key());
    return keys & (1 << index());
  }

  // Is a door unlocked?  keys is a bitset like above
  bool unlocked(int keys) const {
    assert(is_door());
    return keys & (1 << index());
  }
};

// The tunnels
struct tunnels {
  // The map, slightly altered to mark entrances.  Entrances are
  // numbered ('1', '2', ...), and for part 2 the initial map
  // alteration has been done.
  vector<string> scan;
  // All the interesting stuff
  map<char, location> locations;
  // How many keys are there?
  int nkeys;
  // How many entrances are there?
  int nstart;

  // Construct from stdin.  If multiple_start is true, alter the map
  // for part 2
  tunnels(bool multiple_start);

  char at(coords const &c) const;
  // Get the location for key i
  location const &key(int i) const;

  // Breadth-first search from a location to find shortest paths to
  // other locations.  Fills in the links of loc
  void bfs(location &loc) const;

  // Given a location and a bitset of collected keys, search to find
  // uncollected keys that can be reached.  Considers paths only
  // through unlocked doors.  The search stops upon finding a key
  // (i.e., it won't also include uncollected keys beyond the first
  // one).  Returns a vector of (key index, number of steps) pairs,
  // telling the shortest distance for each uncollected key.
  vector<pair<int, int>> next_missing(location const *loc, int keys) const;

  // Return the minimum number of steps to collect all keys
  int collect() const;
};

tunnels::tunnels(bool multiple_start) {
  string row;
  while (getline(cin, row)) {
    scan.push_back(row);
    assert(row.length() == scan.front().length());
  }
  // Fix up entrances
  for (size_t i = 0; i < scan.size(); ++i) {
    auto pos = scan[i].find('@');
    if (pos != string::npos) {
      if (multiple_start) {
        assert(pos > 0 && pos + 1 < scan[i].length());
        assert(i > 0 && i + 1 < scan.size());
        for (size_t j = i - 1; j <= i + 1; ++j)
          for (auto k = pos - 1; k <= pos + 1; ++k)
            scan[j][k] = '#';
        scan[i - 1][pos - 1] = '1';
        scan[i - 1][pos + 1] = '2';
        scan[i + 1][pos - 1] = '3';
        scan[i + 1][pos + 1] = '4';
      } else
        scan[i][pos] = '1';
    }
  }
  // Collect interesting locations
  for (size_t i = 0; i < scan.size(); ++i)
    for (size_t j = 0; j < scan[i].length(); ++j)
      if (scan[i][j] != '#' && scan[i][j] != '.')
        locations.emplace(scan[i][j], location(scan[i][j], { j, i }));
  nkeys = 0;
  nstart = 0;
  // Find local shortest paths, count keys and entrances
  for (auto &[_, loc] : locations) {
    bfs(loc);
    if (loc.is_key())
      ++nkeys;
    else if (loc.is_start())
      ++nstart;
  }
  assert(nstart == (multiple_start ? 4 : 1));
}

char tunnels::at(coords const &c) const {
  char ch = scan[c.second][c.first];
  return isdigit(ch) ? '.' : ch;
}

location const &tunnels::key(int i) const {
  auto p = locations.find('a' + i);
  assert(p != locations.end());
  return p->second;
}

void tunnels::bfs(location &loc) const {
  map<coords, int> visited;
  list<pair<coords, int>> frontier;
  auto visit = [&](coords const &pos, int depth) {
                 if (visited.count(pos)) {
                   assert(visited.find(pos)->second >= depth - 2);
                   return;
                 }
                 visited.emplace(pos, depth);
                 frontier.emplace_back(pos, depth);
               };
  visit(loc.pos, 0);
  loc.links.emplace_back(0, &loc);
  while (!frontier.empty()) {
    auto [pos, depth] = frontier.front();
    frontier.pop_front();
    char ch = at(pos);
    if (ch != '.' && ch != loc.what) {
      auto p = locations.find(ch);
      assert(p != locations.end());
      loc.links.emplace_back(depth, &p->second);
      continue;
    }
    for (auto const &dir : dirs) {
      char ch = at(pos + dir);
      if (ch != '#')
        visit(pos + dir, depth + 1);
    }
  }
}

vector<pair<int, int>> tunnels::next_missing(location const *current,
                                             int keys) const {
  set<location const *> visited;
  vector<pair<location const *, int>> searching;
  auto visit = [&](location const *loc, int steps) {
                 if (visited.count(loc))
                   return;
                 visited.insert(loc);
                 searching.emplace_back(loc, steps);
               };
  visit(current, 0);
  vector<pair<int, int>> next_keys;
  while (!searching.empty()) {
    auto [loc, steps] = searching.back();
    searching.pop_back();
    for (auto [next_steps, next] : loc->links) {
      if (next->is_door() && !next->unlocked(keys))
        continue;
      if (next->is_key() && !next->collected(keys)) {
        next_keys.emplace_back(next->index(), steps + next_steps);
        continue;
      }
      visit(next, steps + next_steps);
    }
  }
  return next_keys;
}

int tunnels::collect() const {
  using locs = vector<location const *>;
  using state = pair<locs, int>;
  map<state, int> visited;
  auto by_steps = [](pair<state, int> const &p1, pair<state, int> const &p2) {
                    // Search by minimum number of steps
                    return p1.second > p2.second;
                  };
  using qt = pair<state, int>;  // queue type
  priority_queue<qt, vector<qt>, decltype(by_steps)> q(by_steps);
  auto visit = [&](state const &st, int steps) {
                 auto p = visited.find(st);
                 if (p != visited.end() && steps >= p->second)
                   // Reached this state before with no more steps
                   return;
                 visited.insert_or_assign(st, steps);
                 q.push({ st, steps });
               };
  locs start;
  for (int i = 0; i < nstart; ++i)
    start.push_back(&locations.find(i + '1')->second);
  visit({ start, 0 }, 0);
  int all_keys = (1 << nkeys) - 1;
  while (true) {
    assert(!q.empty());
    auto [st, steps] = q.top();
    q.pop();
    if (visited.find(st)->second > steps)
      // Already saw this state in fewer steps (it was inserted in the
      // queue with more steps, then with fewer, then searched with
      // fewer, and this is the left over state that I don't want to
      // expand)
      continue;
    auto [ls, keys] = st;
    if (keys == all_keys)
      // Collected all keys.  Since the queue is ordered by minimum
      // number of steps, this is the answer
      return steps;
    for (int i = 0; i < nstart; ++i) {
      // Consider a move by agent i; all other agents stay fixed
      auto next_ls = ls;
      auto const &loc = ls[i];
      auto candidates = next_missing(loc, keys);
      for (auto [next_key, next_steps] : candidates) {
        next_ls[i] = &key(next_key);
        // Add collected key to set of keys
        int next_keys = keys | (1 << next_key);
        visit({ next_ls, next_keys }, steps + next_steps);
      }
    }
  }
}

void part1() { cout << tunnels(false).collect() << '\n'; }
void part2() { cout << tunnels(true).collect() << '\n'; }

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
