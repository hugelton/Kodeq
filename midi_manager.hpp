#ifndef REELIA_MIDI_MANAGER_HPP
#define REELIA_MIDI_MANAGER_HPP

#include "RtMidi.h"
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <map>

/**
 * MIDIメッセージの構造体
 * MIDIイベントキューで使用
 */
struct MIDIMessage {
    enum Type {
        NOTE_ON,
        NOTE_OFF,
        CC,
        PROGRAM_CHANGE,
        AFTERTOUCH,
        PITCH_BEND,
        SYSTEM
    };
    
    Type type;
    int channel;  // 0-15
    int data1;    // ノート番号、CCナンバーなど
    int data2;    // ベロシティ、CCバリューなど
    double timestamp; // 将来的なスケジューリング用
    
    MIDIMessage(Type t, int ch, int d1, int d2 = 0, double ts = 0.0)
        : type(t), channel(ch), data1(d1), data2(d2), timestamp(ts) {}
};

/**
 * MIDIデバイス管理クラス
 * RtMidiを使用してMIDI出力を処理
 */
class MIDIManager {
private:
    std::unique_ptr<RtMidiOut> midiOut;
    std::vector<std::string> availableOutputs;
    int currentOutputDevice;
    bool initialized;
    
    // MIDIイベントキュー
    std::queue<MIDIMessage> messageQueue;
    std::mutex queueMutex;
    std::thread processingThread;
    bool running;
    
    // 内部メソッド
    void processMessages();
    bool sendMessage(const MIDIMessage& msg);
    
public:
    MIDIManager();
    ~MIDIManager();
    
    // 初期化と接続
    bool initialize();
    void cleanup();
    std::vector<std::string> getAvailableOutputs();
    bool openOutputDevice(int deviceId);
    int getCurrentOutputDevice() const;
    bool isInitialized() const;
    
    // MIDIメッセージ送信
    bool sendNoteOn(int channel, int note, int velocity);
    bool sendNoteOff(int channel, int note);
    bool sendCC(int channel, int controller, int value);
    bool sendProgramChange(int channel, int program);
    bool sendPitchBend(int channel, int value);
    bool sendAftertouch(int channel, int note, int pressure);
    bool sendChannelPressure(int channel, int pressure);
    
    // キューベースのメッセージスケジューリング
    void queueMessage(const MIDIMessage& msg);
    void startProcessing();
    void stopProcessing();
    
    // ユーティリティ
    static std::string noteName(int noteNumber);
    static int noteNumber(const std::string& noteName);
};

// シングルトンのMIDIManagerインスタンスを提供するグローバル関数
MIDIManager& getMIDIManager();

#endif // REELIA_MIDI_MANAGER_HPP