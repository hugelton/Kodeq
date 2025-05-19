#include "parser.hpp"
#include "environment.hpp"
#include "base_object.hpp"
#include "midi_manager.hpp"
#include "midi_object.hpp"
#include <chrono>
#include <cstdlib>
#include <iomanip>
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
    
    // MIDIマネージャーへの参照
    MIDIManager& midiManager;
    
    // 現在の入力行
    std::string currentLine;
    
    // 入力履歴
    std::vector<std::string> history;
    int historyIndex;
    
    // 自動的にティックを進めるかどうか
    bool autoTick;
    int tickInterval; // ミリ秒
    
    // 最後にプロンプトを表示した時刻
    std::chrono::steady_clock::time_point lastPromptTime;
    
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
        std::cout << terminal::BOLD << "Reelia Live Coding Environment" << terminal::RESET_COLOR << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  $var = @class       - Create instance" << std::endl;
        std::cout << "  $obj.attr = value   - Set attribute" << std::endl;
        std::cout << "  $var = $obj.attr    - Get attribute" << std::endl;
        std::cout << "  $obj.method()       - Call method" << std::endl;
        std::cout << "  cmd1 | cmd2         - Parallel execution" << std::endl;
        std::cout << std::endl;
        std::cout << "MIDI Commands:" << std::endl;
        std::cout << "  @midi.list          - List available MIDI devices" << std::endl;
        std::cout << "  @midi.device = X    - Select MIDI output device" << std::endl;
        std::cout << "  $n = @midi_note     - Create MIDI note object" << std::endl;
        std::cout << "  $cc = @midi_cc      - Create MIDI CC object" << std::endl;
        std::cout << "  $seq = @midi_seq    - Create MIDI sequence" << std::endl;
        std::cout << std::endl;
        std::cout << "Keyboard shortcuts:" << std::endl;
        std::cout << "  Ctrl+T         - Manual tick" << std::endl;
        std::cout << "  Ctrl+A         - Toggle auto-tick" << std::endl;
        std::cout << "  Ctrl+S         - Change tick interval" << std::endl;
        std::cout << "  Ctrl+M         - MIDI device configuration" << std::endl;
        std::cout << "  Ctrl+L         - Clear screen" << std::endl;
        std::cout << "  ?              - Show this help" << std::endl;
        std::cout << "  Ctrl+D         - Dump variables" << std::endl;
        std::cout << "  Ctrl+X         - Exit" << std::endl;
        std::cout << std::endl;
    }
    
    // 現在の状態表示
    void displayStatus() {
        std::cout << terminal::BOLD;
        
        // Auto-tick状態
        std::cout << "Auto-tick: " << (autoTick ? "ON" : "OFF");
        if (autoTick) {
            std::cout << " (" << tickInterval << "ms)";
        }
        std::cout << " | ";
        
        // MIDI状態
        if (midiManager.isInitialized()) {
            int deviceId = midiManager.getCurrentOutputDevice();
            if (deviceId >= 0) {
                auto outputs = midiManager.getAvailableOutputs();
                if (static_cast<size_t>(deviceId) < outputs.size()) {
                    std::cout << "MIDI: " << outputs[deviceId];
                } else {
                    std::cout << "MIDI: unknown device";
                }
            } else {
                std::cout << "MIDI: not connected";
            }
        } else {
            std::cout << "MIDI: not initialized";
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
    
    // MIDI設定
    void configureMIDI() {
        std::cout << terminal::BOLD << terminal::BLUE;
        std::cout << "MIDI Device Configuration:" << terminal::RESET_COLOR << std::endl;
        
        // 利用可能なMIDIデバイスの一覧表示
        std::cout << "Available MIDI Output Devices:" << std::endl;
        auto outputs = midiManager.getAvailableOutputs();
        
        if (outputs.empty()) {
            std::cout << "  No MIDI output devices found!" << std::endl;
        } else {
            for (size_t i = 0; i < outputs.size(); i++) {
                std::cout << "  " << i << ": " << outputs[i];
                if (static_cast<int>(i) == midiManager.getCurrentOutputDevice()) {
                    std::cout << " (selected)";
                }
                std::cout << std::endl;
            }
            
            // デバイス選択
            std::cout << "Enter device number to select (or just press Enter to cancel): ";
            
            // rawモードを一時的に解除
            terminal::disableRawMode();
            
            std::string input;
            std::getline(std::cin, input);
            
            if (!input.empty()) {
                try {
                    int deviceId = std::stoi(input);
                    if (deviceId >= 0 && static_cast<size_t>(deviceId) < outputs.size()) {
                        midiManager.openOutputDevice(deviceId);
                        midiManager.startProcessing();
                        std::cout << "MIDI output device set to: " << outputs[deviceId] << std::endl;
                    } else {
                        std::cout << "Invalid device ID!" << std::endl;
                    }
                } catch (...) {
                    std::cout << "Invalid input!" << std::endl;
                }
            }
            
            // rawモードを再設定
            terminal::enableRawMode();
        }
    }
    
    // MIDIコマンド処理
    bool handleMIDICommand(const std::string& line) {
        if (line == "@midi.list") {
            // MIDIデバイス一覧表示
            auto outputs = midiManager.getAvailableOutputs();
            std::cout << "Available MIDI Output Devices:" << std::endl;
            
            if (outputs.empty()) {
                std::cout << "  No MIDI output devices found!" << std::endl;
            } else {
                for (size_t i = 0; i < outputs.size(); i++) {
                    std::cout << "  " << i << ": " << outputs[i];
                    if (static_cast<int>(i) == midiManager.getCurrentOutputDevice()) {
                        std::cout << " (selected)";
                    }
                    std::cout << std::endl;
                }
            }
            return true;
        } else if (line.find("@midi.device") == 0) {
            // MIDIデバイス選択
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string valueStr = line.substr(pos + 1);
                try {
                    int deviceId = std::stoi(valueStr);
                    auto outputs = midiManager.getAvailableOutputs();
                    if (deviceId >= 0 && static_cast<size_t>(deviceId) < outputs.size()) {
                        midiManager.openOutputDevice(deviceId);
                        midiManager.startProcessing();
                        std::cout << "MIDI output device set to: " << outputs[deviceId] << std::endl;
                    } else {
                        std::cout << "Invalid device ID!" << std::endl;
                    }
                } catch (...) {
                    std::cout << "Invalid device ID format!" << std::endl;
                }
            }
            return true;
        }
        return false;
    }
    
    // 入力処理
    void handleInput() {
        int c = terminal::readKey();
        if (c == -1) return;
        
        // Ctrl+キー
        if (c < 32) {
            switch (c) {
                case 4:  // Ctrl+D
                    dumpVariables();
                    break;
                case 8:  // Backspace
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
                        
                        // MIDI特殊コマンドかチェック
                        if (!handleMIDICommand(currentLine)) {
                            // 通常のコマンド実行
                            std::cout << terminal::GREEN << "> " << currentLine << terminal::RESET_COLOR << std::endl;
                            parser.parseLine(currentLine);
                        }
                        currentLine.clear();
                    }
                    break;
                case 12: // Ctrl+L
                    clearScreen();
                    break;
                case 1:  // Ctrl+A
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
                    }
                    break;
                case 13: // Ctrl+M
                    configureMIDI();
                    break;
                case 20: // Ctrl+T
                    parser.tick();
                    displayClock();
                    break;
                case 24: // Ctrl+X
                    exit(0);
                    break;
            }
        } 
        // 通常の文字
        else if (c >= 32 && c < 127) {
            // '?' キーでもヘルプを表示
            if (c == 63 && currentLine.empty()) {
                displayHelp();
            } else {
                currentLine += static_cast<char>(c);
            }
        }
        
        // 現在の入力行を表示
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPromptTime).count();
        
        if (elapsed >= 100) { // 100msごとにプロンプト更新（負荷軽減）
            std::cout << "\r" << std::string(80, ' ') << "\r> " << currentLine << std::flush;
            lastPromptTime = now;
        }
    }
    
public:
    ReeliaSimulator() 
        : parser(env), 
          midiManager(getMIDIManager()),
          historyIndex(0), 
          autoTick(false), 
          tickInterval(250),
          lastPromptTime(std::chrono::steady_clock::now()) {
        // MIDI初期化
        midiManager.initialize();
    }
    
    ~ReeliaSimulator() {
        // MIDI片付け
        midiManager.cleanup();
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
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTickTime).count();
                
                if (elapsed >= tickInterval) {
                    parser.tick();
                    displayClock();
                    lastTickTime = now;
                    
                    // 入力行を再表示
                    std::cout << "\r" << std::string(80, ' ') << "\r> " << currentLine << std::flush;
                    lastPromptTime = now;
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
    } catch (const std::exception& e) {
        // 例外発生時にターミナル設定を元に戻す
        terminal::disableRawMode();
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}