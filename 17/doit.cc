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

// The vacuum robot, whom I've named Hoover
struct hoover {
  // Brains of robot (read from stdin)
  CPU cpu;
  // What the scaffolding looks like
  vector<string> scaffolding;
  // Where the robot starts
  optional<coords> start;
  // Which direction the robot is facing
  coords facing{ 0, 0 };

  // Read CPU code, get picture of scaffolding, find where robot is.
  // If wake_up is true, the robot is awake and ready for movement
  // commands afterwards
  hoover(bool wake_up);

  char at(coords const &c) const;

  // Compute camera alignment parameters
  int alignment() const;

  // Find a path for traversing the scaffold
  seq find_path() const;
  // Cut up the traversal path into new movement rules
  optional<array<string, 4>> movement_rules() const;
  // Find, path, compute movement rules, clean, return dust collected
  int clean();
};

hoover::hoover(bool wake_up) {
  if (wake_up)
    cpu.memory[0] = 2;
  cpu.run();
  bool starting_line = true;
  while (cpu.has_output()) {
    char c = cpu.get_output();
    if (c == '\n') {
      if (starting_line)
        // Two newlines in a row is end of camera scan
        break;
      starting_line = true;
    } else {
      if (starting_line)
        scaffolding.push_back("");
      scaffolding.back().push_back(c);
      starting_line = false;
      if (c != '.' && c != '#') {
        start = { scaffolding.back().length() - 1, scaffolding.size() - 1 };
        if (c == '^') facing = { 0, -1 };
        else if (c == '<') facing = { -1, 0 };
        else if (c == '>') facing = { +1, 0 };
        else { assert(c == 'v'); facing = { 0, +1 }; }
      }
    }
  }
  assert(start);
}

char hoover::at(coords const &c) const {
  if (c.second < 0 || c.second >= int(scaffolding.size()))
    // Off the edge
    return '.';
  auto const &row = scaffolding[c.second];
  if (c.first < 0 || c.first >= int(row.length()))
    // Off the edge
    return '.';
  return row[c.first];
}

int hoover::alignment() const {
  int result = 0;
  for (size_t i = 0; i < scaffolding.size(); ++i)
    for (size_t j = 0; j < scaffolding[i].length(); ++j) {
      if (at({ j, i }) == '#') {
        bool intersection = true;
        for (auto const &dir : dirs)
          if (at(coords{ j, i } + dir) != '#')
            intersection = false;
        if (intersection)
          result += i * j;
      }
    }
  return result;
}

seq hoover::find_path() const {
  // To chain everything together, I suspect each of the A, B, C
  // subsequences is going to be like R/L,#,R/L,#,R/L,#, and so there
  // needs to be an initial turn (and a final straight movement).
  assert(start);
  coords pos = *start;
  coords dir = facing;
  seq sequence;
  assert(at(pos + dir) != '#');
  if (at(pos + left(dir)) == '#') {
    sequence.push_back("L");
    dir = left(dir);
  } else {
    assert(at(pos + right(dir)) == '#');
    sequence.push_back("R");
    dir = right(dir);
  }
  // The robot should be the start of the track
  assert(at(pos + -1 * dir) == '.');
  // Strategy is just to drive forward as far as possible, then make
  // the only turn that wouldn't backtrack.  T junctions would be bad
  // news, but I doubt they occur.
  while (true) {
    int steps = 0;
    while (at(pos + (steps + 1) * dir) == '#')
      ++steps;
    assert(steps > 0);
    sequence.back() += ',' + to_string(steps);
    pos = pos + steps * dir;
    if (at(pos + left(dir)) == '#') {
      sequence.push_back("L");
      dir = left(dir);
    } else if (at(pos + right(dir)) == '#') {
      sequence.push_back("R");
      dir = right(dir);
    } else
      // End of the line
      break;
  }
  return sequence;
}

// It's convenient to include a trailing comma here
string concat(seq const &sequence) {
  string result;
  for (auto const &s : sequence) {
    result += s;
    result.push_back(',');
  }
  return result;
}

// Try to make a path from three candidate parts.  Return how to do it
// (reversed), or nullopt if it doesn't work for some reason
optional<vector<int>> build(string path, array<string, 3> const &parts) {
  if (path.empty())
    // Success
    return vector<int>();
  for (int i = 0; i  < 3; ++i)
    if (path.substr(0, parts[i].length()) == parts[i]) {
      auto rest = build(path.substr(parts[i].length()), parts);
      // Only get at most 10 of A, B, C for the top-level subroutine
      if (rest && rest->size() < 10) {
        // Pieces fit, and within the length limit
        rest->push_back(i);
        return rest;
      }
    }
  return nullopt;
}

optional<array<string, 4>> hoover::movement_rules() const {
  // Find solution path, then brute-force cut it up to meet the length
  // constraints
  auto path = find_path();
  // What parts could be used for making the path?
  set<string> choices;
  for (size_t i = 0; i + 1 < path.size(); ++i)
    for (size_t j = i + 1; j < path.size(); ++j) {
      auto candidate = concat(seq(path.begin() + i, path.begin() + j));
      // 20 character limit on subroutine sizes for A, B, C
      if (candidate.length() - 1 <= 20)
        choices.insert(candidate);
    }
  assert(choices.size() >= 3);
  // Stupidly try all possible cutting combinations
  string wanted = concat(path);
  for (auto a = choices.begin(); a != choices.end(); ++a) {
    // One minor optimization...  The build routine cuts things off
    // the front to try to reach an empty string, so it fails early if
    // I haven't picked a valid thing for the start.  But if none of
    // the A, B, C choices are valid endings it can't work.  So force
    // A to be a valid ending.  Note though that the B and C loops
    // then have to scan everything.
    if (wanted.substr(wanted.length() - a->length()) != *a)
      continue;
    for (auto b = choices.begin(); next(b) != choices.end(); ++b)
      for (auto c = next(b); c != choices.end(); ++c) {
        array<string, 3> parts{ *a, *b, *c };
        if (auto how = build(wanted, parts)) {
          // Solution found, package it up nicely
          array<string, 4> sol;
          for (auto i = how->rbegin(); i != how->rend(); ++i) {
            sol[0].push_back('A' + *i);
            sol[0].push_back(',');
          }
          // sol[0] is the top level routine; 1, 2, 3 are for
          // subroutines A, B, C
          for (int i = 0; i < 3; ++i)
            sol[i + 1] = parts[i];
          // Chop off all the trailing commas
          for (int i = 0; i < 4; ++i)
            sol[i].pop_back();
          return sol;
        }
      }
  }
  return nullopt;
}

int hoover::clean() {
  auto rules = movement_rules();
  assert(rules);
  // The robot prompts for something
  auto listen = [&] { cpu.clear_output(); };
  // Answer the prompt
  auto say = [&](string const &s) {
               for (char c : s)
                 cpu.give_input(c);
               cpu.give_input('\n');
               return cpu.run();
             };
  // Provide movement routines
  for (int i = 0; i < 4; ++i) {
    listen();
    say((*rules)[i]);
  }
  // Continuous video feed?
  listen();
  say("n");
  return cpu.last_output();
}

void part1() { cout << hoover(false).alignment() << '\n'; }
void part2() { cout << hoover(true).clean() << '\n'; }

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
