#include "environment.hpp"
#include "parser.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

// ターミナル制御のためのユーティリティ関数
namespace terminal {
// ANSI エスケープコード
const std::string CLEAR_SCREEN = "\033[2J\033[H";
const std::string CURSOR_HOME = "\033[H";
const std::string HIDE_CURSOR = "\033[?25l";
const std::string SHOW_CURSOR = "\033[?25h";
const std::string RESET_COLOR = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";

// 元のターミナル設定を保存
termios original_termios;

// rawモードに設定（キー入力を即時処理するため）
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &original_termios);
  termios raw = original_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  std::cout << HIDE_CURSOR;
}

// 元のターミナル設定に戻す
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
  std::cout << SHOW_CURSOR;
}

// ノンブロッキングでキー入力を取得
int readKey() {
  char c;
  int result = read(STDIN_FILENO, &c, 1);
  if (result < 0) {
    return -1;
  }
  return c;
}
} // namespace terminal

// シミュレータクラス
class ReeliaSimulator {
private:
  Environment env;
  Parser parser;

  // 現在の入力行
  std::string currentLine;

  // 入力履歴
  std::vector<std::string> history;
  int historyIndex;

  // 自動的にティックを進めるかどうか
  bool autoTick;
  int tickInterval; // ミリ秒

  // クロック表示
  void displayClock() {
    int tick = env.getTickCount();
    int beat = tick / 4;
    int subBeat = tick % 4;

    std::cout << terminal::BOLD << terminal::CYAN;
    std::cout << "Tick: " << tick << " (";
    std::cout << beat + 1 << "." << subBeat + 1;
    std::cout << ")" << terminal::RESET_COLOR << std::endl;
  }

  // ヘルプ表示
  void displayHelp() {
    std::cout << terminal::BOLD << "REELIA Live Coding Environment"
              << terminal::RESET_COLOR << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  $var = @class       - Create instance" << std::endl;
    std::cout << "  $obj.attr = value   - Set attribute" << std::endl;
    std::cout << "  $var = $obj.attr    - Get attribute" << std::endl;
    std::cout << "  $obj.method()       - Call method" << std::endl;
    std::cout << "  cmd1 | cmd2         - Parallel execution" << std::endl;
    std::cout << std::endl;
    std::cout << "Keyboard shortcuts:" << std::endl;
    std::cout << "  Ctrl+T         - Manual tick" << std::endl;
    std::cout << "  Ctrl+A         - Toggle auto-tick" << std::endl;
    std::cout << "  Ctrl+S         - Change tick interval" << std::endl;
    std::cout << "  Ctrl+L         - Clear screen" << std::endl;
    std::cout << "  Ctrl+H (?)     - Show this help" << std::endl;
    std::cout << "  Ctrl+D         - Dump variables" << std::endl;
    std::cout << "  Ctrl+X         - Exit" << std::endl;
    std::cout << std::endl;
  }

  // 現在の状態表示
  void displayStatus() {
    std::cout << terminal::BOLD;
    std::cout << "Auto-tick: " << (autoTick ? "ON" : "OFF");
    if (autoTick) {
      std::cout << " (" << tickInterval << "ms)";
    }
    std::cout << terminal::RESET_COLOR << std::endl;
  }

  // 変数ダンプ
  void dumpVariables() {
    std::cout << terminal::BOLD << terminal::YELLOW;
    std::cout << "Variables:" << terminal::RESET_COLOR << std::endl;
    env.dumpVariables();
  }

  // 画面クリア
  void clearScreen() {
    std::cout << terminal::CLEAR_SCREEN;
    displayStatus();
    displayClock();
  }

  // 入力処理
  void handleInput() {
    int c = terminal::readKey();
    if (c == -1)
      return;

    // Ctrl+キー
    if (c < 32) {
      switch (c) {
      case 4: // Ctrl+D
        dumpVariables();
        break;
      case 8:   // Backspace
      case 127: // Delete
        if (!currentLine.empty()) {
          currentLine.pop_back();
        }
        break;
      case 10: // Enter
      case 13: // Carriage return
        if (!currentLine.empty()) {
          // 履歴に追加
          history.push_back(currentLine);
          historyIndex = history.size();

          // コマンド実行
          std::cout << terminal::GREEN << "> " << currentLine
                    << terminal::RESET_COLOR << std::endl;
          parser.parseLine(currentLine);
          currentLine.clear();
        }
        break;
      case 12: // Ctrl+L
        clearScreen();
        break;
      case 1: // Ctrl+A
        autoTick = !autoTick;
        displayStatus();
        break;
      case 19: // Ctrl+S
      {
        autoTick = false;
        displayStatus();
        std::cout << "Enter tick interval (ms): ";
        std::string input;
        // rawモードを一時的に解除
        terminal::disableRawMode();
        std::getline(std::cin, input);
        terminal::enableRawMode();
        try {
          tickInterval = std::stoi(input);
          autoTick = true;
          displayStatus();
        } catch (...) {
          std::cout << "Invalid input" << std::endl;
        }
      } break;
      case 20: // Ctrl+T
        parser.tick();
        displayClock();
        break;
      case 104: // Ctrl+H (修正：値を8から104に変更)
        displayHelp();
        break;
      case 24: // Ctrl+X
        exit(0);
        break;
      }
    }
    // 通常の文字
    else if (c >= 32 && c < 127) {
      // '?' キーでもヘルプを表示
      if (c == 63) {
        displayHelp();
      } else {
        currentLine += static_cast<char>(c);
      }
    }

    // 現在の入力行を表示
    std::cout << "\r" << std::string(80, ' ') << "\r> " << currentLine
              << std::flush;
  }

public:
  ReeliaSimulator()
      : parser(env), historyIndex(0), autoTick(false), tickInterval(250) {
    // 初期化
  }

  void run() {
    // ターミナル設定
    terminal::enableRawMode();

    // 初期表示
    clearScreen();
    displayHelp();

    // 最初のプロンプト
    std::cout << "> " << std::flush;

    // 前回のティック時間
    auto lastTickTime = std::chrono::steady_clock::now();

    // メインループ
    while (true) {
      // 入力処理
      handleInput();

      // 自動ティック
      if (autoTick) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - lastTickTime)
                           .count();

        if (elapsed >= tickInterval) {
          parser.tick();
          displayClock();
          lastTickTime = now;

          // 入力行を再表示
          std::cout << "\r" << std::string(80, ' ') << "\r> " << currentLine
                    << std::flush;
        }
      }

      // CPUの負荷を減らすために少し待機
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
};

// メイン関数
int main() {
  std::cout << "Reelia Live Coding Environment starting..." << std::endl;

  // シミュレータの作成と実行
  ReeliaSimulator simulator;

  try {
    simulator.run();
  } catch (const std::exception &e) {
    // 例外発生時にターミナル設定を元に戻す
    terminal::disableRawMode();
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}