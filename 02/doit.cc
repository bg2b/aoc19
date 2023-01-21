// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <vector>
#include <cassert>

using namespace std;

vector<int> read() {
  vector<int> code;
  int n;
  while (cin >> n) {
    code.push_back(n);
    if (cin.peek() == ',')
      cin.ignore(1);
  }
  return code;
}

int run(vector<int> code, int noun, int verb) {
  code[1] = noun;
  code[2] = verb;
  size_t ip = 0;
  while (true) {
    assert(ip < code.size());
    assert(code[ip] == 1 || code[ip] == 2 || code[ip] == 99);
    if (code[ip] == 99)
      return code[0];
    auto ref = [&](size_t index) -> int & {
                 assert(index < code.size());
                 index = code[index];
                 assert(index < code.size());
                 return code[index];
               };
    int const &arg1 = ref(ip + 1);
    int const &arg2 = ref(ip + 2);
    int &result = ref(ip + 3);
    if (code[ip] == 1)
      result = arg1 + arg2;
    else
      result = arg1 * arg2;
    ip += 4;
  }
}

void part1() { cout << run(read(), 12, 2) << '\n'; }

void part2() {
  auto code = read();
  for (int noun = 0; noun <= 99; ++noun)
    for (int verb = 0; verb <= 99; ++verb)
      if (run(code, noun, verb) == 19690720) {
        cout << 100 * noun + verb << '\n';
        return;
      }
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
