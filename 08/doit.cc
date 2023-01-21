// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cassert>

using namespace std;

int const w = 25;
int const h = 6;

struct image {
  vector<string> layers;

  image();

  string render() const;
};

image::image() {
  char c;
  while (cin >> c) {
    assert(isdigit(c));
    if (layers.empty() || layers.back().length() == w * h)
      layers.push_back("");
    layers.back().push_back(c);
  }
}

string image::render() const {
  string result(w * h, '2');
  for (auto const &layer : layers)
    for (size_t i = 0; i < layer.length(); ++i)
      if (result[i] == '2')
        result[i] = layer[i];
  for (auto &c : result)
    c = c == '1' ? '@' : ' ';
  return result;
}

void part1() {
  image im;
  int min_zeros = w * h + 1;
  int one_x_two = 0;
  for (auto const &layer : im.layers) {
    auto num = [&](char c) { return count(layer.begin(), layer.end(), c); };
    int nz = num('0');
    if (nz < min_zeros) {
      min_zeros = nz;
      one_x_two = num('1') * num('2');
    }
  }
  cout << one_x_two << '\n';
}

void part2() {
  string pic = image().render();
  for (int i = 0; i < h; ++i)
    cout << pic.substr(i * w, w) << '\n';
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
