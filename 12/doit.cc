// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <array>
#include <vector>
#include <numeric>
#include <cassert>

using namespace std;

using coords = array<int, 3>;

coords operator+(coords const &c1, coords const &c2) {
  return { c1[0] + c2[0], c1[1] + c2[1], c1[2] + c2[2] };
}

coords operator-(coords const &c1, coords const &c2) {
  return { c1[0] - c2[0], c1[1] - c2[1], c1[2] - c2[2] };
}

int manhattan(coords const &c) { return abs(c[0]) + abs(c[1]) + abs(c[2]); }

int sgn(int i) { return i == 0 ? 0 : (i < 0 ? -1 : +1); }

coords gravity(coords const &c) {
  return { sgn(c[0]), sgn(c[1]), sgn(c[2]) };
}

struct moons {
  // Positions
  vector<coords> pos;
  // Velocities
  vector<coords> vel;

  // Construct from stdin
  moons();

  // One step of the simulation
  void step();
  // Energy as in part 1
  int energy() const;
  // Cycle length.  This depends on two observations:
  // 1. The evolution is invertible, so the first repeat has to come
  //    back to the initial state.  That is, there can't be a prefix
  //    followed by a cycle, since the first state on the cycle would
  //    not have a unique predecessor.
  // 2. The x, y, and z components are completely independent, so the
  //    overall cycle is the lcm of the cycles in x, y, and z
  long cycle_length();
};

moons::moons() {
  string line;
  auto num = [&](string const &pattern) {
               auto pos = line.find(pattern);
               assert(pos != string::npos);
               return stoi(line.substr(pos + pattern.length()));
             };
  while (getline(cin, line)) {
    int x = num("x=");
    int y = num("y=");
    int z = num("z=");
    pos.push_back({ x, y, z });
  }
  vel.resize(pos.size(), coords{ 0, 0, 0 });
}

void moons::step() {
  for (size_t i = 0; i + 1 < pos.size(); ++i)
    for (size_t j = i + 1; j < pos.size(); ++j) {
      coords g = gravity(pos[j] - pos[i]);
      vel[i] = vel[i] + g;
      vel[j] = vel[j] - g;
    }
  for (size_t i = 0; i < pos.size(); ++i)
    pos[i] = pos[i] + vel[i];
}

int moons::energy() const {
  int total = 0;
  for (size_t i = 0; i < pos.size(); ++i)
    total += manhattan(pos[i]) * manhattan(vel[i]);
  return total;
}

long moons::cycle_length() {
  long result = 1;
  for (size_t c = 0; c < pos[0].size(); ++c) {
    auto orig_pos = pos;
    auto orig_vel = vel;
    long num_steps = 0;
    bool cycled = false;
    while (!cycled) {
      step();
      ++num_steps;
      cycled = true;
      for (size_t i = 0; i < pos.size(); ++i)
        if (pos[i][c] != orig_pos[i][c] || vel[i][c] != orig_vel[i][c]) {
          cycled = false;
          break;
        }
    }
    pos = orig_pos;
    vel = orig_vel;
    result = lcm(result, num_steps);
  }
  return result;
}

void part1() {
  moons ms;
  for (int _ = 0; _ < 1000; ++_)
    ms.step();
  cout << ms.energy() << '\n';
}

void part2() {
  moons ms;
  cout << ms.cycle_length() << '\n';
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
