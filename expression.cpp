#include "expression.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

// Check if a character is whitespace
bool ExpressionEvaluator::isSpace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Skip whitespace characters
void ExpressionEvaluator::skipWhitespace(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  while (it != end && isSpace(*it)) {
    ++it;
  }
}

// Check if the current character matches the expected one
bool ExpressionEvaluator::match(std::string::const_iterator &it,
                                const std::string::const_iterator &end,
                                char expected) {
  skipWhitespace(it, end);
  if (it != end && *it == expected) {
    ++it;
    return true;
  }
  return false;
}

// Evaluate an entire expression
int ExpressionEvaluator::evaluate(const std::string &expr) {
  auto it = expr.begin();
  auto end = expr.end();

  // Evaluate the full expression including conditional operators
  int result = parseConditional(it, end);

  skipWhitespace(it, end);
  if (it != end) {
    std::cerr << "Error: Unexpected character at end of expression: " << *it
              << std::endl;
  }

  return result;
}

// Parse function calls
int ExpressionEvaluator::parseFunction(std::string::const_iterator &it,
                                       const std::string::const_iterator &end) {
  skipWhitespace(it, end);

  if (it == end) {
    return 0;
  }

  // Save start position of function name
  auto funcStart = it;

  // Read function name (alphabetic characters)
  while (it != end && std::isalpha(*it)) {
    ++it;
  }

  // If no function name found, parse as primary expression
  if (it == funcStart) {
    return parsePrimary(it, end);
  }

  // Get function name
  std::string funcName(funcStart, it);
  std::transform(funcName.begin(), funcName.end(), funcName.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  // Check for opening parenthesis
  if (!match(it, end, '(')) {
    // No parenthesis, revert position and parse as primary
    it = funcStart;
    return parsePrimary(it, end);
  }

  // Parse argument list
  std::vector<int> args;

  if (!match(it, end, ')')) { // If not empty argument list
    while (true) {
      args.push_back(parseConditional(it, end)); // Parse argument

      if (match(it, end, ')')) {
        break; // End of argument list
      }

      if (!match(it, end, ',')) {
        std::cerr << "Error: Expected ',' or ')' in function arguments"
                  << std::endl;
        return 0;
      }
    }
  }

  // Identify and execute function
  if (funcName == "MIN") {
    if (args.size() != 2) {
      std::cerr << "Error: MIN requires 2 arguments" << std::endl;
      return 0;
    }
    return std::min(args[0], args[1]);
  } else if (funcName == "MAX") {
    if (args.size() != 2) {
      std::cerr << "Error: MAX requires 2 arguments" << std::endl;
      return 0;
    }
    return std::max(args[0], args[1]);
  } else if (funcName == "ABS") {
    if (args.size() != 1) {
      std::cerr << "Error: ABS requires 1 argument" << std::endl;
      return 0;
    }
    return std::abs(args[0]);
  } else if (funcName == "CLAMP") {
    if (args.size() != 3) {
      std::cerr << "Error: CLAMP requires 3 arguments" << std::endl;
      return 0;
    }
    return std::min(std::max(args[0], args[1]), args[2]);
  } else if (funcName == "RND") {
    if (args.size() != 2) {
      std::cerr << "Error: RND requires 2 arguments" << std::endl;
      return 0;
    }
    return parser.getRandom(args[0], args[1]);
  }

  std::cerr << "Error: Unknown function: " << funcName << std::endl;
  return 0;
}

// Parse primary expressions (literal values, variables, parenthesized
// expressions)
int ExpressionEvaluator::parsePrimary(std::string::const_iterator &it,
                                      const std::string::const_iterator &end) {
  skipWhitespace(it, end);

  if (it == end) {
    std::cerr << "Error: Unexpected end of expression" << std::endl;
    return 0;
  }

  // Check for function calls first
  if (std::isalpha(*it) && *it != 'X' && *it != 'x' && *it != 'T' &&
      *it != 't') {
    return parseFunction(it, end);
  }

  // Parenthesized expression
  if (*it == '(') {
    ++it; // Skip '('
    int value = parseConditional(it, end);
    if (!match(it, end, ')')) {
      std::cerr << "Error: Expected ')'" << std::endl;
      return 0;
    }
    return value;
  }

  // Unary minus
  if (*it == '-') {
    ++it;
    return -parsePrimary(it, end);
  }

  // Bitwise NOT
  if (*it == '~') {
    ++it;
    return ~parsePrimary(it, end);
  }

  // Variable reference ($X)
  if (*it == '$' && (it + 1) != end && std::isalpha(*(it + 1))) {
    char varName = std::toupper(*(it + 1));
    it += 2; // Skip "$X"

    BaseValue *value = parser.getVariable(varName);
    if (value) {
      return value->toInt();
    } else {
      std::cerr << "Error: Undefined variable $" << varName << std::endl;
      return 0;
    }
  }

  // Numeric literals
  if (std::isdigit(*it) || *it == '#' || *it == 'X' || *it == 'x') {
    std::string number;

    // Binary pattern (#1010)
    if (*it == '#') {
      number += *it;
      ++it;
      while (it != end && (*it == '0' || *it == '1')) {
        number += *it;
        ++it;
      }
      return parser.parseLiteral(number);
    }

    // Hexadecimal (XFF or xFF)
    if (*it == 'X' || *it == 'x') {
      number += 'X'; // Standardize to uppercase
      ++it;
      while (it != end && std::isxdigit(*it)) {
        number += std::toupper(*it);
        ++it;
      }
      return parser.parseLiteral(number);
    }

    // Decimal
    while (it != end && std::isdigit(*it)) {
      number += *it;
      ++it;
    }
    return std::stoi(number);
  }

  // System variable T (tick counter)
  if (*it == 'T' || *it == 't') {
    ++it;
    return parser.getTick();
  }

  std::cerr << "Error: Unexpected character in expression: " << *it
            << std::endl;
  return 0;
}

// Parse multiplication, division, and modulo operations
int ExpressionEvaluator::parseTerm(std::string::const_iterator &it,
                                   const std::string::const_iterator &end) {
  int left = parsePrimary(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end)
      break;

    if (*it == '*') {
      ++it;
      left *= parsePrimary(it, end);
    } else if (*it == '/') {
      ++it;
      int right = parsePrimary(it, end);
      if (right == 0) {
        std::cerr << "Error: Division by zero" << std::endl;
        return 0;
      }
      left /= right;
    } else if (*it == '%') {
      ++it;
      int right = parsePrimary(it, end);
      if (right == 0) {
        std::cerr << "Error: Modulo by zero" << std::endl;
        return 0;
      }
      left %= right;
    } else {
      break;
    }
  }

  return left;
}

// Parse addition and subtraction operations
int ExpressionEvaluator::parseAdditive(std::string::const_iterator &it,
                                       const std::string::const_iterator &end) {
  int left = parseTerm(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end)
      break;

    if (*it == '+') {
      ++it;
      left += parseTerm(it, end);
    } else if (*it == '-') {
      ++it;
      left -= parseTerm(it, end);
    } else {
      break;
    }
  }

  return left;
}

// Parse bit shift operations
int ExpressionEvaluator::parseShift(std::string::const_iterator &it,
                                    const std::string::const_iterator &end) {
  int left = parseAdditive(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end || it + 1 == end)
      break;

    if (*it == '<' && *(it + 1) == '<') {
      it += 2;
      left <<= parseAdditive(it, end);
    } else if (*it == '>' && *(it + 1) == '>') {
      it += 2;
      left >>= parseAdditive(it, end);
    } else {
      break;
    }
  }

  return left;
}

// Parse relational operators (<, >, <=, >=)
int ExpressionEvaluator::parseRelational(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  int left = parseShift(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end)
      break;

    if (*it == '<') {
      if (it + 1 != end && *(it + 1) == '=') {
        it += 2;
        left = (left <= parseShift(it, end)) ? 1 : 0;
      } else {
        ++it;
        left = (left < parseShift(it, end)) ? 1 : 0;
      }
    } else if (*it == '>') {
      if (it + 1 != end && *(it + 1) == '=') {
        it += 2;
        left = (left >= parseShift(it, end)) ? 1 : 0;
      } else {
        ++it;
        left = (left > parseShift(it, end)) ? 1 : 0;
      }
    } else {
      break;
    }
  }

  return left;
}

// Parse equality operators (==, !=)
int ExpressionEvaluator::parseEquality(std::string::const_iterator &it,
                                       const std::string::const_iterator &end) {
  int left = parseRelational(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end || it + 1 == end)
      break;

    if (*it == '=' && *(it + 1) == '=') {
      it += 2;
      left = (left == parseRelational(it, end)) ? 1 : 0;
    } else if (*it == '!' && *(it + 1) == '=') {
      it += 2;
      left = (left != parseRelational(it, end)) ? 1 : 0;
    } else {
      break;
    }
  }

  return left;
}

// Parse bitwise AND operations
int ExpressionEvaluator::parseBitwiseAnd(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  int left = parseEquality(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end)
      break;

    if (*it == '&' && (it + 1 == end || *(it + 1) != '&')) {
      ++it;
      left &= parseEquality(it, end);
    } else {
      break;
    }
  }

  return left;
}

// Parse bitwise XOR operations
int ExpressionEvaluator::parseBitwiseXor(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  int left = parseBitwiseAnd(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end)
      break;

    if (*it == '^') {
      ++it;
      left ^= parseBitwiseAnd(it, end);
    } else {
      break;
    }
  }

  return left;
}

// Parse bitwise OR operations
int ExpressionEvaluator::parseBitwiseOr(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  int left = parseBitwiseXor(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end)
      break;

    if (*it == '|' && (it + 1 == end || *(it + 1) != '|')) {
      ++it;
      left |= parseBitwiseXor(it, end);
    } else {
      break;
    }
  }

  return left;
}

// Parse logical AND operations
int ExpressionEvaluator::parseLogicalAnd(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  int left = parseBitwiseOr(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end || it + 1 == end)
      break;

    if (*it == '&' && *(it + 1) == '&') {
      it += 2;
      int right = parseBitwiseOr(it, end);
      left = (left && right) ? 1 : 0;
    } else {
      break;
    }
  }

  return left;
}

// Parse logical OR operations
int ExpressionEvaluator::parseLogicalOr(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  int left = parseLogicalAnd(it, end);

  while (true) {
    skipWhitespace(it, end);
    if (it == end || it + 1 == end)
      break;

    if (*it == '|' && *(it + 1) == '|') {
      it += 2;
      int right = parseLogicalAnd(it, end);
      left = (left || right) ? 1 : 0;
    } else {
      break;
    }
  }

  return left;
}

// Parse conditional operator (cond ? expr1 : expr2)
int ExpressionEvaluator::parseConditional(
    std::string::const_iterator &it, const std::string::const_iterator &end) {
  int condition = parseLogicalOr(it, end);

  skipWhitespace(it, end);
  if (it != end && *it == '?') {
    ++it;
    int trueValue = parseConditional(it, end);

    skipWhitespace(it, end);
    if (!match(it, end, ':')) {
      std::cerr << "Error: Expected ':' in conditional expression" << std::endl;
      return 0;
    }

    int falseValue = parseConditional(it, end);
    return condition ? trueValue : falseValue;
  }

  return condition;
}