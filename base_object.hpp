#ifndef REELIA_BASE_OBJECT_HPP
#define REELIA_BASE_OBJECT_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// 前方宣言
class Environment;

/**
 * ベースオブジェクトクラス
 * すべてのReeliaオブジェクトの基底クラス
 */
class BaseObject {
public:
  virtual ~BaseObject() {}

  // オブジェクトの型名を取得
  virtual std::string getType() const = 0;

  // 現在の値を取得（MIDIなどに出力する際に使用）
  virtual int getValue() const = 0;

  // 属性の設定
  virtual void setAttribute(const std::string &name, BaseObject *value) = 0;

  // 属性の取得
  virtual BaseObject *getAttribute(const std::string &name) = 0;

  // オブジェクトの複製
  virtual BaseObject *clone() const = 0;

  // ティックごとの処理（オーバーライド可能）
  virtual void onTick(Environment & /* env */) {}

  // オブジェクトを文字列表現に変換（デバッグ用）
  virtual std::string toString() const { return "BaseObject:" + getType(); }
};

/**
 * 整数値オブジェクト
 */
class IntObject : public BaseObject {
private:
  int value;

public:
  IntObject(int v = 0) : value(v) {}

  std::string getType() const override { return "int"; }

  int getValue() const override { return value; }

  void setAttribute(const std::string & /* name */,
                    BaseObject * /* value */) override {
    // 整数オブジェクトは属性を持たない
    throw std::runtime_error("Integer objects don't have attributes");
  }

  BaseObject *getAttribute(const std::string & /* name */) override {
    // 整数オブジェクトは属性を持たない
    throw std::runtime_error("Integer objects don't have attributes");
  }

  BaseObject *clone() const override { return new IntObject(value); }

  void setValue(int v) { value = v; }

  std::string toString() const override {
    return "int:" + std::to_string(value);
  }
};

/**
 * 2進数パターンオブジェクト
 */
class BinaryPatternObject : public BaseObject {
private:
  int pattern;

public:
  BinaryPatternObject(int p = 0) : pattern(p) {}

  std::string getType() const override { return "binary"; }

  int getValue() const override { return pattern; }

  void setAttribute(const std::string &name, BaseObject *value) override {
    if (name == "value") {
      pattern = value->getValue();
    } else {
      throw std::runtime_error("Unknown attribute: " + name);
    }
  }

  BaseObject *getAttribute(const std::string &name) override {
    if (name == "value") {
      return new IntObject(pattern);
    } else {
      throw std::runtime_error("Unknown attribute: " + name);
    }
  }

  BaseObject *clone() const override {
    return new BinaryPatternObject(pattern);
  }

  std::string toString() const override {
    std::string binStr = "b";
    for (int i = 7; i >= 0; i--) {
      binStr += ((pattern >> i) & 1) ? '1' : '0';
    }
    return binStr;
  }
};

/**
 * シーケンスオブジェクト
 */
class SeqObject : public BaseObject {
private:
  std::vector<int> data;
  int position;
  int length;
  bool playing;

public:
  SeqObject() : position(0), length(8), playing(false) {
    data.resize(16, 0); // デフォルトで16ステップ、すべて0
  }

  std::string getType() const override { return "seq"; }

  int getValue() const override {
    if (position >= 0 && static_cast<size_t>(position) < data.size()) {
      return data[position];
    }
    return 0;
  }

  void setAttribute(const std::string &name, BaseObject *value) override {
    if (name == "data") {
      // バイナリパターンからデータをセット
      int pattern = value->getValue();
      for (int i = 0; i < 8; i++) {
        if (static_cast<size_t>(i) < data.size()) {
          data[i] = (pattern & (1 << i)) ? 1 : 0;
        }
      }
    } else if (name == "pos" || name == "position") {
      position = value->getValue() % static_cast<int>(data.size());
    } else if (name == "length") {
      length = value->getValue();
      if (length > 16)
        length = 16;
      if (length < 1)
        length = 1;
    } else if (name == "step") {
      // セット対象のステップと値
      int step = value->getValue() & 0xF; // 0-15に制限
      int val = (value->getValue() >> 4) & 0xFF;
      if (static_cast<size_t>(step) < data.size()) {
        data[step] = val;
      }
    } else {
      throw std::runtime_error("Unknown attribute: " + name);
    }
  }

  BaseObject *getAttribute(const std::string &name) override {
    if (name == "data") {
      // 8ビットパターンとして最初の8ステップを返す
      int pattern = 0;
      for (int i = 0; i < 8 && static_cast<size_t>(i) < data.size(); i++) {
        if (data[i] > 0) {
          pattern |= (1 << i);
        }
      }
      return new BinaryPatternObject(pattern);
    } else if (name == "pos" || name == "position") {
      return new IntObject(position);
    } else if (name == "length") {
      return new IntObject(length);
    } else if (name == "step") {
      // 現在のステップの値を返す
      if (static_cast<size_t>(position) < data.size()) {
        return new IntObject(data[position]);
      }
      return new IntObject(0);
    } else {
      throw std::runtime_error("Unknown attribute: " + name);
    }
  }

  BaseObject *clone() const override {
    SeqObject *clone = new SeqObject();
    clone->data = this->data;
    clone->position = this->position;
    clone->length = this->length;
    clone->playing = this->playing;
    return clone;
  }

  void onTick(Environment & /* env */) override {
    if (playing) {
      position = (position + 1) % length;
    }
  }

  // シーケンスの開始（イベントキューでコールされる）
  void start() {
    playing = true;
    position = 0;
  }

  // シーケンスの停止（イベントキューでコールされる）
  void stop() { playing = false; }

  std::string toString() const override {
    std::string result = "seq[";
    for (int i = 0; i < length; i++) {
      if (i > 0)
        result += ",";
      result += std::to_string(data[i]);
      if (i == position)
        result += "*";
    }
    result += "]";
    return result;
  }
};

/**
 * カウンターオブジェクト
 */
class CountObject : public BaseObject {
private:
  int value;
  int max;
  int min;
  int step;
  bool running;

public:
  CountObject() : value(0), max(16), min(0), step(1), running(false) {}

  std::string getType() const override { return "count"; }

  int getValue() const override { return value; }

  void setAttribute(const std::string &name, BaseObject *value) override {
    if (name == "value") {
      this->value = value->getValue();
    } else if (name == "max") {
      max = value->getValue();
    } else if (name == "min") {
      min = value->getValue();
    } else if (name == "step") {
      step = value->getValue();
    } else {
      throw std::runtime_error("Unknown attribute: " + name);
    }
  }

  BaseObject *getAttribute(const std::string &name) override {
    if (name == "value") {
      return new IntObject(value);
    } else if (name == "max") {
      return new IntObject(max);
    } else if (name == "min") {
      return new IntObject(min);
    } else if (name == "step") {
      return new IntObject(step);
    } else {
      throw std::runtime_error("Unknown attribute: " + name);
    }
  }

  BaseObject *clone() const override {
    CountObject *clone = new CountObject();
    clone->value = this->value;
    clone->max = this->max;
    clone->min = this->min;
    clone->step = this->step;
    clone->running = this->running;
    return clone;
  }

  void onTick(Environment & /* env */) override {
    if (running) {
      value += step;
      if (value > max) {
        if (step > 0) {
          value = min;
        } else {
          value = max;
        }
      } else if (value < min) {
        if (step < 0) {
          value = max;
        } else {
          value = min;
        }
      }
    }
  }

  // カウンターの開始（イベントキューでコールされる）
  void start() { running = true; }

  // カウンターの停止（イベントキューでコールされる）
  void stop() { running = false; }

  // カウンターのリセット（イベントキューでコールされる）
  void reset() { value = min; }

  std::string toString() const override {
    return "count:" + std::to_string(value) + " [" + std::to_string(min) + ":" +
           std::to_string(max) + ":" + std::to_string(step) + "]";
  }
};

/**
 * ファクトリークラス
 * オブジェクト名からインスタンスを生成
 */
class ObjectFactory {
public:
  static BaseObject *createObject(const std::string &type) {
    if (type == "int") {
      return new IntObject(0);
    } else if (type == "seq") {
      return new SeqObject();
    } else if (type == "count") {
      return new CountObject();
    } else if (type == "binary") {
      return new BinaryPatternObject(0);
    }
    // 将来的に他のタイプを追加

    throw std::runtime_error("Unknown object type: " + type);
  }
};

#endif // REELIA_BASE_OBJECT_HPP