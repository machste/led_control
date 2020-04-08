

#define CLI_CMDS_MAX 8
#define CLI_LINE_MAX 128
#define CLI_ARGS_MAX 16


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
  int line_pos;
  char prev_ch;

  int run(void);
};

Cli::Cli(const char *prompt, bool echo)
  : prompt(prompt), echo(echo), cmds_size(0), started(false)
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
}

void Cli::start(void)
{
  if (!started) {
    Serial.print(prompt);
    line_pos = 0;
    prev_ch = 0;
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
  prev_ch = ch;
}

int Cli::run(void)
{
  int argc = 0;
  char *argv[CLI_ARGS_MAX];
  int pos = 0;
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
    for (int i = 0; i < cmds_size; i++) {
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

int rgb(Command *cmd, int argc, char *argv[])
{
  return 0;
}

void setup()
{
  Serial.begin(115200);
  // Send escape sequence to reset terminal
  reset_terminal();
  // Setup CLI for the LED Controller
  led_cli.add(new Command("reset", reset));
  led_cli.add(new Command("echo", echo));
  led_cli.add(new Command("rgb", rgb));
  Serial.println("led_control: ok.");
  // Start CLI
  led_cli.start();
}

void loop() {
  if (Serial.available() > 0) {
    led_cli.feed(Serial.read());
  }
}
