// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
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
  ifstream strm("input");
  string line;
  while (getline(strm, line)) {
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

// The exploratory droid
struct droid {
  // Code read from stdin
  CPU cpu;

  // Explore the ship and get into the cockpit
  void solve();
};

void droid::solve() {
  // How to move around
  map<pair<string, string>, string> movement;
  // What's available
  vector<string> items;
  // Don't pick up these
  set<string> forbidden{ "molten lava",
                         "giant electromagnet",
                         "infinite loop",
                         "photons",
                         "escape pod" };
  map<string, string> opposite{ { "north", "south" },
                                { "south", "north" },
                                { "east", "west" },
                                { "west", "east" } };
  // Output from the last command
  list<string> out;
  // The current room
  string loc;
  // Doors in the current room
  vector<string> doors;
  // Things in the current room
  vector<string> things;
  // Run the droid's program, collect output
  auto show =
    [&] {
      cpu.run();
      while (cpu.has_output()) {
        char ch = cpu.get_output();
        if (ch == '\n' || out.empty())
          out.push_back("");
        else
          out.back().push_back(ch);
      }
      while (!out.empty() && out.back().empty())
        out.pop_back();
      for (auto const &s : out)
        cout << s << '\n';
    };
  // Give a command to the droid, collect output
  auto say =
    [&](string const &s) {
      cout << s << '\n';
      for (char c : s)
        cpu.give_input(c);
      cpu.give_input('\n');
      out.clear();
      show();
    };
  // Collect a list of some sort in the room description
  auto read_list =
    [&](vector<string> &lst) {
      while (!out.empty() && out.front().find("- ") != string::npos) {
        lst.push_back(out.front().substr(2));
        out.pop_front();
      }
    };
  // Parse the description of the current room
  auto room =
    [&] {
      while (true) {
        string s = out.front();
        out.pop_front();
        if (s.empty())
          continue;
        if (s[0] == '=') {
          loc = s.substr(3, s.length() - 6);
          doors.clear();
          things.clear();
        } else if (s == "Doors here lead:")
          read_list(doors);
        else if (s == "Items here:")
          read_list(things);
        else if (s == "Command?")
          break;
      }
      out.clear();
    };
  // Build a connection in the movement map
  auto link =
    [&](string const &from, string const &went, string const &to) {
      if (!movement.count({ from, went })) {
        movement.emplace(make_pair(from, went), to);
        movement.emplace(make_pair(to, opposite[went]), from);
      }
    };
  set<string> visited;
  // Explore the ship and collect items
  auto explore =
    [&](string const &from, string const &went, auto self) -> string {
      room();
      string here = loc;
      if (from != "")
        link(from, went, here);
      if (visited.count(here)) {
        // Already found this room
        if (went != "")
          say(opposite[went]);
        return here;
      }
      visited.insert(here);
      // Pick up useful items
      for (auto const &thing : things)
        if (!forbidden.count(thing)) {
          say("take " + thing);
          if (find(items.begin(), items.end(), thing) == items.end()) {
            items.push_back(thing);
            say("inv");
          }
        }
      assert(!doors.empty());
      // Explore all the rooms
      auto here_doors = doors;
      for (auto const &door: here_doors) {
        if (here == "Security Checkpoint" && door == "north")
          // Leave security for the end
          continue;
        if (!movement.count({ here, door })) {
          say(door);
          self(here, door, self);
        }
      }
      if (went != "")
        say(opposite[went]);
      return here;
    };
  show();
  string start = explore("", "", explore);
  // Go to security
  auto find_path =
    [&](string const &goal, vector<string> &path, auto self) -> bool {
      auto current = path.back();
      for (auto const &link : movement) {
        auto const & [from_door, to] = link;
        auto const & [from, door] = from_door;
        if (from != current)
          // Not relevant for the room at the end of the path
          continue;
        if (find(path.begin(), path.end(), to) != path.end())
          // Already somewhere in the path
          continue;
        // Consider going through that door
        path.push_back(door);
        path.push_back(to);
        if (to == goal)
          // Reached the goal, path has the full list of what to do
          return true;
        if (self(goal, path, self))
          // Eventually reached the goal
          return true;
        // Backtrack
        path.pop_back();
        path.pop_back();
      }
      return false;
    };
  vector<string> path{ start };
  find_path("Security Checkpoint", path, find_path);
  for (size_t i = 1; i < path.size(); i += 2)
    say(path[i]);
  // Drop everything
  say("inv");
  for (auto const &item : items)
    say("drop " + item);
  // Figure out how to get past the pressure-sensitive floor
  int holding = 0;
  for (int i = 1; i < (1 << items.size()); ++i) {
    say("north");
    bool worked = true;
    for (auto const &s : out)
      if (s.find("ejected back to the checkpoint") != string::npos) {
        worked = false;
        break;
      }
    if (worked)
      break;
    // Gray code, so only need to pick up or drop one item for each
    // attempt
    int next_holding = i ^ (i >> 1);
    int diff = holding ^ next_holding;
    int item_num;
    for (item_num = 0; diff != (1 << item_num); ++item_num)
      ;
    if (holding & diff)
      say("drop " + items[item_num]);
    else
      say("take " + items[item_num]);
    holding = next_holding;
  }
}

void part1() { droid().solve(); }
void part2() { cout << "Align the Warp Drive!\n"; }

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
