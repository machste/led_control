#include "PCA9624.h"

#define CLI_CMDS_MAX 8
#define CLI_LINE_MAX 128
#define CLI_ARGS_MAX 16
#define PCA9624_I2C_ADDR 0x13
#define PCA9624_OE_PIN 2
#define PCA9624_OE_DEFAULT HIGH


class Command
{
public:
  typedef int (*RunCb)(Command *cmd, int argc, char *argv[]);

  Command(const char *name, RunCb cb=NULL) : name(name), run_cb(cb) {};
  const char *get_name(void) { return name; };
  virtual int run(int argc, char *argv[]);

private:
  const char *name;
  RunCb run_cb;

  void print_err(const char *msg);
};

int Command::run(int argc, char *argv[])
{
  if (run_cb != NULL) {
    return run_cb(this, argc, argv);
  }
  print_err("No run function define!");
  return -1;
}

void Command::print_err(const char *msg)
{
  Serial.print("ERR: ");
  Serial.print(name);
  Serial.print(": ");
  Serial.println(msg);
}


class Cli
{
public:
  Cli(const char *prompt, bool echo=true);
  bool add(Command *cmd);
  void start(void);
  void stop(void);
  void feed(char ch);
  
private:
  const char *prompt;
  size_t cmds_size;
  Command *cmds[CLI_CMDS_MAX];
  bool started;
  bool echo;
  char line[CLI_LINE_MAX];
  size_t line_pos;

  int run(void);
};

Cli::Cli(const char *prompt, bool echo)
  : prompt(prompt), cmds_size(0), started(false), echo(echo)
{}

bool Cli::add(Command *cmd)
{
  if (cmd == NULL) {
    return false;
  }
  if (cmds_size < CLI_CMDS_MAX - 1) {
    cmds[cmds_size++] = cmd;
  } else {
    return false;
  }
  return true;
}

void Cli::start(void)
{
  if (!started) {
    Serial.print(prompt);
    line_pos = 0;
    started = true;
  }
}

void Cli::stop(void)
{
  if (started) {
    started = false;
  }
}

void Cli::feed(char ch)
{
  if (!started) {
    return;
  }
  if (echo) {
    Serial.print(ch);
  }
  if (ch == '\r') {
    Serial.print('\n');
    line[line_pos] = '\0';
    run();
    Serial.print(prompt);
    line_pos = 0;
  } else {
    line[line_pos] = ch;
    if (line_pos < sizeof(line) - 1) {
      line_pos++;
    }
  }
}

int Cli::run(void)
{
  int argc = 0;
  char *argv[CLI_ARGS_MAX];
  size_t pos = 0;
  bool arg_started = false;
  while(pos < line_pos) {
    if (isspace(line[pos])) {
      if (arg_started) {
        // End of argument reached
        arg_started = false;
        line[pos] = '\0';
        // Is there space for an other argument?
        if (argc >= CLI_ARGS_MAX) {
          break;
        }
      }
    } else {
      if (!arg_started) {
        arg_started = true;
        argv[argc++] = line + pos;
      } 
    }
    pos++;
  }
  if (argc > 0) {
    // Search for command
    for (size_t i = 0; i < cmds_size; i++) {
      if (strcmp(argv[0], cmds[i]->get_name()) == 0) {
        return cmds[i]->run(argc, argv);
      }
    }
    Serial.print("ERR: ");
    Serial.print("Command '");
    Serial.print(argv[0]);
    Serial.println("' not found!");
  }
  return 0;
}


//Global Variables
Cli led_cli("> ");
PCA9624 pca9624(PCA9624_I2C_ADDR);


void reset_terminal(void)
{
  // Send escape sequence to reset terminal
  Serial.print("\033c");
}

int reset(Command *cmd, int argc, char *argv[])
{
  reset_terminal();
  return 0;
}

int echo(Command *cmd, int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    Serial.print(argv[i]);
    if (i < argc - 1) {
      Serial.print(" ");
    }
  }
  Serial.println();
  return 0;
}

int led(Command *cmd, int argc, char *argv[])
{
  if (argc != 3) {
    Serial.println("Usage: led <led-number> <duty-cycle>");
    return -1;
  }
  int led = atoi(argv[1]);
  if (led < 0 || led >= PCA9624::LED_MAX) {
    Serial.println("ERR: led: led-number is out of range! (0..7)");
    return -1;
  }
  int duty_cycle = atoi(argv[2]);
  if (duty_cycle < 0 || duty_cycle > PCA9624::DUTY_CYCLE_MAX) {
    Serial.println("ERR: led: duty-cycle is out of range! (0..255)");
    return -1;
  }
  if (!pca9624.set_pwm(led, (uint8_t)duty_cycle)) {
    Serial.println("ERR: led: Unable to set duty-cycle!");
  }
  return 0;
}

bool to_bool(const char *s, bool *success=NULL)
{
  if (success != NULL) *success = true;
  String bool_str(s);
  bool_str.trim();
  bool_str.toLowerCase();
  if (bool_str == "1" || bool_str == "true" || bool_str == "high" || bool_str == "on") {
    return true;
  } else if (bool_str == "0" || bool_str == "false" || bool_str == "low" || bool_str == "off") {
    return false;
  } else {
    if (success != NULL) *success = false;
  }
  return false;
}

int output(Command *cmd, int argc, char *argv[])
{
  if (argc != 2) {
    Serial.println("Usage: output <0|1>");
    return -1;
  }
  bool success;
  bool output_state = to_bool(argv[1], &success);
  if (!success) {
    Serial.println("ERR: output: Invalid argument!");
    return -1;
  }
  if (output_state) {
    digitalWrite(PCA9624_OE_PIN, LOW);
  } else {
    digitalWrite(PCA9624_OE_PIN, HIGH);
  }
  return 0;
}


void setup()
{
  // Setup serial
  Serial.begin(115200);
  // Send escape sequence to reset terminal
  reset_terminal();
  // Setup CLI for the LED Controller
  led_cli.add(new Command("reset", reset));
  led_cli.add(new Command("echo", echo));
  led_cli.add(new Command("led", led));
  led_cli.add(new Command("output", output));
  // Setup PCA9624 LED controller
  if (!pca9624.begin()) {
    Serial.println("pca9624: error!");
    while(true);
  }
  pca9624.clear_all();
  // Setup Output Enable OE of the PCA9624
  pinMode(PCA9624_OE_PIN, OUTPUT);
  digitalWrite(PCA9624_OE_PIN, PCA9624_OE_DEFAULT);
  Serial.println("led_control: ok.");
  // Start CLI
  led_cli.start();
}

void loop() {
  if (Serial.available() > 0) {
    led_cli.feed(Serial.read());
  }
}
