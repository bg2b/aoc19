// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
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
  // Do input instructions block?
  bool blocking{true};

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
  // Is input available?
  bool has_input() const { return !input_values.empty(); }
  // Save an output value
  void save_output(num v) { output_values.push_back(v); }
  // Return whatever the last output value was
  num last_output() const;
  // Is output available?
  size_t has_output() const { return output_values.size(); }
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
  if (!blocking && input_values.empty())
    return -1;
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
    { if (blocking && input_values.empty()) {
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

using coords = pair<int, int>;

coords operator+(coords const &c1, coords const &c2) {
  return { c1.first + c2.first, c1.second + c2.second };
}

coords operator*(int sc, coords const &c) {
  return { sc * c.first, sc * c.second };
}

// Note that y increases downwards, so up is 0 -1, and a left turn
// from that is -1 0, and a right turn is 1 0
coords left(coords const &dir) { return { dir.second, -dir.first }; }
coords right(coords const &dir) { return { -dir.second, dir.first }; }

vector<coords> const dirs{
  { 0, +1 }, { 0, -1 }, { -1, 0 }, { +1, 0 }
};

using seq = vector<string>;

// The network
struct network {
  // All the CPUs
  vector<CPU> cpus;
  // The packet the NAT received last, if any
  optional<num> nat_x;
  optional<num> nat_y;
  // The last y that the NAT sent when the network was found to be
  // idle
  optional<num> last_nat_y;
  // CPU instruction pointers, for detecting idling
  set<vector<int>> idling_ips;

  // Read CPU from stdin
  network();

  // Run, return either the first y that the NAT sees (first_nat true)
  // or the first y that is sent twice in a row to CPU 0 (first_nat
  // false)
  num run(bool first_nat);
};

network::network() {
  cpus.emplace_back();
  cpus.back().blocking = false;
  while (cpus.size() < 50)
    cpus.emplace_back(cpus.back());
  for (size_t addr = 0; addr < cpus.size(); ++addr)
    cpus[addr].give_input(addr);
}

num network::run(bool first_nat) {
  while (true) {
    vector<int> ips;
    for (auto &cpu : cpus) {
      cpu.execute();
      ips.push_back(cpu.ip);
      while (cpu.has_output() >= 3) {
        // Full packet available
        auto addr = cpu.get_output();
        auto x = cpu.get_output();
        auto y = cpu.get_output();
        if (addr == 255) {
          if (first_nat)
            return y;
          nat_x = x;
          nat_y = y;
        } else {
          assert(addr < num(cpus.size()));
          cpus[addr].give_input(x);
          cpus[addr].give_input(y);
        }
      }
    }
    for (auto const &cpu : cpus)
      if (cpu.has_input() || cpu.has_output())
        // Stuff is happening
        idling_ips.clear();
    if (idling_ips.count(ips)) {
      // Looks like the network isn't making progress
      if (nat_x && nat_y) {
        // The NAT received something
        if (last_nat_y && *nat_y == *last_nat_y)
          return *nat_y;
        last_nat_y = *nat_y;
        cpus[0].give_input(*nat_x);
        cpus[0].give_input(*nat_y);
        nat_x.reset();
        nat_y.reset();
      }
      idling_ips.clear();
    } else
      idling_ips.insert(ips);
  }
}

void part1() { cout << network().run(true) << '\n'; }
void part2() { cout << network().run(false) << '\n'; }

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
