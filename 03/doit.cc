// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include <cassert>

using namespace std;

using coords = pair<int, int>;

coords operator+(coords const &c1, coords const &c2) {
  return { c1.first + c2.first, c1.second + c2.second };
}

coords operator*(int sc, coords const &c) {
  return { sc * c.first, sc * c.second };
}

int manhattan(coords const &c) { return abs(c.first) + abs(c.second); }

int length(coords const &c1, coords const &c2) {
  assert(c1.first == c2.first || c1.second == c2.second);
  if (c1.first == c2.first)
    return abs(c1.second - c2.second);
  else
    return abs(c1.first - c2.first);
}

bool vertical(coords const &a1, coords const &a2) {
  return a1.first == a2.first;
}

coords flip(coords const &c) { return { c.second, c.first }; }

optional<coords> flip(optional<coords> const &c) {
  if (c)
    return flip(*c);
  return c;
}

// Find the intersection of perpendicular segments, if any.  (I was
// originally handling intersections of overlapping parallel segments
// too, but after seeing part 2 I'm fairly confident that this
// situation won't occur and wasn't intended to be counted as a real
// intersection.  So this is the simplified version.)
optional<coords> intersect(coords a1, coords a2, coords b1, coords b2) {
  if (!vertical(a1, a2))
    return flip(intersect(flip(a1), flip(a2), flip(b1), flip(b2)));
  if (vertical(b1, b2))
    return nullopt;
  // Vertical and horizontal
  if (a1 > a2)
    swap(a1, a2);
  if (b1 > b2)
    swap(b1, b2);
  // Vertical, horizontal, and in canonical order
  int x = a1.first;
  int y = b1.second;
  if (x < b1.first || b2.first < x)
    // No overlap in x
    return nullopt;
  if (y < a1.second || a2.second < y)
    // No overlap in y
    return nullopt;
  return coords{ x, y };
}

map<char, coords> dirs{
  { 'R', { 1, 0 } }, { 'L', { -1, 0 } }, { 'U', { 0, 1 } }, { 'D', { 0, -1 } }
};

vector<coords> read_wire() {
  string line;
  getline(cin, line);
  line.push_back(',');
  stringstream ss(line);
  char dir, comma;
  int len;
  coords c{ 0, 0 };
  vector<coords> result{ c };
  while (ss >> dir >> len >> comma) {
    c = c + len * dirs.find(dir)->second;
    result.push_back(c);
  }
  return result;
}

void solve(function<void(coords const &c, int steps)> fn) {
  auto wire1 = read_wire();
  auto wire2 = read_wire();
  int steps1 = 0;
  for (size_t i = 0; i + 1 < wire1.size(); ++i) {
    int steps2 = 0;
    for (size_t j = 0; j + 1 < wire2.size(); ++j) {
      auto cross = intersect(wire1[i], wire1[i + 1], wire2[j], wire2[j + 1]);
      if (cross && *cross != coords{ 0, 0 }) {
        int to_cross1 = steps1 + length(wire1[i], *cross);
        int to_cross2 = steps2 + length(wire2[j], *cross);
        fn(*cross, to_cross1 + to_cross2);
      }
      steps2 += length(wire2[j], wire2[j + 1]);
    }
    steps1 += length(wire1[i], wire1[i + 1]);
  }
}

void part1() {
  optional<coords> closest;
  solve([&](coords const &c, int) {
          if (!closest || manhattan(c) < manhattan(*closest))
          closest = c;
        });
  assert(closest);
  cout << manhattan(*closest) << '\n';
}

void part2() {
  optional<int> shortest_dist;
  solve([&](coords const &, int steps) {
          if (!shortest_dist || steps < *shortest_dist)
          shortest_dist = steps;
        });
  assert(shortest_dist);
  cout << *shortest_dist << '\n';
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
