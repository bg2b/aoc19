// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <string>
#include <algorithm>
#include <cassert>

using namespace std;

bool nondecreasing(unsigned passwd) {
  auto s = to_string(passwd);
  return is_sorted(s.begin(), s.end());
}

bool has_dup(unsigned passwd) {
  auto s = to_string(passwd);
  for (size_t i = 0; i + 1 < s.length(); ++i)
    if (s[i] == s[i + 1])
      return true;
  return false;
}

bool has_dup2(unsigned passwd) {
  auto s = to_string(passwd);
  for (size_t i = 0; i < s.length(); ) {
    size_t j = i + 1;
    while (j < s.length() && s[j] == s[i])
      ++j;
    if (j - i == 2)
      return true;
    i = j;
  }
  return false;
}

void solve(bool (*dup_check)(unsigned passwd)) {
  unsigned low, high;
  char _;
  cin >> low >> _ >> high;
  int ans = 0;
  for (auto passwd = low; passwd <= high; ++passwd)
    if (nondecreasing(passwd) && dup_check(passwd))
      ++ans;
  cout << ans << '\n';
}

void part1() { solve(has_dup); }
void part2() { solve(has_dup2); }

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
