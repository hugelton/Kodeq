#ifndef REELIA_ENVIRONMENT_HPP
#define REELIA_ENVIRONMENT_HPP

#include "base_object.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * 環境クラス
 * 変数テーブルと実行コンテキストを管理
 */
class Environment {
private:
  // 変数テーブル: 名前からオブジェクトへのマッピング
  std::map<std::string, BaseObject *> variables;

  // ティックごとのイベントコールバック
  std::vector<std::function<void(Environment &)>> tickHandlers;

  // パイプラインに登録されたイベントキュー
  std::vector<std::function<void(Environment &)>> eventQueue;

  // 現在のティックカウンター
  int tickCounter;

public:
  Environment() : tickCounter(0) {}

  ~Environment() {
    // メモリリークを防ぐため全変数を解放
    for (auto &pair : variables) {
      delete pair.second;
    }
    variables.clear();
  }

  // 変数の設定（所有権を移動）
  void setVariable(const std::string &name, BaseObject *value) {
    // 既存の変数があれば削除
    auto it = variables.find(name);
    if (it != variables.end()) {
      delete it->second;
    }
    variables[name] = value;
  }

  // 変数の取得（所有権は移動しない）
  BaseObject *getVariable(const std::string &name) {
    auto it = variables.find(name);
    if (it != variables.end()) {
      return it->second;
    }
    return nullptr;
  }

  // 変数が存在するかチェック
  bool hasVariable(const std::string &name) {
    return variables.find(name) != variables.end();
  }

  // ティックハンドラの登録
  void addTickHandler(std::function<void(Environment &)> handler) {
    tickHandlers.push_back(handler);
  }

  // イベントキューへのイベント追加
  void queueEvent(std::function<void(Environment &)> event) {
    eventQueue.push_back(event);
  }

  // ティックの実行（1サイクル）
  void tick() {
    // ティックカウンターの更新
    tickCounter = (tickCounter + 1) % 256;

    // 全てのオブジェクトのonTickを呼び出し
    for (auto &pair : variables) {
      if (pair.second) {
        pair.second->onTick(*this);
      }
    }

    // 登録されたティックハンドラを呼び出し
    for (auto &handler : tickHandlers) {
      handler(*this);
    }

    // イベントキューの処理
    auto currentEvents = std::move(eventQueue);
    eventQueue.clear();

    for (auto &event : currentEvents) {
      event(*this);
    }
  }

  // 現在のティックカウンターを取得
  int getTickCount() const { return tickCounter; }

  // 全変数の表示（デバッグ用）
  void dumpVariables() {
    for (const auto &pair : variables) {
      std::cout << "$" << pair.first << " = ";
      if (pair.second) {
        std::cout << pair.second->toString();
      } else {
        std::cout << "null";
      }
      std::cout << std::endl;
    }
  }
};

#endif // REELIA_ENVIRONMENT_HPP