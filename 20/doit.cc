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
#include <optional>
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

// Coordinates, recursion level
using location = pair<coords, int>;

struct maze {
  // Is the maze nested?
  bool recursive;
  // The map
  vector<string> donut;
  // Jumps
  map<coords, coords> conn;
  // Starting location
  coords start;
  // Ending location
  coords end;

  // Construct from stdin
  maze(bool recursive_);

  char at(coords const &c) const;

  // Is c a portal on the outer edge of the donut?
  bool outer(coords const &c) const;

  // Traverse to another portal, taking into account maze nesting
  optional<location> warp(location const &loc) const;

  // Length of shorted path from start to end
  int bfs() const;
};

maze::maze(bool recursive_) : recursive(recursive_) {
  string row;
  while (getline(cin, row)) {
    donut.push_back(row);
    assert(row.length() == donut.front().length());
  }
  map<string, vector<coords>> links;
  for (size_t i = 0; i < donut.size(); ++i)
    for (size_t j = 0; j < donut[i].size(); ++j)
      if (isalpha(donut[i][j])) {
        string s(1, donut[i][j]);
        if (i > 0 && i + 1 < donut.size()) {
          if (isalpha(donut[i - 1][j]) && donut[i + 1][j] == '.') {
            s = donut[i - 1][j] + s;
            links[s].emplace_back(i + 1, j);
          } else if (isalpha(donut[i + 1][j]) && donut[i - 1][j] == '.') {
            s = s + donut[i + 1][j];
            links[s].emplace_back(i - 1, j);
          }
        }
        if (j > 0 && j + 1 < donut[i].size()) {
          if (isalpha(donut[i][j - 1]) && donut[i][j + 1] == '.') {
            s = donut[i][j - 1] + s;
            links[s].emplace_back(i, j + 1);
          } else if (isalpha(donut[i][j + 1]) && donut[i][j - 1] == '.') {
            s = s + donut[i][j + 1];
            links[s].emplace_back(i, j - 1);
          }
        }
      }
  assert(links.count("AA"));
  assert(links.count("ZZ"));
  for (auto const &l : links) {
    if (l.first == "AA") {
      assert(l.second.size() == 1);
      start = l.second.front();
    } else if (l.first == "ZZ") {
      assert(l.second.size() == 1);
      end = l.second.front();
    } else {
      assert(l.second.size() == 2);
      conn.emplace(l.second[0], l.second[1]);
      conn.emplace(l.second[1], l.second[0]);
    }
  }
}

char maze::at(coords const &c) const {
  assert(c.first >= 0 && c.first < int(donut.size()));
  assert(c.second >= 0 && c.second < int(donut[c.first].length()));
  char ch = donut[c.first][c.second];
  // A portal name label doesn't count
  return isalpha(ch) ? ' ' : ch;
}

bool maze::outer(coords const &c) const {
  int max1 = donut.size() - 3;
  int max2 = donut.front().length() - 3;
  return c.first == 2 || c.first == max1 || c.second == 2 || c.second == max2;
}

optional<location> maze::warp(location const &loc) const {
  auto [c, level] = loc;
  auto p = conn.find(c);
  if (p == conn.end())
    return nullopt;
  auto next = p->second;
  if (!recursive)
    // No nesting
    return location{ next, level };
  assert(outer(c) != outer(next));
  bool inward = outer(next);
  if (inward)
    // Going deeper
    return location{ next, level + 1 };
  if (level > 0)
    // Coming back out
    return location{ next, level - 1 };
  // Can't go outward anymore
  return nullopt;
}

int maze::bfs() const {
  set<location> visited;
  list<pair<location, int>> frontier;
  auto visit = [&](location const &loc, int depth) {
                 if (visited.count(loc))
                   return;
                 visited.insert(loc);
                 frontier.emplace_back(loc, depth);
               };
  visit({ start, 0 }, 0);
  while (true) {
    assert(!frontier.empty());
    auto [loc, depth] = frontier.front();
    frontier.pop_front();
    if (loc == location{ end, 0 })
      return depth;
    auto [c, level] = loc;
    for (auto const &dir : dirs) {
      char ch = at(c + dir);
      if (ch == '.')
        visit({ c + dir, level }, depth + 1);
    }
    if (auto jump = warp(loc))
      visit(*jump, depth + 1);
  }
}

void part1() { cout << maze(false).bfs() << '\n'; }
void part2() { cout << maze(true).bfs() << '\n'; }

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
