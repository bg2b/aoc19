// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <cassert>

using namespace std;

using coords = pair<int, int>;

coords operator+(coords const &c1, coords const &c2) {
  return { c1.first + c2.first, c1.second + c2.second };
}

coords operator-(coords const &c1, coords const &c2) {
  return { c1.first - c2.first, c1.second - c2.second };
}

coords operator/(coords const &c, int sc) {
  return { c.first / sc, c.second / sc };
}

coords reduce(coords const &c) { return c / gcd(c.first, c.second); }

int len2(coords const &c) { return c.first * c.first + c.second * c.second; }

// Polar angle order (for reduced coords), with convention matching
// the laser sweep
double laser_angle(coords const &c) {
  double angle = atan2(c.first, -c.second);
  if (angle < 0.0)
    angle += 2.0 * atan2(0, -1);
  assert(angle >= 0.0);
  return angle;
}

struct region {
  set<coords> asteroids;

  region();

  // How many asteroids are visible from a station at c?
  int num_visible(coords const &c) const { return lines_of_sight(c).size(); }
  // The different sight lines from the station at c.  Each map entry
  // is the list of asteroids along a sight line
  map<coords, list<coords>> lines_of_sight(coords const &c) const;
  // Maximum number of visible asteroids and corresponding station
  // location
  pair<int, coords> max_visible() const;
  // Order in which a laser at the optimal station will vaporize
  // the asteroids
  vector<coords> vaporization_order() const;
};

region::region() {
  int y = 0;
  string line;
  while (getline(cin, line)) {
    for (size_t x = 0; x < line.length(); ++x)
      if (line[x] == '#')
        asteroids.emplace(x, y);
    ++y;
  }
  assert(!asteroids.empty());
}

map<coords, list<coords>> region::lines_of_sight(coords const &c) const {
  map<coords, list<coords>> result;
  for (auto const &c1 : asteroids)
    if (c1 != c)
      result[reduce(c1 - c)].push_back(c1 - c);
  return result;
}

pair<int, coords> region::max_visible() const {
  int max_vis = 0;
  coords best{ 0, 0 };
  for (auto const &c : asteroids) {
    int this_vis = num_visible(c);
    if (max_vis < this_vis) {
      max_vis = this_vis;
      best = c;
    }
  }
  return { max_vis, best };
}

vector<coords> region::vaporization_order() const {
  // Where's the station and what does it see?
  auto station = max_visible().second;
  auto los = lines_of_sight(station);
  // Collect directions, sort asteroids along each line of sight by
  // distance.  (Note that asteroids are specified with coords
  // relative to the station.)
  vector<coords> laser_order;
  for (auto &i : los) {
    laser_order.push_back(i.first);
    i.second.sort([](coords const &c1, coords const &c2) {
                    return len2(c1) < len2(c2);
                  });
  }
  // Order sight lines according to the laser's sweep
  sort(laser_order.begin(), laser_order.end(),
       [](coords const &c1, coords const &c2) {
         return laser_angle(c1) < laser_angle(c2);
       });
  // Pair asteroids with the number of the pass where in which they'll
  // be vaporized
  vector<pair<int, coords>> by_pass;
  for (auto const &dir : laser_order) {
    int pass = 0;
    for (auto const &c : los.find(dir)->second)
      by_pass.emplace_back(++pass, c);
  }
  // Order according to pass (preserving previous distance order)
  stable_sort(by_pass.begin(), by_pass.end(),
              [](auto const &p1, auto const &p2) {
                return p1.first < p2.first;
              });
  // Collect final result
  vector<coords> result;
  for (auto const &p : by_pass)
    result.push_back(p.second + station);
  return result;
}

void part1() { cout << region().max_visible().first << '\n'; }

void part2() {
  auto [x, y] = region().vaporization_order()[199];
  cout << 100 * x + y << '\n';
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
