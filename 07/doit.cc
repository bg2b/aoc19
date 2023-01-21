// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <algorithm>
#include <cassert>

using namespace std;

struct CPU {
  // Storage
  vector<int> memory;
  // Instruction pointer
  unsigned ip{0};
  // Was a halt instruction was executed?
  bool halted{false};

  // Things read by input instructions
  list<int> input_values;
  // Things written by output instructions
  list<int> output_values;

  // Return memory location contents
  int mem(unsigned addr) const {
    assert(addr < memory.size());
    return memory[addr];
  }

  // Fetch the next instruction (or the next part of the current
  // instruction)
  int fetch() { return mem(ip++); }
  // Read the next argument in an instruction, handle it appropriately
  // depending on the argument mode (position or immediate), update
  // mode in preparation for next argument
  int arg(int &mode);
  // Read the address where an operation should store a result, update
  // memory
  void store(int v);

  // Supply a value for input instructions
  void give_input(int v) { input_values.push_back(v); }
  // Retrieve the next input value (which must exist)
  int get_input();
  // Save an output value
  void save_output(int v) { output_values.push_back(v); }
  // Return whatever the last output value was
  int last_output() const;
  // Send available output values to another CPU (clears all output)
  void transmit(CPU &other);

  // Execute the next instruction, return true if OK, false if
  // something stops execution (halt instruction or input with no
  // available data)
  bool execute();
  // Execute instructions until the CPU halts or pauses waiting for
  // data at an input instruction.  Return true if the CPU halted.
  bool run();

  // Construct from stdin
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

void CPU::transmit(CPU &other) {
  auto &q = other.input_values;
  q.splice(q.end(), output_values);
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
    { if (input_values.empty()) {
        // Pause execution for later resumption at this same
        // instruction
        --ip;
        return false;
      }
      store(get_input());
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
    halted = true;
    return false;
  }
  return true;
}

bool CPU::run() {
  while (execute())
    ;
  return halted;
}

// Run stages, feeding output from each stage into the next, until the
// last stage halts, then give output from the last stage
int amplify(CPU const &cpu, vector<int> const &phases) {
  vector<CPU> stages;
  for (auto phase : phases) {
    stages.push_back(cpu);
    stages.back().give_input(phase);
  }
  stages.front().give_input(0);
  bool last_halted = false;
  while (!last_halted)
    for (size_t i = 0; i < stages.size(); ++i) {
      bool done = stages[i].run();
      if (i + 1 < stages.size())
        stages[i].transmit(stages[i + 1]);
      else if (!done)
        stages[i].transmit(stages[0]);
      else
        last_halted = true;
    }
  return stages.back().last_output();
}

void solve(vector<int> phases) {
  CPU cpu;
  int max_output = 0;
  do
    max_output = max(max_output, amplify(cpu, phases));
  while (next_permutation(phases.begin(), phases.end()));
  cout << max_output << '\n';
}

void part1() { solve({ 0, 1, 2, 3, 4 }); }
void part2() { solve({ 5, 6, 7, 8, 9 }); }

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
