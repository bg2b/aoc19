// -*- C++ -*-
// Optimization is definitely needed for this approach, since part 2
// takes about 1.5 minutes even with that.
//
// I don't know if there's any way to be more clever.  The
// abs(output[i] % 10) thing eliminated thoughts about linearity and
// maybe some sort of sparsity.  Since not many parts of the output
// are needed, maybe there's some way to chain backwards?
//
// g++ -std=c++17 -Wall -g -O -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <string>
#include <vector>
#include <numeric>

using namespace std;

// Inputs are padded with an initial 0 to get rid of the annoying
// "skip the very first value exactly once" thing

/*
// Explicit version, for checking against
vector<int> slow_phase(vector<int> const &input) {
  int n = input.size();
  vector<int> output(n, 0);
  static vector<int> base_pattern{ 0, 1, 0, -1 };
  for (int i = 1; i < n; ++i) {
    int repeats = i;
    for (int j = 0; j < n; ++j) {
      int base_index = j / repeats;
      base_index %= base_pattern.size();
      output[i] += input[j] * base_pattern[base_index];
    }
    output[i] = abs(output[i] % 10);
  }
  return output;
}
*/

// Turns the repeated +1 +1 +1... or -1 -1 -1... from the base pattern
// into appropriate partial sums, but otherwise just following the
// explicit version above
vector<int> fast_phase(vector<int> const &input) {
  int n = input.size();
  vector<int> sums = input;
  partial_sum(sums.begin(), sums.end(), sums.begin());
  vector<int> output(n, 0);
  static vector<int> base_pattern{ 0, 1, 0, -1 };
  for (int i = 1; i < n; ++i) {
    int const repeats = i;
    // bpi = base_pattern index
    int bpi = 0;
    int j = 0;
    while (j < n) {
      int end = min(j + repeats, n);
      if (base_pattern[bpi] != 0) {
        // Add (sum over k in [j, end) of input[k]) * base_pattern[bpi]
        int range_sum = sums[end - 1];
        if (j > 0)
          range_sum -= sums[j - 1];
        output[i] += range_sum * base_pattern[bpi];
      }
      j = end;
      ++bpi;
      bpi &= 3;
    }
    output[i] = abs(output[i] % 10);
  }
  return output;
}

vector<int> read(int repeats) {
  string input;
  cin >> input;
  // Pad with an extra 0, as noted above
  vector<int> result{ 0 };
  for (auto c : input)
    result.push_back(c - '0');
  int n = result.size() - 1;
  result.reserve(repeats * n + 1);
  while (--repeats > 0)
    result.insert(result.end(), result.begin() + 1, result.begin() + 1 + n);
  return result;
}

void part1() {
  auto x = read(1);
  for (int _ = 0; _ < 100; ++_)
    x = fast_phase(x);
  for (int i = 1; i <= 8; ++i)
    cout << x[i];
  cout << '\n';
}

void part2() {
  auto x = read(10000);
  int offset = 0;
  for (int i = 1; i <= 7; ++i)
    offset = 10 * offset + x[i];
  for (int _ = 0; _ < 100; ++_)
    x = fast_phase(x);
  for (int i = offset + 1; i <= offset + 8; ++i)
    cout << x[i];
  cout << '\n';
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
