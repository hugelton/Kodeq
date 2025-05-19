#ifndef REELIA_MIDI_OBJECT_HPP
#define REELIA_MIDI_OBJECT_HPP

#include "base_object.hpp"
#include "environment.hpp"
#include "midi_manager.hpp"
#include <vector>

/**
 * MIDI Note オブジェクト
 * シーケンサーに接続してMIDIノートを送信
 */
class MIDINoteObject : public BaseObject {
private:
    int channel;       // MIDIチャンネル (0-15)
    int note;          // MIDIノート (0-127)
    int velocity;      // ベロシティ (0-127)
    int duration;      // ノート持続時間（ティック数）
    int durationCount; // 持続時間カウンター
    bool isPlaying;    // 再生状態
    
public:
    MIDINoteObject() 
        : channel(0), note(60), velocity(100),
          duration(1), durationCount(0), isPlaying(false) {}
    
    std::string getType() const override { return "midi_note"; }
    
    int getValue() const override { return isPlaying ? velocity : 0; }
    
    void setAttribute(const std::string &name, BaseObject* value) override {
        if (name == "channel") {
            channel = value->getValue() & 0x0F;  // 0-15に制限
        } else if (name == "note") {
            note = value->getValue() & 0x7F;     // 0-127に制限
        } else if (name == "velocity") {
            velocity = value->getValue() & 0x7F; // 0-127に制限
        } else if (name == "duration") {
            duration = value->getValue();
            if (duration < 1) duration = 1;
        } else {
            throw std::runtime_error("Unknown attribute: " + name);
        }
    }
    
    BaseObject* getAttribute(const std::string &name) override {
        if (name == "channel") {
            return new IntObject(channel);
        } else if (name == "note") {
            return new IntObject(note);
        } else if (name == "velocity") {
            return new IntObject(velocity);
        } else if (name == "duration") {
            return new IntObject(duration);
        } else if (name == "playing") {
            return new IntObject(isPlaying ? 1 : 0);
        } else {
            throw std::runtime_error("Unknown attribute: " + name);
        }
    }
    
    BaseObject* clone() const override {
        MIDINoteObject* clone = new MIDINoteObject();
        clone->channel = this->channel;
        clone->note = this->note;
        clone->velocity = this->velocity;
        clone->duration = this->duration;
        clone->durationCount = this->durationCount;
        clone->isPlaying = this->isPlaying;
        return clone;
    }
    
    void onTick(Environment& /* env */) override {
        if (isPlaying) {
            durationCount++;
            if (durationCount >= duration) {
                // ノート持続時間が経過したらノートオフを送信
                getMIDIManager().sendNoteOff(channel, note);
                isPlaying = false;
                durationCount = 0;
            }
        }
    }
    
    // ノートトリガーメソッド（イベントキューから呼び出される）
    void trigger() {
        if (isPlaying) {
            // 既に再生中の場合、いったんノートオフを送信
            getMIDIManager().sendNoteOff(channel, note);
        }
        
        // ノートオンを送信
        getMIDIManager().sendNoteOn(channel, note, velocity);
        isPlaying = true;
        durationCount = 0;
    }
    
    // ノート停止メソッド（イベントキューから呼び出される）
    void stop() {
        if (isPlaying) {
            getMIDIManager().sendNoteOff(channel, note);
            isPlaying = false;
            durationCount = 0;
        }
    }
    
    std::string toString() const override {
        return "midi_note: ch=" + std::to_string(channel) + 
               " note=" + getMIDIManager().noteName(note) + 
               " vel=" + std::to_string(velocity) + 
               (isPlaying ? " [playing]" : "");
    }
};

/**
 * MIDI CC オブジェクト
 * コントロールチェンジメッセージを送信
 */
class MIDICCObject : public BaseObject {
private:
    int channel;     // MIDIチャンネル (0-15)
    int controller;  // コントローラー番号 (0-127)
    int value;       // CC値 (0-127)
    
public:
    MIDICCObject() : channel(0), controller(1), value(0) {}
    
    std::string getType() const override { return "midi_cc"; }
    
    int getValue() const override { return value; }
    
    void setAttribute(const std::string &name, BaseObject* val) override {
        if (name == "channel") {
            channel = val->getValue() & 0x0F;      // 0-15に制限
        } else if (name == "controller" || name == "cc") {
            controller = val->getValue() & 0x7F;   // 0-127に制限
        } else if (name == "value") {
            value = val->getValue() & 0x7F;        // 0-127に制限
            // 値が変更されたら即座にCC送信
            getMIDIManager().sendCC(channel, controller, value);
        } else {
            throw std::runtime_error("Unknown attribute: " + name);
        }
    }
    
    BaseObject* getAttribute(const std::string &name) override {
        if (name == "channel") {
            return new IntObject(channel);
        } else if (name == "controller" || name == "cc") {
            return new IntObject(controller);
        } else if (name == "value") {
            return new IntObject(value);
        } else {
            throw std::runtime_error("Unknown attribute: " + name);
        }
    }
    
    BaseObject* clone() const override {
        MIDICCObject* clone = new MIDICCObject();
        clone->channel = this->channel;
        clone->controller = this->controller;
        clone->value = this->value;
        return clone;
    }
    
    void send() {
        getMIDIManager().sendCC(channel, controller, value);
    }
    
    std::string toString() const override {
        return "midi_cc: ch=" + std::to_string(channel) + 
               " cc=" + std::to_string(controller) + 
               " val=" + std::to_string(value);
    }
};

/**
 * MIDI対応シーケンスオブジェクト拡張
 * 基本的なSeqObjectに対して、MIDI出力機能を追加
 */
class MIDISeqObject : public SeqObject {
private:
    int midiChannel;         // MIDIチャンネル
    std::vector<int> notes;  // ステップごとのノート番号
    int velocity;            // ベロシティ
    bool midiEnabled;        // MIDI出力有効/無効
    
public:
    MIDISeqObject() : midiChannel(0), velocity(100), midiEnabled(true) {
        // デフォルトのノートマッピング (C4 = 60)
        notes.resize(16, 60);
    }
    
    // オーバーライドされたonTick
    void onTick(Environment& env) override {
        // 基底クラスの処理を呼び出し
        SeqObject::onTick(env);
        
        // MIDI出力が有効で、現在のステップが1（オン）の場合
        if (midiEnabled && getValue() > 0) {
            int position = 0;
            BaseObject* posObj = getAttribute("position");
            if (posObj) {
                position = posObj->getValue();
                delete posObj;
            }
            
            // 位置が有効範囲内の場合、ノートを発音
            if (position >= 0 && static_cast<size_t>(position) < notes.size()) {
                int note = notes[position];
                if (note >= 0) {
                    getMIDIManager().sendNoteOn(midiChannel, note, velocity);
                    
                    // ノートオフを遅延送信するラムダ関数を登録
                    auto noteOffEvent = [this, note](Environment& /* env */) {
                        getMIDIManager().sendNoteOff(this->midiChannel, note);
                    };
                    
                    // 次のティックでノートオフを送信
                    env.queueEvent(noteOffEvent);
                }
            }
        }
    }
    
    // MIDI関連の属性アクセス
    void setAttribute(const std::string &name, BaseObject* value) override {
        if (name == "midi_channel") {
            midiChannel = value->getValue() & 0x0F;  // 0-15に制限
        } else if (name == "midi_velocity") {
            velocity = value->getValue() & 0x7F;     // 0-127に制限
        } else if (name == "midi_enable") {
            midiEnabled = value->getValue() > 0;
        } else if (name == "note_map") {
            // バイナリパターンからノートマッピングを設定
            int baseNote = 60; // デフォルトのベースノート
            int pattern = value->getValue();
            for (int i = 0; i < 8; i++) {
                if (static_cast<size_t>(i) < notes.size()) {
                    notes[i] = (pattern & (1 << i)) ? baseNote + i : -1;
                }
            }
        } else if (name == "note_base") {
            // ベースノートを設定（例：36 = C2）
            int baseNote = value->getValue() & 0x7F;
            for (size_t i = 0; i < notes.size(); i++) {
                if (notes[i] >= 0) {
                    notes[i] = baseNote + static_cast<int>(i);
                }
            }
        } else if (name.substr(0, 5) == "note_" && name.size() > 5) {
            // 特定のステップのノートを設定（例：note_0 = 60）
            try {
                int step = std::stoi(name.substr(5));
                if (step >= 0 && static_cast<size_t>(step) < notes.size()) {
                    notes[step] = value->getValue() & 0x7F;
                }
            } catch (...) {
                // 数値変換エラー
            }
        } else {
            // 基底クラスの属性設定を呼び出し
            SeqObject::setAttribute(name, value);
        }
    }
    
    BaseObject* getAttribute(const std::string &name) override {
        if (name == "midi_channel") {
            return new IntObject(midiChannel);
        } else if (name == "midi_velocity") {
            return new IntObject(velocity);
        } else if (name == "midi_enable") {
            return new IntObject(midiEnabled ? 1 : 0);
        } else if (name == "note_base") {
            // 最初のノート番号を返す
            for (int note : notes) {
                if (note >= 0) {
                    return new IntObject(note);
                }
            }
            return new IntObject(60); // デフォルト
        } else if (name.substr(0, 5) == "note_" && name.size() > 5) {
            try {
                int step = std::stoi(name.substr(5));
                if (step >= 0 && static_cast<size_t>(step) < notes.size()) {
                    return new IntObject(notes[step]);
                }
            } catch (...) {
                // 数値変換エラー
            }
            return new IntObject(-1);
        } else {
            // 基底クラスの属性取得を呼び出し
            return SeqObject::getAttribute(name);
        }
    }
    
    BaseObject* clone() const override {
        MIDISeqObject* clone = new MIDISeqObject();
        
        // 基底クラスの基本プロパティをコピー
        SeqObject* baseClone = static_cast<SeqObject*>(SeqObject::clone());
        *static_cast<SeqObject*>(clone) = *baseClone;
        delete baseClone;
        
        // MIDI固有の属性をコピー
        clone->midiChannel = this->midiChannel;
        clone->notes = this->notes;
        clone->velocity = this->velocity;
        clone->midiEnabled = this->midiEnabled;
        
        return clone;
    }
    
    std::string toString() const override {
        return SeqObject::toString() + " [MIDI ch=" + std::to_string(midiChannel) + 
               (midiEnabled ? " enabled" : " disabled") + "]";
    }
};

#endif // REELIA_MIDI_OBJECT_HPP