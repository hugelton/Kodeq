#ifndef REELIA_PARSER_HPP
#define REELIA_PARSER_HPP

#include "base_object.hpp"
#include "environment.hpp"
#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

/**
 * 新しい構文パーサー
 * 提案された新構文を解析して実行する
 */
class Parser {
private:
  // 環境
  Environment &env;

  // 内部ヘルパー関数
  std::vector<std::string> tokenizeLine(const std::string &line);
  std::vector<std::string> splitByPipe(const std::string &line);
  bool isWhitespace(char c);
  std::string trim(const std::string &str);
  bool isBinaryPattern(const std::string &str);
  int parseBinaryPattern(const std::string &str);

  // 構文パターン処理
  bool processClassCreation(const std::string &line);
  bool processAttributeAccess(const std::string &line);
  bool processMethodCall(const std::string &line);
  bool processVariableAssignment(const std::string &line);
  bool processPipeline(const std::string &line);

  // 式の評価
  BaseObject *evaluateExpression(const std::string &expr);

public:
  Parser(Environment &environment) : env(environment) {}

  // 行の解析と実行
  bool parseLine(const std::string &line);

  // 複数行の解析と実行
  bool parseMultipleLines(const std::string &code);

  // ティック実行
  void tick() { env.tick(); }
};

#endif // REELIA_PARSER_HPP