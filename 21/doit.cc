// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <cassert>

using namespace std;

using num = long;

struct CPU {
  // Storage
  vector<num> memory;
  // Instruction pointer
  num ip{0};
  // Relative base
  num rel_base{0};
  // Was a halt instruction was executed?
  bool halted{false};

  // Things read by input instructions
  list<num> input_values;
  // Things written by output instructions
  list<num> output_values;

  // Return memory location
  num &mem(num addr) {
    assert(addr >= 0);
    if (size_t(addr) >= memory.size())
      memory.resize(max(size_t(addr) + 100, 2 * memory.size()), 0);
    return memory[addr];
  }

  // Fetch the next instruction (or the next part of the current
  // instruction)
  num fetch() { return mem(ip++); }
  // Read the next argument in an instruction, handle it appropriately
  // depending on the argument mode (position or immediate or
  // relative), update mode in preparation for next argument
  num arg(num &mode);
  // Read the address where an operation should store a result, update
  // memory.  Mode must be position or relative.
  void store(num mode, num v);

  // Supply a value for input instructions
  void give_input(num v) { input_values.push_back(v); }
  // Retrieve the next input value (which must exist)
  num get_input();
  // Save an output value
  void save_output(num v) { output_values.push_back(v); }
  // Return whatever the last output value was
  num last_output() const;
  // Is output available?
  bool has_output() const { return !output_values.empty(); }
  // Get output and remove
  num get_output();
  // Clear all output
  void clear_output() { output_values.clear(); }
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
    num n;
    char comma;
    while (ss >> n >> comma)
      memory.push_back(n);
    if (!more_lines)
      break;
  }
}

enum opcode { add = 1, mul, input, output,
              jtrue, jfalse, lt, eq, relbase,
              halt = 99 };

num CPU::arg(num &mode) {
  num v = fetch();
  int m = mode % 10;
  if (m == 0)
    v = mem(v);
  else if (m == 2)
    v = mem(v + rel_base);
  mode /= 10;
  return v;
}

void CPU::store(num mode, num v) {
  assert(mode == 0 || mode == 2);
  num addr = fetch();
  if (mode == 2)
    addr += rel_base;
  mem(addr) = v;
}

num CPU::get_input() {
  assert(!input_values.empty());
  num v = input_values.front();
  input_values.pop_front();
  return v;
}

num CPU::last_output() const {
  assert(!output_values.empty());
  return output_values.back();
}

num CPU::get_output() {
  assert(!output_values.empty());
  num out = output_values.front();
  output_values.pop_front();
  return out;
}

void CPU::transmit(CPU &other) {
  auto &q = other.input_values;
  q.splice(q.end(), output_values);
}

bool CPU::execute() {
  num op_and_modes = fetch();
  auto op = opcode(op_and_modes % 100);
  num mode = op_and_modes / 100;
  switch (op) {
  case add:
    { num arg1 = arg(mode);
      num arg2 = arg(mode);
      store(mode, arg1 + arg2);
      break;
    }
  case mul:
    { num arg1 = arg(mode);
      num arg2 = arg(mode);
      store(mode, arg1 * arg2);
      break;
    }
  case input:
    { if (input_values.empty()) {
        // Pause execution for later resumption at this same
        // instruction
        --ip;
        return false;
      }
      store(mode, get_input());
      break;
    }
  case output:
    { save_output(arg(mode));
      break;
    }
  case jtrue:
    { num test = arg(mode);
      num dest = arg(mode);
      if (test)
        ip = dest;
      break;
    }
  case jfalse:
    { num test = arg(mode);
      num dest = arg(mode);
      if (!test)
        ip = dest;
      break;
    }
  case lt:
    { num arg1 = arg(mode);
      num arg2 = arg(mode);
      store(mode, arg1 < arg2);
      break;
    }
  case eq:
    { num arg1 = arg(mode);
      num arg2 = arg(mode);
      store(mode, arg1 == arg2);
      break;
    }
  case relbase:
    { rel_base += arg(mode);
      break;
    }
  case halt:
    halted = true;
    return false;
  default:
    // Illegal instruction
    assert(op == halt);
    break;
  }
  return true;
}

bool CPU::run() {
  while (execute())
    ;
  return halted;
}

// The springdroid
struct droid {
  // Code read from stdin
  CPU cpu;

  // Run a program, return 0 (and dump output) on failure, else return
  // scan result
  int run(string const &s) const;
};

int droid::run(string const &s) const {
  CPU exec(cpu);
  for (char c : s)
    exec.give_input(c);
  bool halted = exec.run();
  assert(halted);
  int success = exec.last_output();
  if (success > 255)
    return success;
  while (exec.has_output())
    cout << char(exec.get_output());
  return 0;
}

// These were just done by hand.  Perhaps there's some way to search
// and derive working formulas?  These are at least structured though.
// The idea is to see a hole and then wait to jump until we're certain
// that we've got a spot to land.  Or for part 2, to look two landing
// spots ahead.

void part1() {
  // Jump if any hole is 1, 2, or 3 (A, B, C) ahead, and 4 (D) is a
  // safe landing space (the droid lands on the 4th spot after
  // jumping)
  string prog("NOT A T\n"
              "OR T J\n"
              "NOT B T\n"
              "OR T J\n"
              "NOT C T\n"
              "OR T J\n"
              "AND D J\n"
              "WALK\n");
  cout << droid().run(prog) << '\n';
}

void part2() {
  // The droid has to look ahead a bit farther for this part.  Again,
  // jump if hole at 1, 2, or 3, and 4 is a safe landing space.
  // However, also ensure safety for a second possible jump after
  // that.  After landing 4 ahead (D), either:
  // 1. have another landing space 8 ahead (H), or
  // 2. have a space at 5 (E) and:
  //    2a. a jump to 9 ahead (I), or
  //    2b. another space at 6 (F)
  string prog("NOT A T\n"
              "OR T J\n"
              "NOT B T\n"
              "OR T J\n"
              "NOT C T\n"
              "OR T J\n"
              "AND D J\n"
              "NOT I T\n" // T = ~I
              "NOT T T\n" // T = I
              "OR F T\n"  // T = I | F
              "AND E T\n" // T = E & (I | F)
              "OR H T\n"  // T = H | (E & (I | F))
              "AND T J\n" // J = D & (H | (E & (I | F))) & (hole ahead)
              "RUN\n");
  cout << droid().run(prog) << '\n';
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
