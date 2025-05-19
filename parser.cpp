#include "parser.hpp"
#include <iostream>

// ライン全体をトークンに分割
std::vector<std::string> Parser::tokenizeLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;
    
    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
            token += c;
        } else if (c == ' ' && !inQuotes) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// パイプで区切られたコマンドに分割
std::vector<std::string> Parser::splitByPipe(const std::string& line) {
    std::vector<std::string> commands;
    std::string cmd;
    bool inQuotes = false;
    
    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
            cmd += c;
        } else if (c == '|' && !inQuotes) {
            if (!cmd.empty()) {
                commands.push_back(trim(cmd));
                cmd.clear();
            }
        } else {
            cmd += c;
        }
    }
    
    if (!cmd.empty()) {
        commands.push_back(trim(cmd));
    }
    
    return commands;
}

// 空白文字かどうかを判定
bool Parser::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// 文字列の前後の空白を削除
std::string Parser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// バイナリパターン判定（例: b10101010）
bool Parser::isBinaryPattern(const std::string& str) {
    if (str.size() <= 1 || str[0] != 'b') {
        return false;
    }
    
    for (size_t i = 1; i < str.size(); i++) {
        if (str[i] != '0' && str[i] != '1') {
            return false;
        }
    }
    
    return true;
}

// バイナリパターンの解析
int Parser::parseBinaryPattern(const std::string& str) {
    if (!isBinaryPattern(str)) {
        return 0;
    }
    
    int result = 0;
    for (size_t i = 1; i < str.size(); i++) {
        result = (result << 1) | (str[i] == '1' ? 1 : 0);
    }
    
    return result;
}

// クラス生成構文の処理: $seq = @seq
bool Parser::processClassCreation(const std::string& line) {
    std::regex pattern(R"(\$(\w+)\s*=\s*@(\w+))");
    std::smatch matches;
    
    if (std::regex_match(line, matches, pattern)) {
        std::string varName = matches[1].str();
        std::string className = matches[2].str();
        
        try {
            BaseObject* obj = ObjectFactory::createObject(className);
            env.setVariable(varName, obj);
            std::cout << "Created new object $" << varName << " of type " << className << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error creating object: " << e.what() << std::endl;
        }
    }
    
    return false;
}

// 属性アクセス構文の処理: $obj.attr = value または var = $obj.attr
bool Parser::processAttributeAccess(const std::string& line) {
    // 属性設定: $obj.attr = value
    std::regex setPattern(R"(\$(\w+)\.(\w+)\s*=\s*(.*))");
    std::smatch setMatches;
    
    if (std::regex_match(line, setMatches, setPattern)) {
        std::string objName = setMatches[1].str();
        std::string attrName = setMatches[2].str();
        std::string valueExpr = setMatches[3].str();
        
        BaseObject* obj = env.getVariable(objName);
        if (!obj) {
            std::cerr << "Error: Object $" << objName << " not found" << std::endl;
            return false;
        }
        
        // 式を評価して値を取得
        BaseObject* value = evaluateExpression(valueExpr);
        if (!value) {
            std::cerr << "Error evaluating expression: " << valueExpr << std::endl;
            return false;
        }
        
        try {
            obj->setAttribute(attrName, value);
            std::cout << "Set $" << objName << "." << attrName << " = " << value->toString() << std::endl;
            
            // 一時オブジェクトを解放
            delete value;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error setting attribute: " << e.what() << std::endl;
            delete value;
            return false;
        }
    }
    
    // 属性取得: var = $obj.attr
    std::regex getPattern(R"((\$?\w+)\s*=\s*\$(\w+)\.(\w+))");
    std::smatch getMatches;
    
    if (std::regex_match(line, getMatches, getPattern)) {
        std::string destName = getMatches[1].str();
        std::string objName = getMatches[2].str();
        std::string attrName = getMatches[3].str();
        
        // 先頭の$を削除（もしあれば）
        if (!destName.empty() && destName[0] == '$') {
            destName = destName.substr(1);
        }
        
        BaseObject* obj = env.getVariable(objName);
        if (!obj) {
            std::cerr << "Error: Object $" << objName << " not found" << std::endl;
            return false;
        }
        
        try {
            BaseObject* attrValue = obj->getAttribute(attrName);
            if (attrValue) {
                env.setVariable(destName, attrValue->clone());
                std::cout << "Got $" << objName << "." << attrName << " -> $" << destName << std::endl;
                return true;
            } else {
                std::cerr << "Error: Attribute " << attrName << " returned null" << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error getting attribute: " << e.what() << std::endl;
            return false;
        }
    }
    
    return false;
}

// メソッド呼び出し構文の処理: $obj.method()
bool Parser::processMethodCall(const std::string& line) {
    std::regex pattern(R"(\$(\w+)\.(\w+)\(\))");
    std::smatch matches;
    
    if (std::regex_match(line, matches, pattern)) {
        std::string objName = matches[1].str();
        std::string methodName = matches[2].str();
        
        BaseObject* obj = env.getVariable(objName);
        if (!obj) {
            std::cerr << "Error: Object $" << objName << " not found" << std::endl;
            return false;
        }
        
        // メソッド呼び出しを環境のイベントキューに登録
        if (methodName == "start") {
            // Seqオブジェクトのstart()メソッド
            if (obj->getType() == "seq") {
                auto event = [objName](Environment& env) {
                    BaseObject* obj = env.getVariable(objName);
                    if (obj && obj->getType() == "seq") {
                        static_cast<SeqObject*>(obj)->start();
                        std::cout << "Started sequence $" << objName << std::endl;
                    }
                };
                
                env.queueEvent(event);
                return true;
            } 
            // Countオブジェクトのstart()メソッド
            else if (obj->getType() == "count") {
                auto event = [objName](Environment& env) {
                    BaseObject* obj = env.getVariable(objName);
                    if (obj && obj->getType() == "count") {
                        static_cast<CountObject*>(obj)->start();
                        std::cout << "Started counter $" << objName << std::endl;
                    }
                };
                
                env.queueEvent(event);
                return true;
            }
        } 
        else if (methodName == "stop") {
            // Seqオブジェクトのstop()メソッド
            if (obj->getType() == "seq") {
                auto event = [objName](Environment& env) {
                    BaseObject* obj = env.getVariable(objName);
                    if (obj && obj->getType() == "seq") {
                        static_cast<SeqObject*>(obj)->stop();
                        std::cout << "Stopped sequence $" << objName << std::endl;
                    }
                };
                
                env.queueEvent(event);
                return true;
            } 
            // Countオブジェクトのstop()メソッド
            else if (obj->getType() == "count") {
                auto event = [objName](Environment& env) {
                    BaseObject* obj = env.getVariable(objName);
                    if (obj && obj->getType() == "count") {
                        static_cast<CountObject*>(obj)->stop();
                        std::cout << "Stopped counter $" << objName << std::endl;
                    }
                };
                
                env.queueEvent(event);
                return true;
            }
        } 
        else if (methodName == "reset" && obj->getType() == "count") {
            // Countオブジェクトのreset()メソッド
            auto event = [objName](Environment& env) {
                BaseObject* obj = env.getVariable(objName);
                if (obj && obj->getType() == "count") {
                    static_cast<CountObject*>(obj)->reset();
                    std::cout << "Reset counter $" << objName << std::endl;
                }
            };
            
            env.queueEvent(event);
            return true;
        }
        
        std::cerr << "Error: Unknown method or object type: $" << objName << "." << methodName << "()" << std::endl;
    }
    
    return false;
}

// 変数代入構文の処理: $var = value
bool Parser::processVariableAssignment(const std::string& line) {
    // クラス生成と属性アクセスは他のメソッドで処理されるため、
    // ここでは単純な値の代入のみ処理する
    std::regex pattern(R"(\$(\w+)\s*=\s*([^@].*))");
    std::smatch matches;
    
    if (std::regex_match(line, matches, pattern)) {
        std::string varName = matches[1].str();
        std::string valueExpr = matches[2].str();
        
        // 式が他の変数への参照かチェック
        std::regex varRefPattern(R"(\$(\w+))");
        std::smatch varRefMatches;
        
        if (std::regex_match(valueExpr, varRefMatches, varRefPattern)) {
            // 他の変数からのコピー
            std::string srcVarName = varRefMatches[1].str();
            BaseObject* srcObj = env.getVariable(srcVarName);
            
            if (!srcObj) {
                std::cerr << "Error: Variable $" << srcVarName << " not found" << std::endl;
                return false;
            }
            
            // クローンを作成して代入
            env.setVariable(varName, srcObj->clone());
            std::cout << "Copied $" << srcVarName << " to $" << varName << std::endl;
            return true;
        } 
        // バイナリパターンの処理
        else if (isBinaryPattern(valueExpr)) {
            int patternValue = parseBinaryPattern(valueExpr);
            env.setVariable(varName, new BinaryPatternObject(patternValue));
            std::cout << "Set $" << varName << " = " << valueExpr << std::endl;
            return true;
        }
        // 数値リテラルの処理
        else if (std::regex_match(valueExpr, std::regex(R"(\d+)"))) {
            int intValue = std::stoi(valueExpr);
            env.setVariable(varName, new IntObject(intValue));
            std::cout << "Set $" << varName << " = " << intValue << std::endl;
            return true;
        }
        // その他の式の評価
        else {
            BaseObject* evalResult = evaluateExpression(valueExpr);
            if (evalResult) {
                env.setVariable(varName, evalResult);
                std::cout << "Set $" << varName << " = " << evalResult->toString() << std::endl;
                return true;
            } else {
                std::cerr << "Error evaluating expression: " << valueExpr << std::endl;
                return false;
            }
        }
    }
    
    return false;
}

// パイプライン構文の処理: cmd1 | cmd2 | cmd3
bool Parser::processPipeline(const std::string& line) {
    auto commands = splitByPipe(line);
    
    if (commands.size() <= 1) {
        // パイプがない場合は他の処理メソッドに委譲
        return false;
    }
    
    bool success = true;
    
    for (const auto& cmd : commands) {
        // 各コマンドをメソッド呼び出しとして処理
        if (!processMethodCall(cmd)) {
            std::cerr << "Error in pipeline command: " << cmd << std::endl;
            success = false;
        }
    }
    
    return success;
}

// 式の評価
BaseObject* Parser::evaluateExpression(const std::string& expr) {
    // まずは単純なケースを処理
    std::string trimmedExpr = trim(expr);
    
    // 変数参照
    std::regex varRefPattern(R"(\$(\w+))");
    std::smatch varRefMatches;
    
    if (std::regex_match(trimmedExpr, varRefMatches, varRefPattern)) {
        std::string varName = varRefMatches[1].str();
        BaseObject* obj = env.getVariable(varName);
        
        if (!obj) {
            std::cerr << "Error: Variable $" << varName << " not found" << std::endl;
            return nullptr;
        }
        
        // 変数の値を複製して返す
        return obj->clone();
    }
    
    // バイナリパターン
    if (isBinaryPattern(trimmedExpr)) {
        int patternValue = parseBinaryPattern(trimmedExpr);
        return new BinaryPatternObject(patternValue);
    }
    
    // 数値リテラル
    if (std::regex_match(trimmedExpr, std::regex(R"(\d+)"))) {
        int intValue = std::stoi(trimmedExpr);
        return new IntObject(intValue);
    }
    
    // 今後、より複雑な式の評価を追加
    
    std::cerr << "Error: Could not evaluate expression: " << expr << std::endl;
    return nullptr;
}

// 行の解析と実行
bool Parser::parseLine(const std::string& line) {
    // コメント行や空行をスキップ
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty() || trimmedLine[0] == '#' || trimmedLine.substr(0, 2) == "//") {
        return true;
    }
    
    // 各種構文パターンを試す
    if (processClassCreation(trimmedLine)) {
        return true;
    }
    
    if (processAttributeAccess(trimmedLine)) {
        return true;
    }
    
    if (processPipeline(trimmedLine)) {
        return true;
    }
    
    if (processMethodCall(trimmedLine)) {
        return true;
    }
    
    if (processVariableAssignment(trimmedLine)) {
        return true;
    }
    
    // どのパターンにも一致しない
    std::cerr << "Syntax error: " << line << std::endl;
    return false;
}

// 複数行の解析と実行
bool Parser::parseMultipleLines(const std::string& code) {
    std::istringstream stream(code);
    std::string line;
    bool success = true;
    
    while (std::getline(stream, line)) {
        if (!parseLine(line)) {
            success = false;
        }
    }
    
    return success;
}