// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <optional>
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

coords min(coords const &c1, coords const &c2) {
  return { min(c1.first, c2.first), min(c1.second, c2.second) };
}

coords max(coords const &c1, coords const &c2) {
  return { max(c1.first, c2.first), max(c1.second, c2.second) };
}

// Offset indexes of these by 1 for input to droid (1...4)
vector<coords> const dirs{
  { 0, +1 }, { 0, -1 }, { -1, 0 }, { +1, 0 }
};

struct droid {
  // Brains of the droid (read from stdin)
  CPU cpu;
  // The explored space
  map<coords, char> world;
  // Where the oxygen generator is
  optional<coords> generator;

  // Read CPU code from stdin, then explore the world
  droid();

  // Return what's at c, or '?' if unknown
  char at(coords const &c) const;

  // Run the program to explore the world
  void explore(coords const &pos);

  // Breadth first search from start looking for an optional target.
  // Returns either the length of the shortest path if the target
  // exists and is reachable, otherwise the maximum search depth.
  int bfs(coords const &start, optional<coords> const &target) const;
};

droid::droid() {
  world.emplace(coords{ 0, 0 }, 'D');
  explore({ 0, 0 });
}

char droid::at(coords const &c) const {
  auto p = world.find(c);
  if (p == world.end())
    return '?';
  return p->second;
}

void droid::explore(coords const &pos) {
  for (size_t i = 0; i < dirs.size(); ++i) {
    coords next = pos + dirs[i];
    if (at(next) != '?')
      // Already know what's there
      continue;
    auto step = [&](int dir) {
                  cpu.give_input(dir + 1);
                  cpu.run();
                  return cpu.get_output();
                };
    int status = step(i);
    if (status == 0)
      // Hit a wall and didn't move
      world.emplace(next, '#');
    else {
      if (status == 2) {
        assert(!generator);
        generator = next;
      }
      world.emplace(next, status == 2 ? 'O' : '.');
      explore(next);
      // Back up
      assert(dirs[i ^ 1] + dirs[i] == coords({ 0, 0 }));
      int status_back = step(i ^ 1);
      assert(status_back != 0);
    }
  }
}

int droid::bfs(coords const &start, optional<coords> const &target) const {
  set<coords> visited;
  list<pair<int, coords>> frontier;
  auto visit = [&](int depth, coords const &c) {
                 if (visited.count(c) || at(c) == '#')
                   return;
                 visited.insert(c);
                 frontier.emplace_back(depth, c);
               };
  visit(0, start);
  int max_depth = 0;
  while (!frontier.empty()) {
    auto [depth, c] = frontier.front();
    frontier.pop_front();
    max_depth = max(max_depth, depth);
    if (target && c == *target)
      return depth;
    for (auto const &dir : dirs)
      visit(depth + 1, c + dir);
  }
  return max_depth;
}

void part1() {
  droid bot;
  cout << bot.bfs({ 0, 0 }, *bot.generator) << '\n';
}

void part2() {
  droid bot;
  cout << bot.bfs(*bot.generator, nullopt) << '\n';
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
