// -*- C++ -*-
// g++ -std=c++17 -Wall -g -o doit doit.cc
// ./doit 1 < input  # part 1
// ./doit 2 < input  # part 2

#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <chrono>
#include <thread>
#include <cassert>

using namespace std;

// Set visual to false for textual output only.  Setting it to true
// shows an animated game display for part 2 (run it in a terminal
// that understands ANSI-code sequences)
bool const visual = false;
char const esc = char(27);

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

struct arcade_game {
  // Code (read from stdin)
  CPU cpu;
  // Current score
  num score{0};
  // Where's the ball?
  coords ball{ 0, 0 };
  // Where's the paddle?
  coords paddle{ 0, 0 };
  // Display pixels
  map<coords, char> pixels;

  // Read CPU contents from stdin, optionally set to free play
  arcade_game(bool free_play);

  char at(coords const &c) const;

  // Transfer pixels from CPU to display
  void paint_screen();
  // Play the game (without free play just sets up the initial
  // configuration)
  void play();

  // Count pixels of a certain type
  int count_pixels(char pixel) const;

  // Print the display
  void print() const;
};

arcade_game::arcade_game(bool free_play) {
  if (free_play)
    cpu.memory[0] = 2;
}

char arcade_game::at(coords const &c) const {
  auto p = pixels.find(c);
  if (p == pixels.end())
    return ' ';
  return p->second;
}

void arcade_game::paint_screen() {
  while (!cpu.output_values.empty()) {
    int x = cpu.get_output();
    int y = cpu.get_output();
    if (x == -1 && y == 0)
      score = cpu.get_output();
    else {
      int tile = cpu.get_output();
      assert(tile >= 0 && tile <= 4);
      char pixel = " #+-o"[tile];
      if (pixel == 'o')
        ball = { x, y };
      else if (pixel == '-')
        paddle = { x, y };
      pixels.insert_or_assign({ x, y }, pixel);
    }
  }
}

void arcade_game::play() {
  if (visual)
    // Clear screen
    cout << esc << "[2J";
  while (!cpu.run()) {
    paint_screen();
    if (visual) {
      print();
      this_thread::sleep_for(10ms);
    }
    int px = paddle.first;
    int bx = ball.first;
    // Follow the bouncing ball
    cpu.give_input(px == bx ? 0 : (px < bx ? +1 : -1));
  }
  paint_screen();
  if (visual)
    print();
}

int arcade_game::count_pixels(char pixel) const {
  int result = 0;
  for (auto const &i : pixels)
    if (i.second == pixel)
      ++result;
  return result;
}

enum color { red = 31, green = 32, blue = 34, yellow = 33 };

void reset_color() {
  if (visual)
    cout << esc << "[39m" << esc << "[49m";
}

void set_color(color c) {
  if (visual)
    cout << esc << '[' << int(c) << 'm';
}

char color(char pixel) {
  switch (pixel) {
  case '#': set_color(blue); break;
  case '-': set_color(green); break;
  case 'o': set_color(yellow); break;
  case '+': set_color(red); break;
  }
  return pixel;
}

void arcade_game::print() const {
  coords ll = pixels.begin()->first;
  coords ur = ll;
  for (auto const &i : pixels) {
    ll = min(ll, i.first);
    ur = max(ur, i.first);
  }
  reset_color();
  for (int y = ll.second; y <= ur.second; ++y) {
    if (visual) {
      cout << esc << '[' << y << ";0H";
      cout << esc << "[2K";
    }
    for (int x = ll.first; x <= ur.first; ++x)
      cout << color(at({ x, y }));
    cout << '\n';
  }
  set_color(green);
  cout << score << '\n';
  reset_color();
}

void part1() {
  arcade_game game(false);
  game.play();
  cout << game.count_pixels('+') << '\n';
}

void part2() {
  arcade_game game(true);
  game.play();
  cout << game.score << '\n';
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
