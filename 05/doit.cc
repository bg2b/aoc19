// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <cassert>

using namespace std;

struct CPU {
  vector<int> memory;
  unsigned ip{0};

  list<int> input_values;
  list<int> output_values;

  int mem(unsigned index) const {
    assert(index < memory.size());
    return memory[index];
  }

  int fetch() { return mem(ip++); }
  int arg(int &mode);
  void store(int v);

  void give_input(int v) { input_values.push_back(v); }
  int get_input();
  void save_output(int v) { output_values.push_back(v); }
  int last_output() const;

  bool execute();
  void run();

  CPU();
};

CPU::CPU() {
  string line;
  while (getline(cin, line)) {
    if (line.empty())
      continue;
    bool more_lines = line.back() == ',';
    if (!more_lines)
      line.push_back(',');
    stringstream ss(line);
    int n;
    char comma;
    while (ss >> n >> comma)
      memory.push_back(n);
    if (!more_lines)
      break;
  }
}

enum opcode { add = 1, mul, input, output,
              jtrue, jfalse, lt, eq,
              halt = 99 };

int CPU::arg(int &mode) {
  int v = fetch();
  if (mode % 10 == 0)
    v = mem(v);
  mode /= 10;
  return v;
}

void CPU::store(int v) {
  unsigned addr = fetch();
  assert(addr < memory.size());
  memory[addr] = v;
}

int CPU::get_input() {
  assert(!input_values.empty());
  int v = input_values.front();
  input_values.pop_front();
  return v;
}

int CPU::last_output() const {
  assert(!output_values.empty());
  return output_values.back();
}

bool CPU::execute() {
  int op_and_modes = fetch();
  auto op = opcode(op_and_modes % 100);
  int mode = op_and_modes / 100;
  switch (op) {
  case add:
    { int arg1 = arg(mode);
      int arg2 = arg(mode);
      store(arg1 + arg2);
      break;
    }
  case mul:
    { int arg1 = arg(mode);
      int arg2 = arg(mode);
      store(arg1 * arg2);
      break;
    }
  case input:
    { store(get_input());
      break;
    }
  case output:
    { save_output(arg(mode));
      break;
    }
  case jtrue:
    { int test = arg(mode);
      int dest = arg(mode);
      if (test)
        ip = dest;
      break;
    }
  case jfalse:
    { int test = arg(mode);
      int dest = arg(mode);
      if (!test)
        ip = dest;
      break;
    }
  case lt:
    { int arg1 = arg(mode);
      int arg2 = arg(mode);
      store(arg1 < arg2);
      break;
    }
  case eq:
    { int arg1 = arg(mode);
      int arg2 = arg(mode);
      store(arg1 == arg2);
      break;
    }
  case halt:
    return false;
  }
  return true;
}

void CPU::run() {
  while (execute())
    ;
}

void run_with(int input) {
  CPU cpu;
  cpu.give_input(input);
  cpu.run();
  cout << cpu.last_output() << '\n';
}

void part1() { run_with(1); }
void part2() { run_with(5); }

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
