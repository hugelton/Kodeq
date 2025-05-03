#ifndef KODEQ_PARSER_HPP
#define KODEQ_PARSER_HPP

#include "expression.hpp"
#include "value.hpp"
#include <map>
#include <random> // For random number generation
#include <string>
#include <vector>

/**
 * KODEQ Parser Class
 * Handles the parsing and execution of KODEQ language commands
 */
class KodeqParser {
private:
  // Variable table (A-Z)
  std::map<char, BaseValue *> variables;

  // Expression evaluator
  ExpressionEvaluator *evaluator;

  // Tick counter
  int tickCounter = 0;

  // Random number generator
  static std::mt19937 rng;

  // Helper functions
  bool isInteger(const std::string &s);
  bool isBinaryPattern(const std::string &s);
  bool isHexPattern(const std::string &s);
  int parseLiteral(const std::string &s);
  std::string toUpper(const std::string &s);

  // Command processors
  bool processConditional(const std::vector<std::string> &tokens);
  bool processRepeat(const std::vector<std::string> &tokens);
  bool processFunctionCall(const std::vector<std::string> &tokens);
  bool processPatternOperation(const std::vector<std::string> &tokens);

public:
  KodeqParser();
  ~KodeqParser();

  // Module parameter setting
  bool setModuleParameter(char varName, const std::string &paramName,
                          int value);
  // Variable operations
  void setVariable(char name, BaseValue *value);
  BaseValue *getVariable(char name);

  // Input processing
  bool parseLine(const std::string &line);

  // Expression evaluation
  int evaluateExpression(const std::string &expr);

  // Random number generation
  int getRandom(int min, int max);

  // Debugging
  void printVariables();
  void inspectVariable(char varName);

  // Run multiple ticks
  void runTicks(int count);

  // Tick counter operations
  void advanceTick();
  int getTick() const { return tickCounter; }

  // Friend class declaration
  friend class ExpressionEvaluator;
};

#endif // KODEQ_PARSER_HPP