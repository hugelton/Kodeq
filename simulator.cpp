#include "parser.hpp"
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

// Terminal control constants and helper functions for Unix-like systems
namespace terminal {
// ANSI escape codes for terminal control
const std::string CLEAR_SCREEN = "\033[2J\033[H";
const std::string CURSOR_HOME = "\033[H";
const std::string HIDE_CURSOR = "\033[?25l";
const std::string SHOW_CURSOR = "\033[?25h";

// Set terminal to raw mode (no need to press Enter after each keystroke)
termios original_termios;

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &original_termios);
  termios raw = original_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  std::cout << HIDE_CURSOR;
}

void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
  std::cout << SHOW_CURSOR;
}

// Read a single character without blocking
char readChar() {
  char c;
  read(STDIN_FILENO, &c, 1);
  return c;
}
} // namespace terminal

// Virtual OLED display
class OLEDDisplay {
private:
  static const int DISPLAY_WIDTH = 12;
  static const int DISPLAY_HEIGHT = 4;
  std::vector<std::string> buffer;
  std::deque<std::string> fullContent;
  int scrollPosition = 0;
  int cursorX = 0;
  int cursorY = 0;
  std::string currentLine;
  bool editMode = false;

public:
  OLEDDisplay() {
    // Initialize empty buffer
    buffer.resize(DISPLAY_HEIGHT, std::string(DISPLAY_WIDTH - 1, ' '));

    // Initial welcome message
    fullContent.push_back("KODEQ v1.0");
    fullContent.push_back("Ready.");
    fullContent.push_back("");
    fullContent.push_back("");

    updateBuffer();
  }

  void addLine(const std::string &line) {
    // Add new content line
    fullContent.push_back(line);

    // Keep a maximum history (prevent excessive memory use)
    while (fullContent.size() > 100) {
      fullContent.pop_front();
    }

    // Auto-scroll to bottom when new content is added
    scrollPosition =
        std::max(0, static_cast<int>(fullContent.size()) - DISPLAY_HEIGHT);
    updateBuffer();
  }

  void scrollUp() {
    if (scrollPosition > 0) {
      scrollPosition--;
      updateBuffer();
    }
  }

  void scrollDown() {
    if (scrollPosition <
        static_cast<int>(fullContent.size()) - DISPLAY_HEIGHT) {
      scrollPosition++;
      updateBuffer();
    }
  }

  void startEdit() {
    editMode = true;
    currentLine = "";
    cursorX = 0;
  }

  void endEdit() { editMode = false; }

  std::string getEditLine() { return currentLine; }

  void handleKeypress(char key) {
    if (!editMode)
      return;

    // Handle backspace
    if (key == 127 || key == 8) {
      if (cursorX > 0) {
        currentLine.erase(cursorX - 1, 1);
        cursorX--;
      }
    }
    // Handle printable characters
    else if (key >= 32 && key <= 126) {
      if (cursorX < DISPLAY_WIDTH - 1) {
        if (cursorX == currentLine.length()) {
          currentLine += key;
        } else {
          currentLine.insert(cursorX, 1, key);
        }
        cursorX++;
      }
    }
  }

  void updateBuffer() {
    // Clear buffer
    for (int i = 0; i < DISPLAY_HEIGHT; i++) {
      buffer[i] = std::string(DISPLAY_WIDTH - 1, ' ');
    }

    // Fill with visible content
    for (int i = 0; i < DISPLAY_HEIGHT; i++) {
      int contentIdx = scrollPosition + i;
      if (contentIdx < static_cast<int>(fullContent.size())) {
        std::string line = fullContent[contentIdx];
        if (line.length() > DISPLAY_WIDTH - 1) {
          // Truncate and leave space for scroll indicator
          buffer[i] = line.substr(0, DISPLAY_WIDTH - 1);
        } else {
          // Pad with spaces
          buffer[i] =
              line + std::string(DISPLAY_WIDTH - 1 - line.length(), ' ');
        }
      }
    }
  }

  void render() {
    // Clear screen and position cursor at home
    std::cout << terminal::CLEAR_SCREEN;

    // Draw OLED frame with ASCII characters instead of Unicode box drawing
    std::cout << "+" << std::string(DISPLAY_WIDTH, '-') << "+\n";

    // Draw content
    for (int i = 0; i < DISPLAY_HEIGHT; i++) {
      std::cout << "|" << buffer[i];

      // Show scroll indicators if needed
      if (i == 0 && scrollPosition > 0) {
        std::cout << "^";
      } else if (i == DISPLAY_HEIGHT - 1 &&
                 scrollPosition + DISPLAY_HEIGHT <
                     static_cast<int>(fullContent.size())) {
        std::cout << "v";
      } else {
        std::cout << " ";
      }

      std::cout << "|\n";
    }

    // Draw bottom frame
    std::cout << "+" << std::string(DISPLAY_WIDTH, '-') << "+\n";

    // Draw edit line if in edit mode
    if (editMode) {
      std::string displayLine = currentLine;
      if (displayLine.length() > DISPLAY_WIDTH - 1) {
        displayLine = displayLine.substr(0, DISPLAY_WIDTH - 1);
      } else {
        displayLine +=
            std::string(DISPLAY_WIDTH - 1 - displayLine.length(), ' ');
      }

      std::cout << "> " << displayLine << "\n";
      // Position cursor
      std::cout << "\033[A\033[" << cursorX + 2
                << "G"; // Move up one line and to cursorX+2 column
    } else {
      std::cout << "Press 'e' to enter edit mode, q to quit\n";
    }

    std::cout.flush();
  }
};

// Handle KODEQ commands and updates
void processCommand(const std::string &command, KodeqParser &parser,
                    OLEDDisplay &display) {
  // Handle special commands
  if (command == "exit" || command == "quit") {
    terminal::disableRawMode();
    exit(0);
  }

  if (command == "cls" || command == "clear") {
    // Clear display by adding empty lines
    for (int i = 0; i < 10; i++) {
      display.addLine("");
    }
    return;
  }

  if (command == "help") {
    display.addLine("KODEQ Commands:");
    display.addLine("$X=VAL - Set var");
    display.addLine("$X=MODULE - New mod");
    display.addLine("$X.P=VAL - Set param");
    display.addLine("tick - Advance");
    display.addLine("exit - Quit");
    return;
  }

  if (command == "vars") {
    // Capture and display variables
    // This is simplified - a proper implementation would redirect stdout
    parser.printVariables();
    display.addLine("See terminal for variables");
    return;
  }

  if (command == "tick" || command == "t") {
    parser.advanceTick();
    display.addLine("Tick: " + std::to_string(parser.getTick()));
    return;
  }

  // Process normal KODEQ commands
  display.addLine("> " + command);
  bool success = parser.parseLine(command);

  if (!success) {
    display.addLine("Error: Invalid command");
  }
}

// Main function
int main() {
  KodeqParser parser;
  OLEDDisplay display;

  // Setup terminal for immediate key input
  terminal::enableRawMode();

  // Main application loop
  bool running = true;
  bool editing = false;

  while (running) {
    // Render the current display state
    display.render();

    // Handle user input
    char c = terminal::readChar();

    if (editing) {
      if (c == '\n' || c == '\r') {
        // Process entered command
        std::string command = display.getEditLine();
        display.endEdit();
        editing = false;

        if (!command.empty()) {
          processCommand(command, parser, display);
        }
      } else if (c == 27) { // ESC key
        // Cancel edit
        display.endEdit();
        editing = false;
      } else {
        // Add character to edit line
        display.handleKeypress(c);
      }
    } else {
      // Not in edit mode
      switch (c) {
      case 'q':
        running = false;
        break;
      case 'e':
        editing = true;
        display.startEdit();
        break;
      case 'k':
        display.scrollUp();
        break;
      case 'j':
        display.scrollDown();
        break;
      case 't':
        parser.advanceTick();
        display.addLine("Tick: " + std::to_string(parser.getTick()));
        break;
      }
    }
  }

  // Restore terminal settings
  terminal::disableRawMode();

  return 0;
}