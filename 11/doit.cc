// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
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
    if (size_t(addr) > memory.size())
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
  // Get output and remove
  num get_output();
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
  }
  return true;
}

bool CPU::run() {
  while (execute())
    ;
  return halted;
}

using coords = pair<int, int>;

coords operator+(coords const &c1, coords const &c2) {
  return { c1.first + c2.first, c1.second + c2.second };
}

coords min(coords const &c1, coords const &c2) {
  return { min(c1.first, c2.first), min(c1.second, c2.second) };
}

coords max(coords const &c1, coords const &c2) {
  return { max(c1.first, c2.first), max(c1.second, c2.second) };
}

vector<coords> const dirs{
  { 0, +1 }, { -1, 0 }, { 0, -1 }, { +1, 0 }
};

struct robot {
  // Brains of the robot (read from stdin)
  CPU cpu;
  // Where the robot is
  coords pos{ 0, 0 };
  // Direction the robot is facing
  int dir{0};
  // What the robot has painted
  map<coords, int> painted;

  // Read CPU contents from stdin, set initial panel color
  robot(int initial_color);

  // What's on the panel at c?
  int at(coords const &c) const;

  // Run the program to paint the hull
  int paint();

  // Display what was painted
  void print() const;
};

robot::robot(int initial_color) {
  painted.emplace(pos, initial_color);
}

int robot::at(coords const &c) const {
  auto p = painted.find(c);
  if (p == painted.end())
    return 0;
  return p->second;
}

int robot::paint() {
  cpu.give_input(at(pos));
  while (!cpu.run()) {
    int color = cpu.get_output();
    int turn = cpu.get_output();
    painted.insert_or_assign(pos, color);
    if (turn)
      dir += 3;
    else
      ++dir;
    dir %= 4;
    pos = pos + dirs[dir];
    cpu.give_input(at(pos));
  }
  return painted.size();
}

void robot::print() const {
  coords ll = painted.begin()->first;
  coords ur = ll;
  for (auto const &i : painted) {
    ll = min(ll, i.first);
    ur = max(ur, i.first);
  }
  for (int y = ur.second; y >= ll.second; --y) {
    for (int x = ll.first; x <= ur.first; ++x)
      cout << (at({ x, y }) ? '#' : ' ');
    cout << '\n';
  }
}

void part1() { cout << robot(0).paint() << '\n'; }

void part2() {
  robot bot(1);
  bot.paint();
  bot.print();
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
