// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>

using namespace std;

int fuel_for(int mass) { return max((mass / 3) - 2, 0); }

void part1() {
  int total_fuel = 0;
  int mass;
  while (cin >> mass)
    total_fuel += fuel_for(mass);
  cout << total_fuel << '\n';
}

void part2() {
  int total_fuel = 0;
  int mass;
  while (cin >> mass)
    for (int fuel = fuel_for(mass); fuel > 0; fuel = fuel_for(fuel))
      total_fuel += fuel;
  cout << total_fuel << '\n';
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
