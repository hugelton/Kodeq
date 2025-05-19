#include "midi_manager.hpp"
#include <chrono>
#include <cmath>
#include <array>
#include <algorithm>

// シングルトンインスタンス
static std::unique_ptr<MIDIManager> g_instance = nullptr;

// グローバルアクセス関数
MIDIManager& getMIDIManager() {
    if (!g_instance) {
        g_instance = std::make_unique<MIDIManager>();
    }
    return *g_instance;
}

// コンストラクタ
MIDIManager::MIDIManager()
    : currentOutputDevice(-1), initialized(false), running(false) {
}

// デストラクタ
MIDIManager::~MIDIManager() {
    cleanup();
}

// MIDIの初期化
bool MIDIManager::initialize() {
    if (initialized) {
        return true;
    }
    
    try {
        midiOut = std::make_unique<RtMidiOut>();
        initialized = true;
        
        // 利用可能なMIDI出力ポートをスキャン
        availableOutputs.clear();
        unsigned int portCount = midiOut->getPortCount();
        
        for (unsigned int i = 0; i < portCount; i++) {
            availableOutputs.push_back(midiOut->getPortName(i));
        }
        
        return true;
    } catch (RtMidiError &error) {
        std::cerr << "MIDI initialization error: " << error.getMessage() << std::endl;
        initialized = false;
        return false;
    }
}

// 利用可能なMIDI出力デバイスのリストを取得
std::vector<std::string> MIDIManager::getAvailableOutputs() {
    if (!initialized && !initialize()) {
        return {};
    }
    
    return availableOutputs;
}

// MIDI出力デバイスを開く
bool MIDIManager::openOutputDevice(int deviceId) {
    if (!initialized && !initialize()) {
        return false;
    }
    
    try {
        if (deviceId >= 0 && deviceId < static_cast<int>(availableOutputs.size())) {
            if (midiOut->isPortOpen()) {
                midiOut->closePort();
            }
            
            midiOut->openPort(deviceId);
            currentOutputDevice = deviceId;
            std::cout << "MIDI output device opened: " << availableOutputs[deviceId] << std::endl;
            return true;
        } else {
            std::cerr << "Invalid MIDI output device ID: " << deviceId << std::endl;
            return false;
        }
    } catch (RtMidiError &error) {
        std::cerr << "MIDI output device open error: " << error.getMessage() << std::endl;
        return false;
    }
}

// 現在のMIDI出力デバイスIDを取得
int MIDIManager::getCurrentOutputDevice() const {
    return currentOutputDevice;
}

// 初期化状態を取得
bool MIDIManager::isInitialized() const {
    return initialized;
}

// リソース解放
void MIDIManager::cleanup() {
    if (running) {
        stopProcessing();
    }
    
    if (initialized && midiOut) {
        if (midiOut->isPortOpen()) {
            midiOut->closePort();
        }
    }
    
    midiOut.reset();
    initialized = false;
}

// MIDIノートオンメッセージの送信
bool MIDIManager::sendNoteOn(int channel, int note, int velocity) {
    MIDIMessage msg(MIDIMessage::NOTE_ON, channel, note, velocity);
    return sendMessage(msg);
}

// MIDIノートオフメッセージの送信
bool MIDIManager::sendNoteOff(int channel, int note) {
    MIDIMessage msg(MIDIMessage::NOTE_OFF, channel, note, 0);
    return sendMessage(msg);
}

// MIDIコントロールチェンジメッセージの送信
bool MIDIManager::sendCC(int channel, int controller, int value) {
    MIDIMessage msg(MIDIMessage::CC, channel, controller, value);
    return sendMessage(msg);
}

// MIDIプログラムチェンジメッセージの送信
bool MIDIManager::sendProgramChange(int channel, int program) {
    MIDIMessage msg(MIDIMessage::PROGRAM_CHANGE, channel, program);
    return sendMessage(msg);
}

// MIDIピッチベンドメッセージの送信
bool MIDIManager::sendPitchBend(int channel, int value) {
    MIDIMessage msg(MIDIMessage::PITCH_BEND, channel, value & 0x7F, (value >> 7) & 0x7F);
    return sendMessage(msg);
}

// MIDIアフタータッチメッセージの送信
bool MIDIManager::sendAftertouch(int channel, int note, int pressure) {
    MIDIMessage msg(MIDIMessage::AFTERTOUCH, channel, note, pressure);
    return sendMessage(msg);
}

// MIDIチャンネルプレッシャーメッセージの送信
bool MIDIManager::sendChannelPressure(int channel, int pressure) {
    MIDIMessage msg(MIDIMessage::AFTERTOUCH, channel, pressure);
    return sendMessage(msg);
}

// メッセージをキューに追加
void MIDIManager::queueMessage(const MIDIMessage& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(msg);
}

// メッセージ処理スレッドの開始
void MIDIManager::startProcessing() {
    if (running) return;
    
    running = true;
    processingThread = std::thread(&MIDIManager::processMessages, this);
}

// メッセージ処理スレッドの停止
void MIDIManager::stopProcessing() {
    running = false;
    if (processingThread.joinable()) {
        processingThread.join();
    }
}

// キュー内のMIDIメッセージを処理
void MIDIManager::processMessages() {
    while (running) {
        MIDIMessage msg(MIDIMessage::NOTE_OFF, 0, 0);
        bool hasMessage = false;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!messageQueue.empty()) {
                msg = messageQueue.front();
                messageQueue.pop();
                hasMessage = true;
            }
        }
        
        if (hasMessage) {
            sendMessage(msg);
        }
        
        // スリープして CPU 使用率を抑える
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// MIDIメッセージの実際の送信処理
bool MIDIManager::sendMessage(const MIDIMessage& msg) {
    if (!initialized || !midiOut || !midiOut->isPortOpen()) {
        return false;
    }
    
    try {
        std::vector<unsigned char> message;
        
        switch (msg.type) {
            case MIDIMessage::NOTE_ON:
                message.push_back(0x90 | (msg.channel & 0x0F));
                message.push_back(msg.data1 & 0x7F);
                message.push_back(msg.data2 & 0x7F);
                break;
                
            case MIDIMessage::NOTE_OFF:
                message.push_back(0x80 | (msg.channel & 0x0F));
                message.push_back(msg.data1 & 0x7F);
                message.push_back(msg.data2 & 0x7F);
                break;
                
            case MIDIMessage::CC:
                message.push_back(0xB0 | (msg.channel & 0x0F));
                message.push_back(msg.data1 & 0x7F);
                message.push_back(msg.data2 & 0x7F);
                break;
                
            case MIDIMessage::PROGRAM_CHANGE:
                message.push_back(0xC0 | (msg.channel & 0x0F));
                message.push_back(msg.data1 & 0x7F);
                break;
                
            case MIDIMessage::AFTERTOUCH:
                message.push_back(0xA0 | (msg.channel & 0x0F));
                message.push_back(msg.data1 & 0x7F);
                message.push_back(msg.data2 & 0x7F);
                break;
                
            case MIDIMessage::PITCH_BEND:
                message.push_back(0xE0 | (msg.channel & 0x0F));
                message.push_back(msg.data1 & 0x7F);
                message.push_back(msg.data2 & 0x7F);
                break;
                
            default:
                return false;
        }
        
        midiOut->sendMessage(&message);
        return true;
    } catch (RtMidiError &error) {
        std::cerr << "MIDI send error: " << error.getMessage() << std::endl;
        return false;
    }
}

// MIDI ノート番号から名前へ変換
std::string MIDIManager::noteName(int noteNumber) {
    static const std::array<std::string, 12> noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    if (noteNumber < 0 || noteNumber > 127) {
        return "Invalid";
    }
    
    int octave = noteNumber / 12 - 1;
    int note = noteNumber % 12;
    
    return noteNames[note] + std::to_string(octave);
}

// ノート名からMIDIノート番号への変換
int MIDIManager::noteNumber(const std::string& noteName) {
    static const std::map<std::string, int> noteOffsets = {
        {"C", 0}, {"C#", 1}, {"DB", 1}, {"D", 2}, {"D#", 3}, {"EB", 3},
        {"E", 4}, {"F", 5}, {"F#", 6}, {"GB", 6}, {"G", 7}, {"G#", 8},
        {"AB", 8}, {"A", 9}, {"A#", 10}, {"BB", 10}, {"B", 11}
    };
    
    // 入力を正規化（大文字に変換）
    std::string normalizedName = noteName;
    std::transform(normalizedName.begin(), normalizedName.end(), normalizedName.begin(), ::toupper);
    
    // 音名とオクターブを分離
    std::string notePart;
    int octave = 0;
    size_t i = 0;
    
    while (i < normalizedName.size() && !std::isdigit(normalizedName[i]) && normalizedName[i] != '-') {
        notePart += normalizedName[i++];
    }
    
    if (i < normalizedName.size()) {
        octave = std::stoi(normalizedName.substr(i));
    }
    
    // MIDI ノート番号を計算
    auto it = noteOffsets.find(notePart);
    if (it == noteOffsets.end()) {
        return -1;  // 無効なノート名
    }
    
    return (octave + 1) * 12 + it->second;
}