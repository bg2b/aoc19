// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;

// a * b mod m, but done carefully since for part 2, a and b will be
// enough bits that the naive product overflows.  g++ has a convenient
// 128-bit integer extension, but substitute with something like
// schoolboy multiplication if your compiler doesn't have an
// equivalent.
long x_mod(long a, long b, long m) {
  return long(__int128_t(a) * __int128_t(b) % __int128_t(m));
}

// Algebra of shuffles
//
// Every combination of primitive shuffles can be reduced to a
// standard form of deal * cut * rev, where deal means one of the
// "deal with increment" forms, cut is a single cut, and rev (reverse)
// is an optional application of "deal into new stack".  The following
// relations can be used for the reduction.
//
// Commutation (swapping orders of ops of different types):
//   cut(m) * deal(n) = deal(n) * cut(m * n)
//   rev * deal(n) = deal(n) * rev * cut(n - 1)
//   rev * cut(n) = cut(-n) * rev
// Reduction (combining ops of the same type):
//   rev * rev = identity
//   cut(m) * cut(n) = cut(m + n)
//   deal(m) * deal(n) = deal(m * n)
// (All arithmetic above is modulo the size of the deck.)

struct shuffl {
  // Size of the deck
  long const sz;
  // First the piles to deal into...
  long deal{1};
  // Then the amount to cut (rotate by)...
  long cut{0};
  // Finally an optional reverse
  bool rev{false};

  // Identity
  shuffl(long sz_) : sz(sz_) {}

  // Construct from string form
  shuffl(long sz_, string const &s);

  // Product with another shuffle
  shuffl &operator*=(shuffl const &shfl);

  // Apply to a deck (just for testing, not actually used in parts 1
  // and 2)
  vector<int> operator()(vector<int> const &deck) const;

  // Where does pos wind up?
  long forward(long pos) const;
  // What lands at spot pos?
  long backward(long pos) const;
};

shuffl::shuffl(long sz_, string const &s) : sz(sz_) {
  stringstream ss(s);
  string cmd;
  ss >> cmd;
  if (cmd == "deal") {
    string how;
    ss >> how;
    if (how == "into")
      rev = true;
    else {
      assert(how == "with");
      string inc;
      ss >> inc >> deal;
    }
  } else {
    assert(cmd == "cut");
    ss >> cut;
    // I prefer things to be nonnegative so I don't have to worry
    // about the sign produed by %
    while (cut < 0)
      cut += sz;
  }
}

// The heart of everything; use the algebra of shuffles relations
// above to form the product of two shuffles
shuffl &shuffl::operator*=(shuffl const &shfl) {
  // ' quantities refer to shfl, unprimed to *this
  // Start with (deal * cut * rev) * (deal' * cut' * rev')
  long cut4p;
  if (rev) {
    // Move the rev to the end...
    // deal * cut * deal' * rev * cut2' * cut' * rev'
    long cut2p = shfl.deal - 1;
    // deal * cut * deal' * rev * cut3' * rev'
    long cut3p = cut2p + shfl.cut;
    // deal * cut * deal' * cut4' * rev * rev'
    cut4p = -cut3p;
  } else
    // No rev to move
    cut4p = shfl.cut;
  // Final rev
  bool new_rev = (rev && !shfl.rev) || (!rev && shfl.rev);
  // Move cut across deal'...
  // deal * deal' * cut1 * cut4' * (rev * rev')
  long cut1 = x_mod(shfl.deal, cut, sz);
  // Final cut
  long new_cut = cut1 + cut4p;
  while (new_cut < 0)
    new_cut += sz;
  new_cut %= sz;
  // deal * deal' * (cut1 * cut4') * (rev * rev')
  // Final deal
  long new_deal = x_mod(deal, shfl.deal, sz);
  // Assign
  deal = new_deal;
  assert(deal >= 0);
  cut = new_cut;
  assert(cut >= 0);
  rev = new_rev;
  return *this;
}

vector<int> shuffl::operator()(vector<int> const &deck) const {
  vector<int> result(sz);
  long pos = 0;
  for (int i = 0; i < sz; ++i) {
    result[pos] = deck[i];
    pos += deal;
    pos %= sz;
  }
  rotate(result.begin(), result.begin() + cut, result.end());
  if (rev)
    reverse(result.begin(), result.end());
  return result;
}

long shuffl::forward(long pos) const {
  pos = x_mod(pos, deal, sz);
  pos += sz - cut;
  pos %= sz;
  if (rev)
    pos = sz - 1 - pos;
  return pos;
}

long mult_inv(long a, long p) {
  // p is prime, multiplicative inverse of a mod p is a ^ (p - 2)
  long pm2 = p - 2;
  long result = 1;
  long aa = a;
  while (pm2 > 0) {
    if (pm2 & 1)
      result = x_mod(result, aa, p);
    aa = x_mod(aa, aa, p);
    pm2 >>= 1;
  }
  assert(x_mod(a, result, p) == 1);
  return result;
}

long shuffl::backward(long pos) const {
  if (rev)
    pos = sz - 1 - pos;
  pos += cut;
  pos %= sz;
  pos = x_mod(pos, mult_inv(deal, sz), sz);
  return pos;
}

shuffl read(long sz) {
  string line;
  shuffl result(sz);
  while (getline(cin, line))
    result *= shuffl(sz, line);
  return result;
}

void part1() { cout << read(10007).forward(2019) << '\n'; }

void part2() {
  auto shfl = read(119315717514047L);
  long times = 101741582076661L;
  shuffl powers = shfl;
  shuffl iterated(shfl.sz);
  while (times > 0) {
    if (times & 1)
      iterated *= powers;
    powers *= powers;
    times >>= 1;
  }
  cout << iterated.backward(2020) << '\n';
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
