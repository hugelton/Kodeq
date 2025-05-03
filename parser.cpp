#include "parser.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

// Initialize static random number generator
std::mt19937 KodeqParser::rng(std::random_device{}());

KodeqParser::KodeqParser() {
  // Initialize expression evaluator
  evaluator = new ExpressionEvaluator(*this);
}

KodeqParser::~KodeqParser() {
  // Free memory
  for (auto &pair : variables) {
    delete pair.second;
  }
  variables.clear();

  // Free expression evaluator
  delete evaluator;
}

bool KodeqParser::isInteger(const std::string &s) {
  if (s.empty())
    return false;

  size_t start = 0;
  if (s[0] == '-') {
    start = 1;
    if (s.length() == 1)
      return false;
  }

  for (size_t i = start; i < s.length(); ++i) {
    if (!std::isdigit(s[i]))
      return false;
  }

  return true;
}

bool KodeqParser::isBinaryPattern(const std::string &s) {
  if (s.size() < 2 || s[0] != '#')
    return false;

  for (size_t i = 1; i < s.length(); ++i) {
    if (s[i] != '0' && s[i] != '1')
      return false;
  }

  return true;
}

bool KodeqParser::isHexPattern(const std::string &s) {
  if (s.size() < 2 || s[0] != 'X')
    return false;

  for (size_t i = 1; i < s.length(); ++i) {
    if (!std::isxdigit(s[i]))
      return false;
  }

  return true;
}

int KodeqParser::parseLiteral(const std::string &s) {
  if (isInteger(s)) {
    return std::stoi(s);
  } else if (isBinaryPattern(s)) {
    // Binary pattern (#10110)
    std::string binStr = s.substr(1); // Remove '#'
    return std::stoi(binStr, nullptr, 2);
  } else if (isHexPattern(s)) {
    // Hexadecimal pattern (XFF)
    std::string hexStr = s.substr(1); // Remove 'X'
    return std::stoi(hexStr, nullptr, 16);
  }

  // Default return 0 for other formats
  return 0;
}

std::string KodeqParser::toUpper(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

void KodeqParser::setVariable(char name, BaseValue *value) {
  // Free existing value
  auto it = variables.find(name);
  if (it != variables.end()) {
    delete it->second;
  }

  // Set new value
  variables[name] = value;
}

BaseValue *KodeqParser::getVariable(char name) {
  auto it = variables.find(name);
  if (it != variables.end()) {
    return it->second;
  }
  return nullptr;
}

int KodeqParser::evaluateExpression(const std::string &expr) {
  return evaluator->evaluate(expr);
}

bool KodeqParser::processConditional(const std::vector<std::string> &tokens) {
  // Format: IF expr THEN command
  if (tokens.size() < 4 || tokens[0] != "IF" ||
      std::find(tokens.begin(), tokens.end(), "THEN") == tokens.end()) {
    return false;
  }

  // Find the position of "THEN"
  auto thenPos = std::find(tokens.begin(), tokens.end(), "THEN");
  size_t thenIndex = std::distance(tokens.begin(), thenPos);

  // Extract condition expression
  std::string condition;
  for (size_t i = 1; i < thenIndex; ++i) {
    if (i > 1)
      condition += " ";
    condition += tokens[i];
  }

  // Evaluate the condition
  int result = evaluateExpression(condition);

  // If condition is true (non-zero), execute the command after "THEN"
  if (result != 0) {
    // Reconstruct the command after THEN
    std::string command;
    for (size_t i = thenIndex + 1; i < tokens.size(); ++i) {
      if (i > thenIndex + 1)
        command += " ";
      command += tokens[i];
    }

    // Execute the command
    return parseLine(command);
  }

  return true;
}

bool KodeqParser::processRepeat(const std::vector<std::string> &tokens) {
  // Format: REPEAT expr DO command
  if (tokens.size() < 4 || tokens[0] != "REPEAT" ||
      std::find(tokens.begin(), tokens.end(), "DO") == tokens.end()) {
    return false;
  }

  // Find the position of "DO"
  auto doPos = std::find(tokens.begin(), tokens.end(), "DO");
  size_t doIndex = std::distance(tokens.begin(), doPos);

  // Extract count expression
  std::string countExpr;
  for (size_t i = 1; i < doIndex; ++i) {
    if (i > 1)
      countExpr += " ";
    countExpr += tokens[i];
  }

  // Evaluate the count
  int count = evaluateExpression(countExpr);

  // Reconstruct the command after DO
  std::string command;
  for (size_t i = doIndex + 1; i < tokens.size(); ++i) {
    if (i > doIndex + 1)
      command += " ";
    command += tokens[i];
  }

  // Execute the command count times
  bool success = true;
  for (int i = 0; i < count && success; ++i) {
    success = parseLine(command);
  }

  return success;
}

int KodeqParser::getRandom(int min, int max) {
  std::uniform_int_distribution<int> dist(min, max);
  return dist(rng);
}

bool KodeqParser::processFunctionCall(const std::vector<std::string> &tokens) {
  if (tokens.size() >= 1) {
    // RND(min, max) function
    if (tokens[0].substr(0, 4) == "RND(" && tokens[0].back() == ')') {
      // Parse parameters
      std::string params = tokens[0].substr(4, tokens[0].length() - 5);
      std::istringstream paramStream(params);
      std::string minStr, maxStr;
      std::getline(paramStream, minStr, ',');
      std::getline(paramStream, maxStr);

      try {
        int min = std::stoi(minStr);
        int max = std::stoi(maxStr);
        int result = getRandom(min, max);

        // Create a temporary variable with the result
        setVariable('_', new IntValue(result));
        return true;
      } catch (const std::exception &e) {
        std::cout << "Error in RND function: " << e.what() << std::endl;
        return false;
      }
    }

    // Future functions can be added here
  }

  return false;
}

// Function to handle pattern operations (reverse, rotate, etc.)
bool KodeqParser::processPatternOperation(
    const std::vector<std::string> &tokens) {
  if (tokens.size() >= 3 && tokens[1] == "=") {
    // Format: $X = ROTATE($Y, amount)
    if (tokens[2].substr(0, 7) == "ROTATE(" && tokens[2].back() == ')') {
      std::string params = tokens[2].substr(7, tokens[2].length() - 8);
      std::istringstream paramStream(params);
      std::string sourceVar, amountStr;
      std::getline(paramStream, sourceVar, ',');
      std::getline(paramStream, amountStr);

      // Check if sourceVar is a variable reference
      if (sourceVar.size() == 2 && sourceVar[0] == '$' &&
          std::isalpha(sourceVar[1])) {
        char srcName = std::toupper(sourceVar[1]);
        BaseValue *srcValue = getVariable(srcName);

        if (srcValue && srcValue->getType() == "MODULE" &&
            static_cast<ModuleValue *>(srcValue)->getModuleName() == "PAT") {

          // Get source pattern value
          ModuleValue *srcMod = static_cast<ModuleValue *>(srcValue);
          PatternModule *srcPattern =
              static_cast<PatternModule *>(srcMod->getModule());

          // Get rotation amount
          int amount;
          try {
            amount = std::stoi(amountStr);
          } catch (...) {
            amount = evaluateExpression(amountStr);
          }

          // Create a new pattern module with rotated value
          PatternModule *newPattern = new PatternModule();

          // Get original pattern value (assuming pattern is in parameter P)
          int originalPattern = 0;
          srcPattern->setParameter("P", originalPattern);

          // Implement bit rotation (8-bit)
          amount = amount % 8;
          if (amount < 0)
            amount += 8;
          int rotatedPattern = ((originalPattern << amount) |
                                (originalPattern >> (8 - amount))) &
                               0xFF;

          newPattern->setParameter("P", rotatedPattern);

          // Parse destination variable
          if (tokens[0].size() == 2 && tokens[0][0] == '$' &&
              std::isalpha(tokens[0][1])) {
            char destName = std::toupper(tokens[0][1]);
            setVariable(destName, new ModuleValue(newPattern));
            return true;
          }
        }
      }
    }

    // Format: $X = REVERSE($Y)
    if (tokens[2].substr(0, 8) == "REVERSE(" && tokens[2].back() == ')') {
      std::string sourceVar = tokens[2].substr(8, tokens[2].length() - 9);

      // Check if sourceVar is a variable reference
      if (sourceVar.size() == 2 && sourceVar[0] == '$' &&
          std::isalpha(sourceVar[1])) {
        char srcName = std::toupper(sourceVar[1]);
        BaseValue *srcValue = getVariable(srcName);

        if (srcValue && srcValue->getType() == "MODULE" &&
            static_cast<ModuleValue *>(srcValue)->getModuleName() == "PAT") {

          // Get source pattern value
          ModuleValue *srcMod = static_cast<ModuleValue *>(srcValue);
          PatternModule *srcPattern =
              static_cast<PatternModule *>(srcMod->getModule());

          // Create a new pattern module with reversed value
          PatternModule *newPattern = new PatternModule();

          // Get original pattern value (assuming pattern is in parameter P)
          int originalPattern = 0;
          srcPattern->setParameter("P", originalPattern);

          // Implement bit reversal (8-bit)
          int reversedPattern = 0;
          for (int i = 0; i < 8; i++) {
            if ((originalPattern >> i) & 1) {
              reversedPattern |= (1 << (7 - i));
            }
          }

          newPattern->setParameter("P", reversedPattern);

          // Parse destination variable
          if (tokens[0].size() == 2 && tokens[0][0] == '$' &&
              std::isalpha(tokens[0][1])) {
            char destName = std::toupper(tokens[0][1]);
            setVariable(destName, new ModuleValue(newPattern));
            return true;
          }
        }
      }
    }
  }

  return false;
}

// Update parseLine to include the new commands
bool KodeqParser::parseLine(const std::string &line) {
  // Skip empty lines
  if (line.empty())
    return true;

  // Convert input to uppercase
  std::string upperLine = toUpper(line);

  // Split into tokens
  std::istringstream iss(upperLine);
  std::vector<std::string> tokens;
  std::string token;

  while (iss >> token) {
    tokens.push_back(token);
  }

  if (tokens.empty())
    return true;

  // Check for new command types
  if (tokens[0] == "IF") {
    return processConditional(tokens);
  } else if (tokens[0] == "REPEAT") {
    return processRepeat(tokens);
  } else if (tokens[0].substr(0, 4) == "RND(") {
    return processFunctionCall(tokens);
  } else if (tokens[0] == "RUN" && tokens.size() > 1) {
    try {
      int count = std::stoi(tokens[1]);
      runTicks(count);
      return true;
    } catch (const std::exception &e) {
      std::cout << "Error in RUN command: " << e.what() << std::endl;
      return false;
    }
  }

  // Try pattern operations
  if (processPatternOperation(tokens)) {
    return true;
  }

  // Module parameter setting: $X.PARAM=VALUE
  if (tokens.size() >= 3 && tokens[0].size() > 3 && tokens[0][0] == '$' &&
      std::isalpha(tokens[0][1]) && tokens[0][2] == '.' && tokens[1] == "=") {

    char varName = tokens[0][1];
    std::string paramName = tokens[0].substr(3); // Extract parameter name part

    // Combine remaining tokens as expression to evaluate
    std::string exprStr;
    for (size_t i = 2; i < tokens.size(); ++i) {
      if (i > 2)
        exprStr += " ";
      exprStr += tokens[i];
    }

    // Evaluate expression
    try {
      int paramValue = evaluateExpression(exprStr);
      return setModuleParameter(varName, paramName, paramValue);
    } catch (const std::exception &e) {
      std::cout << "Error evaluating expression: " << e.what() << std::endl;
      return false;
    }
  }
  // Variable assignment: $X=VALUE
  else if (tokens.size() >= 3 && tokens[0].size() == 2 && tokens[0][0] == '$' &&
           std::isalpha(tokens[0][1]) && tokens[1] == "=") {

    char varName = tokens[0][1];

    // Combine remaining tokens as expression to evaluate
    std::string exprStr;
    for (size_t i = 2; i < tokens.size(); ++i) {
      if (i > 2)
        exprStr += " ";
      exprStr += tokens[i];
    }

    // Determine if the expression is a simple value or complex expression
    if (isInteger(exprStr) || isBinaryPattern(exprStr) ||
        isHexPattern(exprStr)) {
      // Simple literal
      setVariable(varName, new IntValue(parseLiteral(exprStr)));
      std::cout << "$" << varName << " = " << parseLiteral(exprStr)
                << " (INTEGER)" << std::endl;
    } else if (exprStr.size() == 2 && exprStr[0] == '$' &&
               std::isalpha(exprStr[1])) {
      // Variable reference
      char srcVarName = exprStr[1];
      BaseValue *srcValue = getVariable(srcVarName);
      if (srcValue) {
        if (srcValue->getType() == "INTEGER") {
          setVariable(varName, new IntValue(srcValue->toInt()));
          std::cout << "$" << varName << " = " << srcValue->toInt()
                    << " (INTEGER)" << std::endl;
        } else {
          ModuleValue *modVal = static_cast<ModuleValue *>(srcValue);
          Module *clonedModule = modVal->getModule()->clone();
          setVariable(varName, new ModuleValue(clonedModule));
          std::cout << "$" << varName << " = " << modVal->getModuleName()
                    << " (MODULE)" << std::endl;
        }
      } else {
        std::cout << "Error: Undefined variable $" << srcVarName << std::endl;
        return false;
      }
    } else if (ModuleFactory::createModule(exprStr) != nullptr) {
      // Module initialization
      setVariable(varName, new ModuleValue(exprStr));
      std::cout << "$" << varName << " = " << exprStr << " (MODULE)"
                << std::endl;
    } else {
      // Complex expression evaluation
      try {
        int result = evaluateExpression(exprStr);
        setVariable(varName, new IntValue(result));
        std::cout << "$" << varName << " = " << result
                  << " (INTEGER from expression)" << std::endl;
      } catch (const std::exception &e) {
        std::cout << "Error evaluating expression: " << e.what() << std::endl;
        return false;
      }
    }

    return true;
  }

  std::cout << "Syntax Error: Invalid command format" << std::endl;
  return false;
}

void KodeqParser::printVariables() {
  std::cout << "Variables:" << std::endl;
  for (auto &pair : variables) {
    std::cout << "$" << pair.first << " = ";
    if (pair.second->getType() == "INTEGER") {
      IntValue *intVal = static_cast<IntValue *>(pair.second);
      std::cout << intVal->getValue() << " (INTEGER)" << std::endl;
    } else {
      ModuleValue *modVal = static_cast<ModuleValue *>(pair.second);
      std::cout << modVal->getModuleName() << " (MODULE)" << std::endl;
    }
  }
}

bool KodeqParser::setModuleParameter(char varName, const std::string &paramName,
                                     int value) {
  BaseValue *val = getVariable(varName);
  if (!val || val->getType() != "MODULE") {
    std::cout << "Error: $" << varName << " is not a module" << std::endl;
    return false;
  }

  ModuleValue *modVal = static_cast<ModuleValue *>(val);
  modVal->setParameter(paramName, value);
  std::cout << "$" << varName << "." << paramName << " = " << value
            << std::endl;
  return true;
}

void KodeqParser::inspectVariable(char varName) {
  BaseValue *val = getVariable(varName);
  if (!val) {
    std::cout << "Variable $" << varName << " is not defined." << std::endl;
    return;
  }

  std::cout << "Variable $" << varName << ":" << std::endl;

  if (val->getType() == "INTEGER") {
    IntValue *intVal = static_cast<IntValue *>(val);
    std::cout << "Type: INTEGER" << std::endl;
    std::cout << "Value: " << intVal->getValue() << std::endl;

    // Binary representation
    std::cout << "Binary: ";
    for (int i = 7; i >= 0; i--) {
      std::cout << ((intVal->getValue() >> i) & 1);
    }
    std::cout << std::endl;

    // Hexadecimal representation
    std::cout << "Hex: 0x" << std::hex << intVal->getValue() << std::dec
              << std::endl;
  } else if (val->getType() == "MODULE") {
    ModuleValue *modVal = static_cast<ModuleValue *>(val);
    std::cout << "Type: MODULE (" << modVal->getModuleName() << ")"
              << std::endl;
    std::cout << "Current Value: " << modVal->toInt() << std::endl;
    std::cout << modVal->getVisualRepresentation() << std::endl;
  }
}

void KodeqParser::advanceTick() {
  tickCounter = (tickCounter + 1) % 256; // Allow for larger tick cycles
  std::cout << "Tick: " << tickCounter << std::endl;

  // Update POS parameter for all modules
  for (auto &pair : variables) {
    BaseValue *val = pair.second;
    if (val && val->getType() == "MODULE") {
      ModuleValue *modVal = static_cast<ModuleValue *>(val);
      // Update POS parameter if it exists
      modVal->setParameter("POS", tickCounter);

      // For index-based modules, also update I
      const std::string modType = modVal->getModuleName();
      if (modType == "PAT" || modType == "EUC") {
        modVal->setParameter("I", tickCounter);
      }
    }
  }
}

void KodeqParser::runTicks(int count) {
  for (int i = 0; i < count; i++) {
    advanceTick();
  }
  std::cout << "Ran " << count << " ticks. Current tick: " << tickCounter
            << std::endl;
}