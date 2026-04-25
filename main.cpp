
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <clocale>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cwchar>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unordered_map>
#include <unistd.h>
#include <vector>
#include <wchar.h>

#ifndef AETHE_ENABLE_THREADS
#define AETHE_ENABLE_THREADS 0
#endif

#if AETHE_ENABLE_THREADS
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#endif

namespace aethe {

enum class TokenType {
    IDENTIFIER,
    NUMBER,
    STRING,
    SYMBOL,
    END
};

struct Token {
    TokenType type;
    std::string text;
    int line;
};

class Lexer {
public:
        explicit Lexer(const std::string& source)
        : source_(source), position_(0), line_(1) {
    }

        std::vector<Token> tokenize() {
        std::vector<Token> tokens;

        while (!isAtEnd()) {
            skipWhitespace();

            if (isAtEnd()) {
                break;
            }

            if (peek() == '/' && peekNext() == '/') {
                skipLineComment();
                continue;
            }

            const char current = peek();
            if (std::isdigit(static_cast<unsigned char>(current))) {
                tokens.push_back(readNumber());
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(current)) || current == '_') {
                tokens.push_back(readIdentifier());
                continue;
            }

            if (current == '"') {
                tokens.push_back(readString());
                continue;
            }

            tokens.push_back(readSymbol());
        }

        tokens.push_back(Token{TokenType::END, "", line_});
        return tokens;
    }

private:
    bool isAtEnd() const {
        return position_ >= source_.size();
    }

    char peek() const {
        return isAtEnd() ? '\0' : source_[position_];
    }

    char peekNext() const {
        return position_ + 1 >= source_.size() ? '\0' : source_[position_ + 1];
    }

    char advance() {
        const char current = source_[position_];
        ++position_;
        return current;
    }

    void skipWhitespace() {
        while (!isAtEnd()) {
            const char current = peek();
            if (current == '\n') {
                ++line_;
                ++position_;
                continue;
            }
            if (current == ' ' || current == '\t' || current == '\r') {
                ++position_;
                continue;
            }
            break;
        }
    }

    void skipLineComment() {
        while (!isAtEnd() && peek() != '\n') {
            ++position_;
        }
    }

    Token readNumber() {
        const int tokenLine = line_;
        std::string number;

        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            number.push_back(advance());
        }

        return Token{TokenType::NUMBER, number, tokenLine};
    }

    Token readIdentifier() {
        const int tokenLine = line_;
        std::string identifier;

        while (!isAtEnd()) {
            const char current = peek();
            if (std::isalnum(static_cast<unsigned char>(current)) || current == '_') {
                identifier.push_back(advance());
            } else {
                break;
            }
        }

        return Token{TokenType::IDENTIFIER, identifier, tokenLine};
    }

    Token readString() {
        const int tokenLine = line_;
        std::string value;
        advance();

        while (!isAtEnd() && peek() != '"') {
            const char current = advance();
            if (current == '\\') {
                if (isAtEnd()) {
                    lexError("unterminated string escape sequence");
                }

                const char escaped = advance();
                if (escaped == 'n') {
                    value.push_back('\n');
                } else if (escaped == 't') {
                    value.push_back('\t');
                } else if (escaped == '"') {
                    value.push_back('"');
                } else if (escaped == '\\') {
                    value.push_back('\\');
                } else {
                    lexError("unsupported string escape sequence");
                }
                continue;
            }

            if (current == '\n') {
                ++line_;
            }
            value.push_back(current);
        }

        if (isAtEnd()) {
            lexError("unterminated string literal");
        }

        advance();
        return Token{TokenType::STRING, value, tokenLine};
    }

    Token readSymbol() {
        const int tokenLine = line_;

        if (peek() == '|' && peekNext() == '>') {
            position_ += 2;
            return Token{TokenType::SYMBOL, "|>", tokenLine};
        }

        if (peek() == '|' && peekNext() == '?') {
            position_ += 2;
            return Token{TokenType::SYMBOL, "|?", tokenLine};
        }

        if ((peek() == '=' && peekNext() == '=') ||
            (peek() == '!' && peekNext() == '=') ||
            (peek() == '<' && peekNext() == '=') ||
            (peek() == '>' && peekNext() == '=') ||
            (peek() == '+' && peekNext() == '=') ||
            (peek() == '-' && peekNext() == '=') ||
            (peek() == '*' && peekNext() == '=') ||
            (peek() == '/' && peekNext() == '=') ||
            (peek() == '%' && peekNext() == '=') ||
            (peek() == '&' && peekNext() == '&') ||
            (peek() == '|' && peekNext() == '|')) {
            const char first = advance();
            const char second = advance();
            return Token{TokenType::SYMBOL, std::string(1, first) + std::string(1, second), tokenLine};
        }

        const char symbol = advance();
        const std::string text(1, symbol);
        if (text == "[" || text == "]" || text == "{" || text == "}" ||
            text == "(" || text == ")" || text == "," || text == ":" ||
            text == ";" || text == "$" || text == "." || text == "+" ||
            text == "-" || text == "*" || text == "/" || text == "%" ||
            text == "!" || text == "<" || text == ">" || text == "=") {
            return Token{TokenType::SYMBOL, text, tokenLine};
        }

        lexError("unexpected character '" + text + "'");
    }

    [[noreturn]] void lexError(const std::string& message) const {
        throw std::runtime_error("Lex Error(line " + std::to_string(line_) + "): " + message);
    }

    std::string source_;
    size_t position_;
    int line_;
};

struct ObjectData;
struct CallableData;
struct StreamData;
struct CompiledFunction;
struct CompiledProgram;
class Interpreter;
class BytecodeVirtualMachine;

class Value {
public:
    enum Type {
        INT,
        BOOL,
        STRING,
        NIL,
        ARRAY,
        DICT,
        OBJECT,
        CALLABLE,
        STREAM,
        RESULT_OK,
        RESULT_ERR
    };

    Value();
    explicit Value(int value);
    explicit Value(bool value);
    explicit Value(const std::string& value);
    explicit Value(const char* value);
    explicit Value(const std::vector<Value>& value);
    explicit Value(const std::unordered_map<std::string, Value>& value);
    explicit Value(const std::shared_ptr<ObjectData>& value);
    explicit Value(const std::shared_ptr<CallableData>& value);
    explicit Value(const std::shared_ptr<StreamData>& value);

        bool isTruthy() const {
        switch (type) {
            case INT:
                return intValue != 0;
            case BOOL:
                return boolValue;
            case STRING:
                return !stringValue.empty();
            case ARRAY:
                return !asArray().empty();
            case DICT:
                return !asDict().empty();
            case OBJECT:
                return objectValue.get() != nullptr;
            case CALLABLE:
                return callableValue.get() != nullptr;
            case STREAM:
                return streamValue.get() != nullptr;
            case NIL:
                return false;
            default:
                return false;
        }
    }

        std::string typeName() const {
        switch (type) {
            case INT:
                return "int";
            case BOOL:
                return "bool";
            case STRING:
                return "string";
            case ARRAY:
                return "array";
            case DICT:
                return "dict";
            case OBJECT:
                return "object";
            case CALLABLE:
                return "callable";
            case STREAM:
                return "stream";
            case RESULT_OK:
                return "ok";
            case RESULT_ERR:
                return "err";
            case NIL:
                return "nil";
            default:
                return "unknown";
        }
    }

        std::string toString() const;
        bool equals(const Value& other) const;
        const std::vector<Value>& asArray() const;
        std::vector<Value>& mutableArray();
        const std::unordered_map<std::string, Value>& asDict() const;
        std::unordered_map<std::string, Value>& mutableDict();

    Type type;
    int intValue;
    bool boolValue;
    std::string stringValue;
    std::shared_ptr<std::vector<Value> > arrayValue;
    std::shared_ptr<std::unordered_map<std::string, Value> > dictValue;
    std::shared_ptr<ObjectData> objectValue;
    std::shared_ptr<CallableData> callableValue;
    std::shared_ptr<StreamData> streamValue;
    std::shared_ptr<Value> resultValue;

    static Value Ok(const Value& val) {
        Value v;
        v.type = RESULT_OK;
        v.resultValue = std::make_shared<Value>(val);
        return v;
    }

    static Value Err(const Value& val) {
        Value v;
        v.type = RESULT_ERR;
        v.resultValue = std::make_shared<Value>(val);
        return v;
    }
};

Value::Value()
    : type(NIL), intValue(0), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue(), callableValue(), streamValue(), resultValue() {
}

Value::Value(int value)
    : type(INT), intValue(value), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue(), callableValue(), streamValue(), resultValue() {
}

Value::Value(bool value)
    : type(BOOL), intValue(0), boolValue(value), stringValue(), arrayValue(), dictValue(), objectValue(), callableValue(), streamValue(), resultValue() {
}

Value::Value(const std::string& value)
    : type(STRING), intValue(0), boolValue(false), stringValue(value), arrayValue(), dictValue(), objectValue(), callableValue(), streamValue(), resultValue() {
}

Value::Value(const char* value)
    : type(STRING), intValue(0), boolValue(false), stringValue(value == nullptr ? "" : value), arrayValue(), dictValue(), objectValue(), callableValue(), streamValue(), resultValue() {
}

Value::Value(const std::vector<Value>& value)
    : type(ARRAY),
      intValue(0),
      boolValue(false),
      stringValue(),
      arrayValue(new std::vector<Value>(value)),
      dictValue(),
      objectValue(),
      callableValue(),
      streamValue(),
      resultValue() {
}

Value::Value(const std::unordered_map<std::string, Value>& value)
    : type(DICT),
      intValue(0),
      boolValue(false),
      stringValue(),
      arrayValue(),
      dictValue(new std::unordered_map<std::string, Value>(value)),
      objectValue(),
      callableValue(),
      streamValue(),
      resultValue() {
}

Value::Value(const std::shared_ptr<ObjectData>& value)
    : type(OBJECT), intValue(0), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue(value), callableValue(), streamValue(), resultValue() {
}

const std::vector<Value>& Value::asArray() const {
    static const std::vector<Value> empty;
    return arrayValue.get() == nullptr ? empty : *arrayValue;
}

std::vector<Value>& Value::mutableArray() {
    if (arrayValue.get() == nullptr) {
        arrayValue.reset(new std::vector<Value>());
    }
    return *arrayValue;
}

const std::unordered_map<std::string, Value>& Value::asDict() const {
    static const std::unordered_map<std::string, Value> empty;
    return dictValue.get() == nullptr ? empty : *dictValue;
}

std::unordered_map<std::string, Value>& Value::mutableDict() {
    if (dictValue.get() == nullptr) {
        dictValue.reset(new std::unordered_map<std::string, Value>());
    }
    return *dictValue;
}

struct ObjectData {
    explicit ObjectData(const std::string& type)
        : typeName(type), fields() {
    }

    std::string typeName;
    std::unordered_map<std::string, Value> fields;
};

std::string Value::toString() const {
    switch (type) {
        case INT:
            return std::to_string(intValue);
        case BOOL:
            return boolValue ? "true" : "false";
        case STRING:
            return stringValue;
        case NIL:
            return "nil";
        case ARRAY: {
            const std::vector<Value>& values = asArray();
            std::ostringstream stream;
            stream << "[";
            for (size_t index = 0; index < values.size(); ++index) {
                stream << values[index].toString();
                if (index + 1 < values.size()) {
                    stream << ", ";
                }
            }
            stream << "]";
            return stream.str();
        }
        case DICT: {
            const std::unordered_map<std::string, Value>& entries = asDict();
            std::vector<std::string> keys;
            keys.reserve(entries.size());
            for (std::unordered_map<std::string, Value>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                keys.push_back(it->first);
            }
            std::sort(keys.begin(), keys.end());

            std::ostringstream stream;
            stream << "{";
            for (size_t index = 0; index < keys.size(); ++index) {
                const std::string& key = keys[index];
                stream << '"' << key << "\": " << entries.find(key)->second.toString();
                if (index + 1 < keys.size()) {
                    stream << ", ";
                }
            }
            stream << "}";
            return stream.str();
        }
        case OBJECT: {
            if (objectValue.get() == nullptr) {
                return "<object nil>";
            }

            std::vector<std::string> keys;
            keys.reserve(objectValue->fields.size());
            for (std::unordered_map<std::string, Value>::const_iterator it = objectValue->fields.begin(); it != objectValue->fields.end(); ++it) {
                keys.push_back(it->first);
            }
            std::sort(keys.begin(), keys.end());

            std::ostringstream stream;
            stream << objectValue->typeName << "{";
            for (size_t index = 0; index < keys.size(); ++index) {
                const std::string& key = keys[index];
                stream << key << ": " << objectValue->fields.find(key)->second.toString();
                if (index + 1 < keys.size()) {
                    stream << ", ";
                }
            }
            stream << "}";
            return stream.str();
        }
        case CALLABLE:
            return callableValue.get() == nullptr ? "<pipe nil>" : "<pipe>";
        case STREAM:
            return streamValue.get() == nullptr ? "<stream nil>" : "<stream>";
        case RESULT_OK:
            return "Ok(" + (resultValue.get() == nullptr ? "nil" : resultValue->toString()) + ")";
        case RESULT_ERR:
            return "Err(" + (resultValue.get() == nullptr ? "nil" : resultValue->toString()) + ")";
        default:
            return "nil";
    }
}

bool Value::equals(const Value& other) const {
    if (type != other.type) {
        return false;
    }

    switch (type) {
        case INT:
            return intValue == other.intValue;
        case BOOL:
            return boolValue == other.boolValue;
        case STRING:
            return stringValue == other.stringValue;
        case NIL:
            return true;
        case ARRAY:
            if (asArray().size() != other.asArray().size()) {
                return false;
            }
            for (size_t index = 0; index < asArray().size(); ++index) {
                if (!asArray()[index].equals(other.asArray()[index])) {
                    return false;
                }
            }
            return true;
        case DICT:
            if (asDict().size() != other.asDict().size()) {
                return false;
            }
            for (std::unordered_map<std::string, Value>::const_iterator it = asDict().begin(); it != asDict().end(); ++it) {
                std::unordered_map<std::string, Value>::const_iterator matchEntry = other.asDict().find(it->first);
                if (matchEntry == other.asDict().end() || !it->second.equals(matchEntry->second)) {
                    return false;
                }
            }
            return true;
        case RESULT_OK:
        case RESULT_ERR:
            if (resultValue.get() == nullptr || other.resultValue.get() == nullptr) {
                return resultValue.get() == other.resultValue.get();
            }
            return resultValue->equals(*other.resultValue);
        case OBJECT:
            if (objectValue.get() == nullptr || other.objectValue.get() == nullptr) {
                return objectValue.get() == other.objectValue.get();
            }
            if (objectValue->typeName != other.objectValue->typeName ||
                objectValue->fields.size() != other.objectValue->fields.size()) {
                return false;
            }
            for (std::unordered_map<std::string, Value>::const_iterator it = objectValue->fields.begin(); it != objectValue->fields.end(); ++it) {
                std::unordered_map<std::string, Value>::const_iterator matchEntry = other.objectValue->fields.find(it->first);
                if (matchEntry == other.objectValue->fields.end() || !it->second.equals(matchEntry->second)) {
                    return false;
                }
            }
            return true;
        case CALLABLE:
            return callableValue.get() == other.callableValue.get();
        case STREAM:
            return streamValue.get() == other.streamValue.get();
        default:
            return false;
    }
}

struct Expr {
    virtual ~Expr() {}
};

struct LiteralExpr : Expr {
    explicit LiteralExpr(const Value& literal) : value(literal) {}
    Value value;
};

struct VariableExpr : Expr {
    explicit VariableExpr(const std::string& variableName) : name(variableName) {}
    std::string name;
};

struct IdentifierExpr : Expr {
    explicit IdentifierExpr(const std::string& identifierName) : name(identifierName) {}
    std::string name;
};

struct PlaceholderExpr : Expr {
};

struct ArrayExpr : Expr {
    explicit ArrayExpr(std::vector<std::unique_ptr<Expr> > values) : elements(std::move(values)) {}
    std::vector<std::unique_ptr<Expr> > elements;
};

struct DictExpr : Expr {
    explicit DictExpr(std::vector<std::pair<std::string, std::unique_ptr<Expr> > > values)
        : entries(std::move(values)) {
    }

    std::vector<std::pair<std::string, std::unique_ptr<Expr> > > entries;
};

struct UnaryExpr : Expr {
    UnaryExpr(const std::string& unaryOperator, std::unique_ptr<Expr> operand)
        : op(unaryOperator), right(std::move(operand)) {
    }

    std::string op;
    std::unique_ptr<Expr> right;
};

struct BinaryExpr : Expr {
    BinaryExpr(std::unique_ptr<Expr> leftExpr, const std::string& binaryOperator, std::unique_ptr<Expr> rightExpr)
        : left(std::move(leftExpr)), op(binaryOperator), right(std::move(rightExpr)) {
    }

    std::unique_ptr<Expr> left;
    std::string op;
    std::unique_ptr<Expr> right;
};

struct AssignExpr : Expr {
    AssignExpr(std::unique_ptr<Expr> targetExpr, const std::string& assignOperator, std::unique_ptr<Expr> valueExpr)
        : target(std::move(targetExpr)), op(assignOperator), value(std::move(valueExpr)) {
    }

    std::unique_ptr<Expr> target;
    std::string op;
    std::unique_ptr<Expr> value;
};

struct MemberExpr : Expr {
    MemberExpr(std::unique_ptr<Expr> objectExpr, const std::string& member)
        : object(std::move(objectExpr)), memberName(member) {
    }

    std::unique_ptr<Expr> object;
    std::string memberName;
};

struct IndexExpr : Expr {
    IndexExpr(std::unique_ptr<Expr> containerExpr, std::unique_ptr<Expr> indexExpr)
        : container(std::move(containerExpr)), index(std::move(indexExpr)) {
    }

    std::unique_ptr<Expr> container;
    std::unique_ptr<Expr> index;
};

struct CallExpr : Expr {
    CallExpr(std::unique_ptr<Expr> calleeExpr, std::vector<std::unique_ptr<Expr> > arguments)
        : callee(std::move(calleeExpr)), args(std::move(arguments)) {
    }

    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr> > args;
};

struct PipelineExpr : Expr {
    struct Stage {
        std::string op;
        std::unique_ptr<Expr> expr;
    };

    explicit PipelineExpr(std::unique_ptr<Expr> sourceExpr)
        : source(std::move(sourceExpr)), stages() {
    }

    std::unique_ptr<Expr> source;
    std::vector<Stage> stages;
};

struct Statement {
    enum Type {
        EXPR,
        FLOW_DEF,
        STAGE_DEF,
        TYPE_DEF,
        WHEN,
        MATCH,
        WHILE_LOOP,
        FOR_LOOP,
        RETURN,
        DEFER,
        BREAK_LOOP,
        CONTINUE_LOOP
    };

    explicit Statement(Type statementType, int statementLine)
        : type(statementType), line(statementLine), name(), params(), expr(), body(), elseBody(), methods(), arms(), armGuards(), armBodies() {
    }

    Type type;
    int line;
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<Expr> expr;
    std::vector<std::unique_ptr<Statement> > body;
    std::vector<std::unique_ptr<Statement> > elseBody;
    std::vector<std::unique_ptr<Statement> > methods;
    std::vector<std::unique_ptr<Expr> > arms;
    std::vector<std::unique_ptr<Expr> > armGuards;
    std::vector<std::vector<std::unique_ptr<Statement> > > armBodies;
};

struct PipeExpr : Expr {
    PipeExpr(std::vector<std::string> parameterNames, std::vector<std::unique_ptr<Statement> > callableBody)
        : params(std::move(parameterNames)), body(std::move(callableBody)) {
    }

    std::vector<std::string> params;
    std::vector<std::unique_ptr<Statement> > body;
};

struct CallableData {
    enum Kind {
        PIPE_BODY,
        BOUND,
        CHAIN,
        BRANCH,
        GUARD
    };

    CallableData(std::vector<std::string> parameterNames,
                 const std::vector<std::unique_ptr<Statement> >* callableBody,
                 const std::unordered_map<std::string, Value>& capturedValues)
        : kind(PIPE_BODY),
          params(std::move(parameterNames)),
          body(callableBody),
          compiledBody(),
          captured(capturedValues),
          parts(),
          label("pipe") {
    }

    CallableData(std::vector<std::string> parameterNames,
                 const std::shared_ptr<CompiledFunction>& compiledCallableBody,
                 const std::unordered_map<std::string, Value>& capturedValues)
        : kind(PIPE_BODY),
          params(std::move(parameterNames)),
          body(nullptr),
          compiledBody(compiledCallableBody),
          captured(capturedValues),
          parts(),
          label("pipe") {
    }

    CallableData(Kind callableKind,
                 std::vector<Value> storedParts,
                 const std::string& callableLabel)
        : kind(callableKind),
          params(),
          body(nullptr),
          compiledBody(),
          captured(),
          parts(std::move(storedParts)),
          label(callableLabel) {
    }

    Kind kind;
    std::vector<std::string> params;
    const std::vector<std::unique_ptr<Statement> >* body;
    std::shared_ptr<CompiledFunction> compiledBody;
    std::unordered_map<std::string, Value> captured;
    std::vector<Value> parts;
    std::string label;
};

struct StreamData {
    enum OpKind {
        MAP,
        FILTER,
        TAP,
        SKIP
    };

    struct Operation {
        explicit Operation(OpKind operationKind)
            : kind(operationKind), callable(), extraArgs(), count(0) {
        }

        OpKind kind;
        Value callable;
        std::vector<Value> extraArgs;
        int count;
    };

    StreamData()
        : start(0), end(0), hasEnd(false), step(1), operations() {
    }

    int start;
    int end;
    bool hasEnd;
    int step;
    std::vector<Operation> operations;
};

class BytecodeVirtualMachine {
public:
    static void executeProgram(Interpreter* runtime,
                               const CompiledProgram& compiled);
    static Value runFlow(Interpreter* runtime,
                         const CompiledFunction& function,
                         const std::vector<Value>& args,
                         const Value* self);
    static Value runStage(Interpreter* runtime,
                          const CompiledFunction& function,
                          const Value& input,
                          const std::vector<Value>& args);
    static Value runPipe(Interpreter* runtime,
                         const CompiledFunction& function,
                         const std::unordered_map<std::string, Value>& captured,
                         const std::vector<Value>& args);

private:
    static Value runFunction(Interpreter* runtime,
                             const CompiledFunction& function,
                             const std::vector<Value>& args,
                             const std::unordered_map<std::string, Value>* captured,
                             const Value* self,
                             const Value* stageInput,
                             bool createScope,
                             bool incrementCallDepth);
    static Value runInlinePipeExpr(Interpreter* runtime,
                                   const CompiledFunction& function,
                                   const Value& input);
};

Value::Value(const std::shared_ptr<CallableData>& value)
    : type(CALLABLE), intValue(0), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue(), callableValue(value), streamValue(), resultValue() {
}

Value::Value(const std::shared_ptr<StreamData>& value)
    : type(STREAM), intValue(0), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue(), callableValue(), streamValue(value), resultValue() {
}

class Parser {
public:
        explicit Parser(const std::vector<Token>& tokens)
        : tokens_(tokens), position_(0) {
    }

        std::vector<std::unique_ptr<Statement> > parseProgram() {
        std::vector<std::unique_ptr<Statement> > program;
        while (!isAtEnd()) {
            program.push_back(parseStatement());
        }
        return program;
    }

private:
    bool isAtEnd() const {
        return peek().type == TokenType::END;
    }

    const Token& peek() const {
        return tokens_[position_];
    }

    const Token& previous() const {
        return tokens_[position_ - 1];
    }

    const Token& advance() {
        if (!isAtEnd()) {
            ++position_;
        }
        return previous();
    }

    bool checkSymbol(const std::string& text) const {
        return !isAtEnd() && peek().type == TokenType::SYMBOL && peek().text == text;
    }

    bool matchSymbol(const std::string& text) {
        if (checkSymbol(text)) {
            advance();
            return true;
        }
        return false;
    }

    bool checkType(TokenType type) const {
        return !isAtEnd() && peek().type == type;
    }

    bool matchType(TokenType type) {
        if (checkType(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool checkKeyword(const std::string& text) const {
        return !isAtEnd() && peek().type == TokenType::IDENTIFIER && peek().text == text;
    }

    bool matchKeyword(const std::string& text) {
        if (checkKeyword(text)) {
            advance();
            return true;
        }
        return false;
    }

    const Token& expectSymbol(const std::string& text, const std::string& message) {
        if (!checkSymbol(text)) {
            syntaxError(message);
        }
        return advance();
    }

    const Token& expectType(TokenType type, const std::string& message) {
        if (!checkType(type)) {
            syntaxError(message);
        }
        return advance();
    }

    const Token& expectKeyword(const std::string& text, const std::string& message) {
        if (!checkKeyword(text)) {
            syntaxError(message);
        }
        return advance();
    }

    std::unique_ptr<Statement> parseStatement() {
        if (matchKeyword("flow") || matchKeyword("fn")) {
            return parseCallableDefinition(Statement::FLOW_DEF);
        }

        if (matchKeyword("stage") || matchKeyword("stream")) {
            return parseCallableDefinition(Statement::STAGE_DEF);
        }

        if (matchKeyword("type")) {
            return parseTypeDefinition();
        }

        if (matchKeyword("when")) {
            return parseWhenStatement();
        }

        if (matchKeyword("match")) {
            return parseMatchStatement();
        }

        if (matchKeyword("while")) {
            return parseWhileStatement();
        }

        if (matchKeyword("for")) {
            return parseForStatement();
        }

        if (matchKeyword("give") || matchKeyword("return")) {
            return parseReturnStatement();
        }

        if (matchKeyword("let")) {
            return parseLetStatement();
        }

        if (matchKeyword("defer")) {
            return parseDeferStatement();
        }

        if (matchKeyword("break")) {
            return parseSignalStatement(Statement::BREAK_LOOP);
        }

        if (matchKeyword("continue")) {
            return parseSignalStatement(Statement::CONTINUE_LOOP);
        }

        return parseExprStatement();
    }

    std::unique_ptr<Statement> parseCallableDefinition(Statement::Type type) {
        const Token& name = expectType(TokenType::IDENTIFIER, "expected callable name");
        std::unique_ptr<Statement> statement(new Statement(type, name.line));
        statement->name = name.text;
        statement->params = parseParameterNames();
        statement->body = parseBlock();
        return statement;
    }

    std::unique_ptr<Statement> parseTypeDefinition() {
        const Token& name = expectType(TokenType::IDENTIFIER, "expected type name");
        std::unique_ptr<Statement> statement(new Statement(Statement::TYPE_DEF, name.line));
        statement->name = name.text;
        statement->params = parseParameterNames();
        expectSymbol("{", "expected '{' to open type body");
        while (!checkSymbol("}")) {
            if (!matchKeyword("flow") && !matchKeyword("fn")) {
                syntaxError("type body accepts only flow or fn methods");
            }
            statement->methods.push_back(parseCallableDefinition(Statement::FLOW_DEF));
        }
        expectSymbol("}", "expected '}' to close type body");
        return statement;
    }

    std::unique_ptr<Statement> parseWhenStatement() {
        std::unique_ptr<Statement> statement(new Statement(Statement::WHEN, previous().line));
        statement->expr = parseExpression();
        statement->body = parseBlock();
        if (matchKeyword("else")) {
            statement->elseBody = parseBlock();
        }
        return statement;
    }

    std::unique_ptr<Statement> parseMatchStatement() {
        std::unique_ptr<Statement> statement(new Statement(Statement::MATCH, previous().line));
        statement->expr = parseExpression();
        expectSymbol("{", "expected '{' to open match body");
        while (!checkSymbol("}")) {
            if (matchKeyword("case")) {
                statement->arms.push_back(parseExpression());
                if (matchKeyword("when")) {
                    statement->armGuards.push_back(parseExpression());
                } else {
                    statement->armGuards.push_back(std::unique_ptr<Expr>());
                }
                statement->armBodies.push_back(parseBlock());
                continue;
            }
            if (matchKeyword("else")) {
                statement->elseBody = parseBlock();
                continue;
            }
            syntaxError("match only accepts case and else branches");
        }
        expectSymbol("}", "expected '}' to close match body");
        return statement;
    }

    std::unique_ptr<Statement> parseWhileStatement() {
        std::unique_ptr<Statement> statement(new Statement(Statement::WHILE_LOOP, previous().line));
        statement->expr = parseExpression();
        statement->body = parseBlock();
        return statement;
    }

    std::unique_ptr<Statement> parseForStatement() {
        const Token& iterator = expectType(TokenType::IDENTIFIER, "expected iterator variable name");
        std::unique_ptr<Statement> statement(new Statement(Statement::FOR_LOOP, iterator.line));
        statement->name = iterator.text;
        if (matchSymbol(",")) {
            statement->params.push_back(expectType(TokenType::IDENTIFIER, "expected second iterator variable name").text);
        }
        expectKeyword("in", "expected 'in' in for statement");
        statement->expr = parseExpression();
        statement->body = parseBlock();
        return statement;
    }

    std::unique_ptr<Statement> parseReturnStatement() {
        std::unique_ptr<Statement> statement(new Statement(Statement::RETURN, previous().line));
        if (!checkSymbol(";")) {
            statement->expr = parseExpression();
        }
        expectSymbol(";", "expected ';' after return statement");
        return statement;
    }

    std::unique_ptr<Statement> parseLetStatement() {
        const Token& name = expectType(TokenType::IDENTIFIER, "expected variable name after let");
        expectSymbol("=", "expected '=' after variable name");
        std::unique_ptr<Expr> value = parseExpression();
        expectSymbol(";", "expected ';' after let statement");

        std::unique_ptr<Statement> statement(new Statement(Statement::EXPR, name.line));
        statement->expr = wrapIntoStage(std::move(value), name.text);
        return statement;
    }

    std::unique_ptr<Statement> parseDeferStatement() {
        std::unique_ptr<Statement> statement(new Statement(Statement::DEFER, previous().line));
        statement->body = parseBlock();
        return statement;
    }

    std::unique_ptr<Statement> parseSignalStatement(Statement::Type type) {
        std::unique_ptr<Statement> statement(new Statement(type, previous().line));
        expectSymbol(";", "expected ';' after control statement");
        return statement;
    }

    std::unique_ptr<Statement> parseExprStatement() {
        std::unique_ptr<Statement> statement(new Statement(Statement::EXPR, peek().line));
        statement->expr = parseExpression();
        expectSymbol(";", "expected ';' to terminate statement");
        return statement;
    }

    std::vector<std::string> parseParameterNames() {
        std::vector<std::string> params;
        expectSymbol("(", "expected '('");
        if (!checkSymbol(")")) {
            do {
                params.push_back(expectType(TokenType::IDENTIFIER, "expected parameter name").text);
            } while (matchSymbol(","));
        }
        expectSymbol(")", "expected ')'");
        return params;
    }

    std::vector<std::unique_ptr<Statement> > parseBlock() {
        std::vector<std::unique_ptr<Statement> > body;
        expectSymbol("{", "expected '{'");
        while (!checkSymbol("}")) {
            body.push_back(parseStatement());
        }
        expectSymbol("}", "expected '}'");
        return body;
    }

    std::unique_ptr<Expr> parseExpression() {
        return parseAssignment();
    }

    bool isAssignmentOperator(const std::string& text) const {
        return text == "=" ||
               text == "+=" ||
               text == "-=" ||
               text == "*=" ||
               text == "/=" ||
               text == "%=";
    }

    std::unique_ptr<Expr> parseAssignment() {
        std::unique_ptr<Expr> expr = parsePipeline();
        if (!isAtEnd() && peek().type == TokenType::SYMBOL && isAssignmentOperator(peek().text)) {
            const std::string op = advance().text;
            expr.reset(new AssignExpr(std::move(expr), op, parseAssignment()));
        }
        return expr;
    }

    std::unique_ptr<Expr> parsePipeline() {
        std::unique_ptr<Expr> expr = parseLogicalOr();
        while (checkSymbol("|>") || checkSymbol("|?")) {
            const std::string op = advance().text;
            std::unique_ptr<PipelineExpr> pipeline;
            PipelineExpr* existing = dynamic_cast<PipelineExpr*>(expr.get());
            if (existing != nullptr) {
                pipeline.reset(static_cast<PipelineExpr*>(expr.release()));
            } else {
                pipeline.reset(new PipelineExpr(std::move(expr)));
            }
            pipeline->stages.push_back({op, parseLogicalOr()});
            expr = std::move(pipeline);
        }
        return expr;
    }

    std::unique_ptr<Expr> parseLogicalOr() {
        std::unique_ptr<Expr> expr = parseLogicalAnd();
        while (matchSymbol("||")) {
            expr.reset(new BinaryExpr(std::move(expr), "||", parseLogicalAnd()));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseLogicalAnd() {
        std::unique_ptr<Expr> expr = parseEquality();
        while (matchSymbol("&&")) {
            expr.reset(new BinaryExpr(std::move(expr), "&&", parseEquality()));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseEquality() {
        std::unique_ptr<Expr> expr = parseComparison();
        while (checkSymbol("==") || checkSymbol("!=")) {
            const std::string op = advance().text;
            expr.reset(new BinaryExpr(std::move(expr), op, parseComparison()));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseComparison() {
        std::unique_ptr<Expr> expr = parseTerm();
        while (checkSymbol(">") || checkSymbol(">=") || checkSymbol("<") || checkSymbol("<=")) {
            const std::string op = advance().text;
            expr.reset(new BinaryExpr(std::move(expr), op, parseTerm()));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseTerm() {
        std::unique_ptr<Expr> expr = parseFactor();
        while (checkSymbol("+") || checkSymbol("-")) {
            const std::string op = advance().text;
            expr.reset(new BinaryExpr(std::move(expr), op, parseFactor()));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseFactor() {
        std::unique_ptr<Expr> expr = parseUnary();
        while (checkSymbol("*") || checkSymbol("/") || checkSymbol("%")) {
            const std::string op = advance().text;
            expr.reset(new BinaryExpr(std::move(expr), op, parseUnary()));
        }
        return expr;
    }

    std::unique_ptr<Expr> parseUnary() {
        if (matchSymbol("!")) {
            return std::unique_ptr<Expr>(static_cast<Expr*>(new UnaryExpr("!", parseUnary())));
        }
        if (matchSymbol("-")) {
            return std::unique_ptr<Expr>(static_cast<Expr*>(new UnaryExpr("-", parseUnary())));
        }
        return parsePostfix();
    }

    std::unique_ptr<Expr> parsePostfix() {
        std::unique_ptr<Expr> expr = parsePrimary();
        while (true) {
            if (matchSymbol("(")) {
                std::vector<std::unique_ptr<Expr> > args;
                if (!checkSymbol(")")) {
                    do {
                        args.push_back(parseExpression());
                    } while (matchSymbol(","));
                }
                expectSymbol(")", "expected ')' after arguments");
                expr.reset(new CallExpr(std::move(expr), std::move(args)));
                continue;
            }

            if (matchSymbol(".")) {
                const Token& member = expectType(TokenType::IDENTIFIER, "expected member name after '.'");
                expr.reset(new MemberExpr(std::move(expr), member.text));
                continue;
            }

            if (matchSymbol("[")) {
                std::unique_ptr<Expr> index = parseExpression();
                expectSymbol("]", "expected ']' after index expression");
                expr.reset(new IndexExpr(std::move(expr), std::move(index)));
                continue;
            }

            break;
        }
        return expr;
    }

    std::unique_ptr<Expr> parsePrimary() {
        if (matchSymbol("(")) {
            std::unique_ptr<Expr> expr = parseExpression();
            expectSymbol(")", "expected ')'");
            return expr;
        }

        if (matchSymbol("[")) {
            --position_;
            return parseArrayExpr();
        }

        if (matchSymbol("{")) {
            --position_;
            return parseDictExpr();
        }

        if (matchType(TokenType::NUMBER)) {
            return std::unique_ptr<Expr>(static_cast<Expr*>(new LiteralExpr(Value(std::stoi(previous().text)))));
        }

        if (matchType(TokenType::STRING)) {
            return std::unique_ptr<Expr>(static_cast<Expr*>(new LiteralExpr(Value(previous().text))));
        }

        if (matchSymbol("$")) {
            const Token& token = expectType(TokenType::IDENTIFIER, "expected variable name after '$'");
            return std::unique_ptr<Expr>(static_cast<Expr*>(new VariableExpr(token.text)));
        }

        if (matchType(TokenType::IDENTIFIER)) {
            const Token& token = previous();
            if (token.text == "pipe") {
                return parsePipeExpr();
            }
            if (token.text == "true") {
                return std::unique_ptr<Expr>(static_cast<Expr*>(new LiteralExpr(Value(true))));
            }
            if (token.text == "false") {
                return std::unique_ptr<Expr>(static_cast<Expr*>(new LiteralExpr(Value(false))));
            }
            if (token.text == "nil") {
                return std::unique_ptr<Expr>(static_cast<Expr*>(new LiteralExpr(Value())));
            }
            if (token.text == "_") {
                return std::unique_ptr<Expr>(static_cast<Expr*>(new PlaceholderExpr()));
            }
            return std::unique_ptr<Expr>(static_cast<Expr*>(new IdentifierExpr(token.text)));
        }

        syntaxError("expected expression");
    }

    std::unique_ptr<Expr> parseArrayExpr() {
        expectSymbol("[", "expected '['");
        std::vector<std::unique_ptr<Expr> > elements;
        if (!checkSymbol("]")) {
            do {
                elements.push_back(parseExpression());
            } while (matchSymbol(","));
        }
        expectSymbol("]", "expected ']'");
        return std::unique_ptr<Expr>(static_cast<Expr*>(new ArrayExpr(std::move(elements))));
    }

    std::unique_ptr<Expr> parseDictExpr() {
        expectSymbol("{", "expected '{'");
        std::vector<std::pair<std::string, std::unique_ptr<Expr> > > entries;
        if (!checkSymbol("}")) {
            do {
                std::string key;
                if (matchType(TokenType::STRING) || matchType(TokenType::IDENTIFIER)) {
                    key = previous().text;
                } else {
                    syntaxError("dictionary key must be a string or identifier");
                }
                expectSymbol(":", "expected ':' after dictionary key");
                entries.push_back(std::make_pair(key, parseExpression()));
            } while (matchSymbol(","));
        }
        expectSymbol("}", "expected '}'");
        return std::unique_ptr<Expr>(static_cast<Expr*>(new DictExpr(std::move(entries))));
    }

    std::unique_ptr<Expr> parsePipeExpr() {
        std::vector<std::string> params = parseParameterNames();
        std::vector<std::unique_ptr<Statement> > body = parseBlock();
        return std::unique_ptr<Expr>(static_cast<Expr*>(new PipeExpr(std::move(params), std::move(body))));
    }

    std::unique_ptr<Expr> wrapIntoStage(std::unique_ptr<Expr> value, const std::string& name) {
        std::unique_ptr<PipelineExpr> pipeline(new PipelineExpr(std::move(value)));
        std::vector<std::unique_ptr<Expr> > args;
        args.push_back(std::unique_ptr<Expr>(static_cast<Expr*>(new IdentifierExpr(name))));
        pipeline->stages.push_back({
            "|>",
            std::unique_ptr<Expr>(
                static_cast<Expr*>(new CallExpr(
                    std::unique_ptr<Expr>(static_cast<Expr*>(new IdentifierExpr("into"))),
                    std::move(args))))
        });
        return std::unique_ptr<Expr>(pipeline.release());
    }

    [[noreturn]] void syntaxError(const std::string& message) const {
        throw std::runtime_error("Syntax Error(line " + std::to_string(peek().line) + "): " + message);
    }

    const std::vector<Token>& tokens_;
    size_t position_;
};

struct ReturnSignal {
    explicit ReturnSignal(const Value& returnValue)
        : value(returnValue) {
    }

    Value value;
};

struct BreakSignal {
};

struct ContinueSignal {
};

#if AETHE_ENABLE_THREADS
class SharedThreadPool {
public:
    explicit SharedThreadPool(size_t threadCount)
        : workers_(), queue_(), mutex_(), ready_(), stopping_(false) {
        const size_t count = std::max<size_t>(1, threadCount);
        workers_.reserve(count);
        for (size_t index = 0; index < count; ++index) {
            workers_.push_back(std::thread([this]() { workerLoop(); }));
        }
    }

    ~SharedThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        ready_.notify_all();
        for (size_t index = 0; index < workers_.size(); ++index) {
            if (workers_[index].joinable()) {
                workers_[index].join();
            }
        }
    }

    template<typename Fn>
    std::future<typename std::result_of<Fn()>::type> submit(Fn fn) {
        typedef typename std::result_of<Fn()>::type ResultType;
        std::shared_ptr<std::packaged_task<ResultType()> > task(new std::packaged_task<ResultType()>(fn));
        std::future<ResultType> future = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("thread pool is stopping");
            }
            queue_.push([task]() {
                (*task)();
            });
        }
        ready_.notify_one();
        return future;
    }

    size_t size() const {
        return workers_.size();
    }

private:
    void workerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ready_.wait(lock, [this]() {
                    return stopping_ || !queue_.empty();
                });
                if (stopping_ && queue_.empty()) {
                    return;
                }
                task = queue_.front();
                queue_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()> > queue_;
    mutable std::mutex mutex_;
    std::condition_variable ready_;
    bool stopping_;
};

SharedThreadPool& sharedThreadPool() {
    const unsigned int hc = std::thread::hardware_concurrency();
    const size_t workerCount = hc == 0 ? 4 : static_cast<size_t>(hc);
    static SharedThreadPool pool(workerCount);
    return pool;
}
#endif

class Interpreter {
public:
    typedef std::function<void(const std::string&)> OutputHandler;
    typedef std::function<bool(const std::string&, std::string*)> InputHandler;

    friend class BytecodeVirtualMachine;

        explicit Interpreter(const OutputHandler& outputHandler = OutputHandler(),
                         const InputHandler& inputHandler = InputHandler())
        : scopes_(1),
          flows_(),
          stages_(),
          compiledFlows_(),
          compiledStages_(),
          types_(),
          callDepth_(0),
          loopDepth_(0),
          outputHandler_(outputHandler ? outputHandler : consoleOutput),
          inputHandler_(inputHandler ? inputHandler : consoleInput) {
    }

        void executeProgram(const std::vector<std::unique_ptr<Statement> >& program) {
        for (size_t index = 0; index < program.size(); ++index) {
            registerTopLevelDefinition(*program[index]);
        }

        for (size_t index = 0; index < program.size(); ++index) {
            if (program[index]->type == Statement::FLOW_DEF ||
                program[index]->type == Statement::STAGE_DEF ||
                program[index]->type == Statement::TYPE_DEF) {
                continue;
            }
            executeStatement(*program[index]);
        }
    }

private:
    struct ScopeFrame {
        ScopeFrame()
            : vars(), defers() {
        }

        std::unordered_map<std::string, Value> vars;
        std::vector<const Statement*> defers;
    };

    struct TypeInfo {
        std::vector<std::string> fields;
        std::unordered_map<std::string, const Statement*> methods;
    };

    void registerTopLevelDefinition(const Statement& statement) {
        if (statement.type == Statement::FLOW_DEF) {
            flows_[statement.name] = &statement;
        } else if (statement.type == Statement::STAGE_DEF) {
            stages_[statement.name] = &statement;
        } else if (statement.type == Statement::TYPE_DEF) {
            TypeInfo info;
            info.fields = statement.params;
            for (size_t index = 0; index < statement.methods.size(); ++index) {
                info.methods[statement.methods[index]->name] = statement.methods[index].get();
            }
            types_[statement.name] = info;
        }
    }

    Value executeStatement(const Statement& statement) {
        switch (statement.type) {
            case Statement::EXPR:
                return evalExpr(statement.expr.get());
            case Statement::FLOW_DEF:
                flows_[statement.name] = &statement;
                return Value();
            case Statement::STAGE_DEF:
                stages_[statement.name] = &statement;
                return Value();
            case Statement::TYPE_DEF: {
                TypeInfo info;
                info.fields = statement.params;
                for (size_t index = 0; index < statement.methods.size(); ++index) {
                    info.methods[statement.methods[index]->name] = statement.methods[index].get();
                }
                types_[statement.name] = info;
                return Value();
            }
            case Statement::WHEN:
                if (evalExpr(statement.expr.get()).isTruthy()) {
                    return executeScopedBlock(statement.body);
                }
                return executeScopedBlock(statement.elseBody);
            case Statement::MATCH:
                return executeMatch(statement);
            case Statement::WHILE_LOOP: {
                Value last;
                while (evalExpr(statement.expr.get()).isTruthy()) {
                    ++loopDepth_;
                    try {
                        last = executeScopedBlock(statement.body);
                    } catch (const ContinueSignal&) {
                    } catch (const BreakSignal&) {
                        --loopDepth_;
                        break;
                    }
                    --loopDepth_;
                }
                return last;
            }
            case Statement::FOR_LOOP:
                return executeForLoop(statement);
            case Statement::RETURN:
                if (callDepth_ == 0) {
                    runtimeError("give can only be used inside flow, stream/stage, or method bodies");
                }
                throw ReturnSignal(statement.expr.get() == nullptr ? Value() : evalExpr(statement.expr.get()));
            case Statement::DEFER:
                currentScope().defers.push_back(&statement);
                return Value();
            case Statement::BREAK_LOOP:
                if (loopDepth_ == 0) {
                    runtimeError("break can only be used inside while or for");
                }
                throw BreakSignal();
            case Statement::CONTINUE_LOOP:
                if (loopDepth_ == 0) {
                    runtimeError("continue can only be used inside while or for");
                }
                throw ContinueSignal();
            default:
                runtimeError("unknown statement");
        }
    }

    Value executeScopedBlock(const std::vector<std::unique_ptr<Statement> >& body) {
        pushScope();
        try {
            Value result = executeStatements(body);
            popScope();
            return result;
        } catch (...) {
            popScope();
            throw;
        }
    }

    Value executeMatch(const Statement& statement) {
        const Value target = evalExpr(statement.expr.get());
        for (size_t index = 0; index < statement.arms.size(); ++index) {
            const Expr* pattern = statement.arms[index].get();
            const bool wildcard = dynamic_cast<const PlaceholderExpr*>(pattern) != nullptr;

            if (const CallExpr* callPattern = dynamic_cast<const CallExpr*>(pattern)) {
                if (const IdentifierExpr* id = dynamic_cast<const IdentifierExpr*>(callPattern->callee.get())) {
                    if ((id->name == "Ok" && target.type == Value::RESULT_OK) ||
                        (id->name == "Err" && target.type == Value::RESULT_ERR)) {
                        if (matchGuardPasses(target, statement.armGuards[index].get())) {
                            Value inner = target.resultValue ? *target.resultValue : Value();
                            if (callPattern->args.size() == 1) {
                                if (const IdentifierExpr* binding = dynamic_cast<const IdentifierExpr*>(callPattern->args[0].get())) {
                                    pushScope();
                                    currentScope().vars["it"] = inner;
                                    currentScope().vars[binding->name] = inner;
                                    try {
                                        Value result = executeStatements(statement.armBodies[index]);
                                        popScope();
                                        return result;
                                    } catch (...) {
                                        popScope();
                                        throw;
                                    }
                                }
                            }
                            return executeMatchBlock(inner, statement.armBodies[index]);
                        }
                        continue;
                    }
                }
            }

            if (!wildcard && !target.equals(evalExpr(pattern))) {
                continue;
            }
            if (matchGuardPasses(target, statement.armGuards[index].get())) {
                return executeMatchBlock(target, statement.armBodies[index]);
            }
        }
        return executeMatchBlock(target, statement.elseBody);
    }

    Value executeStatements(const std::vector<std::unique_ptr<Statement> >& body) {
        Value last;
        for (size_t index = 0; index < body.size(); ++index) {
            last = executeStatement(*body[index]);
        }
        return last;
    }

    Value executeForLoop(const Statement& statement) {
        Value iterable = evalExpr(statement.expr.get());
        Value last;
        const bool hasSecondBinding = !statement.params.empty();

        if (iterable.type == Value::ARRAY) {
            const std::vector<Value>& items = iterable.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                ++loopDepth_;
                try {
                    last = hasSecondBinding
                               ? executeLoopIteration(statement.name,
                                                    Value(static_cast<int>(index)),
                                                    &statement.params.front(),
                                                    &items[index],
                                                    statement.body)
                               : executeLoopIteration(statement.name, items[index], nullptr, nullptr, statement.body);
                } catch (const ContinueSignal&) {
                } catch (const BreakSignal&) {
                    --loopDepth_;
                    break;
                }
                --loopDepth_;
            }
            return last;
        }

        if (iterable.type == Value::STRING) {
            for (size_t index = 0; index < iterable.stringValue.size(); ++index) {
                const Value character(std::string(1, iterable.stringValue[index]));
                ++loopDepth_;
                try {
                    last = hasSecondBinding
                               ? executeLoopIteration(statement.name,
                                                    Value(static_cast<int>(index)),
                                                    &statement.params.front(),
                                                    &character,
                                                    statement.body)
                               : executeLoopIteration(statement.name, character, nullptr, nullptr, statement.body);
                } catch (const ContinueSignal&) {
                } catch (const BreakSignal&) {
                    --loopDepth_;
                    break;
                }
                --loopDepth_;
            }
            return last;
        }

        if (iterable.type == Value::DICT) {
            std::vector<std::string> keys;
            const std::unordered_map<std::string, Value>& entries = iterable.asDict();
            for (std::unordered_map<std::string, Value>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                keys.push_back(it->first);
            }
            std::sort(keys.begin(), keys.end());

            for (size_t index = 0; index < keys.size(); ++index) {
                std::unordered_map<std::string, Value> pairValue;
                pairValue["key"] = Value(keys[index]);
                pairValue["value"] = entries.find(keys[index])->second;
                const Value keyValue(keys[index]);
                const Value currentValue = entries.find(keys[index])->second;
                ++loopDepth_;
                try {
                    last = hasSecondBinding
                               ? executeLoopIteration(statement.name,
                                                    keyValue,
                                                    &statement.params.front(),
                                                    &currentValue,
                                                    statement.body)
                               : executeLoopIteration(statement.name, Value(pairValue), nullptr, nullptr, statement.body);
                } catch (const ContinueSignal&) {
                } catch (const BreakSignal&) {
                    --loopDepth_;
                    break;
                }
                --loopDepth_;
            }
            return last;
        }

        if (iterable.type == Value::OBJECT && iterable.objectValue.get() != nullptr) {
            std::vector<std::string> keys;
            for (std::unordered_map<std::string, Value>::const_iterator it = iterable.objectValue->fields.begin();
                 it != iterable.objectValue->fields.end();
                 ++it) {
                keys.push_back(it->first);
            }
            std::sort(keys.begin(), keys.end());

            for (size_t index = 0; index < keys.size(); ++index) {
                std::unordered_map<std::string, Value> pairValue;
                pairValue["key"] = Value(keys[index]);
                pairValue["value"] = iterable.objectValue->fields.find(keys[index])->second;
                const Value keyValue(keys[index]);
                const Value currentValue = iterable.objectValue->fields.find(keys[index])->second;
                ++loopDepth_;
                try {
                    last = hasSecondBinding
                               ? executeLoopIteration(statement.name,
                                                    keyValue,
                                                    &statement.params.front(),
                                                    &currentValue,
                                                    statement.body)
                               : executeLoopIteration(statement.name, Value(pairValue), nullptr, nullptr, statement.body);
                } catch (const ContinueSignal&) {
                } catch (const BreakSignal&) {
                    --loopDepth_;
                    break;
                }
                --loopDepth_;
            }
            return last;
        }

        if (isStreamValue(iterable)) {
            StreamCursorState cursor = makeStreamCursor(iterable, "for");
            Value item;
            size_t index = 0;
            while (cursor.next(this, &item, "for")) {
                ++loopDepth_;
                try {
                    if (hasSecondBinding) {
                        last = executeLoopIteration(statement.name,
                                                    Value(static_cast<int>(index)),
                                                    &statement.params.front(),
                                                    &item,
                                                    statement.body);
                    } else {
                        last = executeLoopIteration(statement.name, item, nullptr, nullptr, statement.body);
                    }
                } catch (const ContinueSignal&) {
                } catch (const BreakSignal&) {
                    --loopDepth_;
                    break;
                }
                --loopDepth_;
                ++index;
            }
            return last;
        }

        runtimeError("for expects an array, string, dict, object, or stream iterable");
    }

    Value executeLoopIteration(const std::string& name,
                               const Value& value,
                               const std::string* secondName,
                               const Value* secondValue,
                               const std::vector<std::unique_ptr<Statement> >& body) {
        pushScope();
        currentScope().vars[name] = value;
        if (secondName != nullptr && secondValue != nullptr) {
            currentScope().vars[*secondName] = *secondValue;
        }
        try {
            Value result = executeStatements(body);
            popScope();
            return result;
        } catch (...) {
            popScope();
            throw;
        }
    }

    bool matchGuardPasses(const Value& target, const Expr* guard) {
        if (guard == nullptr) {
            return true;
        }
        pushScope();
        currentScope().vars["it"] = target;
        try {
            const bool result = evalExpr(guard).isTruthy();
            popScope();
            return result;
        } catch (...) {
            popScope();
            throw;
        }
    }

    Value executeMatchBlock(const Value& target, const std::vector<std::unique_ptr<Statement> >& body) {
        pushScope();
        currentScope().vars["it"] = target;
        try {
            Value result = executeStatements(body);
            popScope();
            return result;
        } catch (...) {
            popScope();
            throw;
        }
    }

    Value evalExpr(const Expr* expr) {
        return evalExpr(expr, nullptr);
    }

    Value evalExpr(const Expr* expr, const Value* pipeInput) {
        if (const LiteralExpr* literal = dynamic_cast<const LiteralExpr*>(expr)) {
            return literal->value;
        }

        if (const VariableExpr* variable = dynamic_cast<const VariableExpr*>(expr)) {
            return getVariable(variable->name);
        }

        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(expr)) {
            return Value(identifier->name);
        }

        if (dynamic_cast<const PlaceholderExpr*>(expr) != nullptr) {
            if (pipeInput == nullptr) {
                runtimeError("placeholder '_' is a pipeline value slot, not an anonymous function; it can only be used on the right side of a pipeline");
            }
            return *pipeInput;
        }

        if (const ArrayExpr* array = dynamic_cast<const ArrayExpr*>(expr)) {
            std::vector<Value> elements;
            elements.reserve(array->elements.size());
            for (size_t index = 0; index < array->elements.size(); ++index) {
                elements.push_back(evalExpr(array->elements[index].get(), pipeInput));
            }
            return Value(elements);
        }

        if (const DictExpr* dict = dynamic_cast<const DictExpr*>(expr)) {
            std::unordered_map<std::string, Value> entries;
            for (size_t index = 0; index < dict->entries.size(); ++index) {
                entries[dict->entries[index].first] = evalExpr(dict->entries[index].second.get(), pipeInput);
            }
            return Value(entries);
        }

        if (const PipeExpr* pipe = dynamic_cast<const PipeExpr*>(expr)) {
            return Value(std::shared_ptr<CallableData>(new CallableData(pipe->params, &pipe->body, captureVisibleVars())));
        }

        if (const UnaryExpr* unary = dynamic_cast<const UnaryExpr*>(expr)) {
            return evalUnary(*unary, pipeInput);
        }

        if (const BinaryExpr* binary = dynamic_cast<const BinaryExpr*>(expr)) {
            return evalBinary(*binary, pipeInput);
        }

        if (const AssignExpr* assign = dynamic_cast<const AssignExpr*>(expr)) {
            return evalAssign(*assign, pipeInput);
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
            return evalMember(*member, pipeInput);
        }

        if (const IndexExpr* index = dynamic_cast<const IndexExpr*>(expr)) {
            return evalIndex(*index, pipeInput);
        }

        if (const CallExpr* call = dynamic_cast<const CallExpr*>(expr)) {
            return evalCall(*call, pipeInput);
        }

        if (const PipelineExpr* pipeline = dynamic_cast<const PipelineExpr*>(expr)) {
            return evalPipeline(*pipeline);
        }

        runtimeError("unknown expression");
    }

    bool containsPlaceholder(const Expr* expr) const {
        if (dynamic_cast<const PlaceholderExpr*>(expr) != nullptr) {
            return true;
        }
        if (const ArrayExpr* array = dynamic_cast<const ArrayExpr*>(expr)) {
            for (size_t index = 0; index < array->elements.size(); ++index) {
                if (containsPlaceholder(array->elements[index].get())) {
                    return true;
                }
            }
            return false;
        }
        if (const DictExpr* dict = dynamic_cast<const DictExpr*>(expr)) {
            for (size_t index = 0; index < dict->entries.size(); ++index) {
                if (containsPlaceholder(dict->entries[index].second.get())) {
                    return true;
                }
            }
            return false;
        }
        if (const UnaryExpr* unary = dynamic_cast<const UnaryExpr*>(expr)) {
            return containsPlaceholder(unary->right.get());
        }
        if (const BinaryExpr* binary = dynamic_cast<const BinaryExpr*>(expr)) {
            return containsPlaceholder(binary->left.get()) || containsPlaceholder(binary->right.get());
        }
        if (const AssignExpr* assign = dynamic_cast<const AssignExpr*>(expr)) {
            return containsPlaceholder(assign->target.get()) || containsPlaceholder(assign->value.get());
        }
        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
            return containsPlaceholder(member->object.get());
        }
        if (const IndexExpr* index = dynamic_cast<const IndexExpr*>(expr)) {
            return containsPlaceholder(index->container.get()) || containsPlaceholder(index->index.get());
        }
        if (const CallExpr* call = dynamic_cast<const CallExpr*>(expr)) {
            if (containsPlaceholder(call->callee.get())) {
                return true;
            }
            for (size_t index = 0; index < call->args.size(); ++index) {
                if (containsPlaceholder(call->args[index].get())) {
                    return true;
                }
            }
            return false;
        }
        if (const PipelineExpr* pipeline = dynamic_cast<const PipelineExpr*>(expr)) {
            if (containsPlaceholder(pipeline->source.get())) {
                return true;
            }
            for (size_t index = 0; index < pipeline->stages.size(); ++index) {
                if (containsPlaceholder(pipeline->stages[index].expr.get())) {
                    return true;
                }
            }
            return false;
        }
        if (dynamic_cast<const PipeExpr*>(expr) != nullptr) {
            return false;
        }
        return false;
    }

    Value evalUnary(const UnaryExpr& unary, const Value* pipeInput) {
        const Value right = evalExpr(unary.right.get(), pipeInput);
        if (unary.op == "!") {
            return Value(!right.isTruthy());
        }
        if (unary.op == "-") {
            return Value(-requireInt(right, "unary '-'"));
        }
        runtimeError("unknown unary operator '" + unary.op + "'");
    }

    Value evalBinary(const BinaryExpr& binary, const Value* pipeInput) {
        if (binary.op == "&&") {
            const Value left = evalExpr(binary.left.get(), pipeInput);
            if (!left.isTruthy()) {
                return Value(false);
            }
            return Value(evalExpr(binary.right.get(), pipeInput).isTruthy());
        }

        if (binary.op == "||") {
            const Value left = evalExpr(binary.left.get(), pipeInput);
            if (left.isTruthy()) {
                return Value(true);
            }
            return Value(evalExpr(binary.right.get(), pipeInput).isTruthy());
        }

        const Value left = evalExpr(binary.left.get(), pipeInput);
        const Value right = evalExpr(binary.right.get(), pipeInput);
        return applyBinaryOperator(left, binary.op, right);
    }

    Value applyBinaryOperator(const Value& left, const std::string& op, const Value& right) const {
        if (op == "+") {
            if (left.type == Value::STRING || right.type == Value::STRING) {
                return Value(left.toString() + right.toString());
            }
            return Value(requireInt(left, op) + requireInt(right, op));
        }

        if (op == "-") {
            return Value(requireInt(left, op) - requireInt(right, op));
        }

        if (op == "*") {
            return Value(requireInt(left, op) * requireInt(right, op));
        }

        if (op == "/") {
            const int divisor = requireInt(right, op);
            if (divisor == 0) {
                runtimeError("division by zero");
            }
            return Value(requireInt(left, op) / divisor);
        }

        if (op == "%") {
            const int divisor = requireInt(right, op);
            if (divisor == 0) {
                runtimeError("modulo by zero");
            }
            return Value(requireInt(left, op) % divisor);
        }

        if (op == "==") {
            return Value(left.equals(right));
        }

        if (op == "!=") {
            return Value(!left.equals(right));
        }

        if (op == ">") {
            return Value(requireInt(left, op) > requireInt(right, op));
        }

        if (op == ">=") {
            return Value(requireInt(left, op) >= requireInt(right, op));
        }

        if (op == "<") {
            return Value(requireInt(left, op) < requireInt(right, op));
        }

        if (op == "<=") {
            return Value(requireInt(left, op) <= requireInt(right, op));
        }

        runtimeError("unknown binary operator '" + op + "'");
    }

    bool isAssignableTarget(const Expr* expr) const {
        if (dynamic_cast<const VariableExpr*>(expr) != nullptr) {
            return true;
        }
        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
            return isAssignableTarget(member->object.get());
        }
        if (const IndexExpr* index = dynamic_cast<const IndexExpr*>(expr)) {
            return isAssignableTarget(index->container.get());
        }
        return false;
    }

    Value applyAssignmentOperator(const Value& left, const std::string& op, const Value& right) const {
        if (op == "=") {
            return right;
        }
        if (op == "+=") {
            if (left.type == Value::STRING || right.type == Value::STRING) {
                return Value(left.toString() + right.toString());
            }
            return Value(requireInt(left, op) + requireInt(right, op));
        }
        if (op == "-=") {
            return Value(requireInt(left, op) - requireInt(right, op));
        }
        if (op == "*=") {
            return Value(requireInt(left, op) * requireInt(right, op));
        }
        if (op == "/=") {
            const int divisor = requireInt(right, op);
            if (divisor == 0) {
                runtimeError("division by zero");
            }
            return Value(requireInt(left, op) / divisor);
        }
        if (op == "%=") {
            const int divisor = requireInt(right, op);
            if (divisor == 0) {
                runtimeError("modulo by zero");
            }
            return Value(requireInt(left, op) % divisor);
        }
        runtimeError("unknown assignment operator '" + op + "'");
    }

    void assignTarget(const Expr* target, const Value& value, const Value* pipeInput) {
        if (const VariableExpr* variable = dynamic_cast<const VariableExpr*>(target)) {
            storeVariable(variable->name, value);
            return;
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(target)) {
            const Value object = evalExpr(member->object.get(), pipeInput);
            const Value updated = writeIndexedValue(object, Value(member->memberName), value, "assignment");
            assignTarget(member->object.get(), updated, pipeInput);
            return;
        }

        if (const IndexExpr* index = dynamic_cast<const IndexExpr*>(target)) {
            const Value container = evalExpr(index->container.get(), pipeInput);
            const Value key = evalExpr(index->index.get(), pipeInput);
            const Value updated = writeIndexedValue(container, key, value, "assignment");
            assignTarget(index->container.get(), updated, pipeInput);
            return;
        }

        runtimeError("assignment target must be a variable, member access, or index access");
    }

    Value evalAssign(const AssignExpr& assign, const Value* pipeInput) {
        if (!isAssignableTarget(assign.target.get())) {
            runtimeError("assignment target must be a variable, member access, or index access");
        }

        const Value right = evalExpr(assign.value.get(), pipeInput);
        const Value assigned = assign.op == "="
                                   ? right
                                   : applyAssignmentOperator(evalExpr(assign.target.get(), pipeInput), assign.op, right);
        assignTarget(assign.target.get(), assigned, pipeInput);
        return assigned;
    }

    Value evalMember(const MemberExpr& member, const Value* pipeInput) {
        const Value object = evalExpr(member.object.get(), pipeInput);
        return readMember(object, member.memberName);
    }

    Value evalIndex(const IndexExpr& index, const Value* pipeInput) {
        const Value container = evalExpr(index.container.get(), pipeInput);
        const Value key = evalExpr(index.index.get(), pipeInput);
        return readIndexedValue(container, key, "index access");
    }

    Value evalCall(const CallExpr& call, const Value* pipeInput) {
        std::vector<Value> args = evalArgs(call.args, pipeInput);

        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(call.callee.get())) {
            return invokeIdentifierCall(identifier->name, args);
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(call.callee.get())) {
            Value object = evalExpr(member->object.get(), pipeInput);
            if (object.type == Value::OBJECT && hasMethod(object.objectValue->typeName, member->memberName)) {
                return invokeMethod(object, member->memberName, args);
            }
            return invokeCallableValue(readMember(object, member->memberName), args, "call");
        }

        return invokeCallableValue(evalExpr(call.callee.get(), pipeInput), args, "call");
    }

    Value evalPipeline(const PipelineExpr& pipeline) {
        Value value = evalExpr(pipeline.source.get());
        for (size_t index = 0; index < pipeline.stages.size(); ++index) {
            const PipelineExpr::Stage& stage = pipeline.stages[index];
            if (stage.op == "|?") {
                if (value.type == Value::RESULT_ERR) {
                    continue;
                }
                if (value.type == Value::RESULT_OK) {
                    value = *(value.resultValue);
                }
                try {
                    value = evalPipeTarget(stage.expr.get(), value);
                    if (value.type != Value::RESULT_ERR && value.type != Value::RESULT_OK) {
                        value = Value::Ok(value);
                    }
                } catch (const std::runtime_error& e) {
                    value = Value::Err(Value(std::string(e.what())));
                }
            } else {
                value = evalPipeTarget(stage.expr.get(), value);
            }
        }
        return value;
    }

    std::vector<Value> evalArgs(const std::vector<std::unique_ptr<Expr> >& args, const Value* pipeInput = nullptr) {
        std::vector<Value> values;
        values.reserve(args.size());
        for (size_t index = 0; index < args.size(); ++index) {
            values.push_back(evalExpr(args[index].get(), pipeInput));
        }
        return values;
    }

    Value invokeIdentifierCall(const std::string& name, const std::vector<Value>& args) {
        if (name == "read_file") {
            runtimeError("read_file is disabled in sandbox mode");
        }

        if (name == "Ok") {
            if (args.size() != 1) runtimeError("Ok expects exactly one argument");
            return Value::Ok(args[0]);
        }
        if (name == "Err") {
            if (args.size() != 1) runtimeError("Err expects exactly one argument");
            return Value::Err(args[0]);
        }
        if (name == "is_ok") {
            if (args.size() != 1) runtimeError("is_ok expects exactly one argument");
            return Value(args[0].type == Value::RESULT_OK);
        }
        if (name == "is_err") {
            if (args.size() != 1) runtimeError("is_err expects exactly one argument");
            return Value(args[0].type == Value::RESULT_ERR);
        }
        if (name == "unwrap") {
            if (args.size() != 1) runtimeError("unwrap expects exactly one argument");
            if (args[0].type == Value::RESULT_OK || args[0].type == Value::RESULT_ERR) {
                return *(args[0].resultValue);
            }
            return args[0]; // If not wrapped, just return itself
        }

        if (name == "range") {
            if (args.size() == 1) {
                return makeRange(0, requireInt(args[0], "range"));
            }
            if (args.size() == 2) {
                if (args[1].type == Value::NIL) {
                    return makeRange(requireInt(args[0], "range"), 0, false, 1);
                }
                return makeRange(requireInt(args[0], "range"), requireInt(args[1], "range"));
            }
            if (args.size() == 3) {
                if (args[1].type != Value::NIL) {
                    runtimeError("range(start, nil, step) requires nil as second argument");
                }
                return makeRange(requireInt(args[0], "range"), 0, false, requireInt(args[2], "range"));
            }
            runtimeError("range expects (end), (start, end), (start, nil), or (start, nil, step)");
        }

        if (name == "str") {
            if (args.size() != 1) {
                runtimeError("str expects one argument");
            }
            return Value(args[0].toString());
        }

        if (name == "int") {
            if (args.size() != 1) {
                runtimeError("int expects one argument");
            }
            return Value(toInt(args[0]));
        }

        if (name == "bool") {
            if (args.size() != 1) {
                runtimeError("bool expects one argument");
            }
            return Value(args[0].isTruthy());
        }

        if (name == "type_of") {
            if (args.size() != 1) {
                runtimeError("type_of expects one argument");
            }
            return Value(args[0].typeName());
        }

        if (name == "input") {
            if (args.size() > 1) {
                runtimeError("input expects zero or one argument");
            }
            const std::string prompt = args.empty() ? std::string() : requireString(args[0], "input");
            std::string line;
            if (!inputHandler_(prompt, &line)) {
                return Value();
            }
            return Value(line);
        }

        if (name == "bind") {
            if (args.empty()) {
                runtimeError("bind expects a callable and optional bound arguments");
            }
            expectCallableValue(args[0], "bind");
            return makeNativeCallable(CallableData::BOUND, args, "bind");
        }

        if (name == "chain") {
            if (args.empty()) {
                runtimeError("chain expects at least one callable");
            }
            for (size_t index = 0; index < args.size(); ++index) {
                expectCallableValue(args[index], "chain");
            }
            return makeNativeCallable(CallableData::CHAIN, args, "chain");
        }

        if (name == "branch") {
            if (args.empty()) {
                runtimeError("branch expects at least one callable");
            }
            for (size_t index = 0; index < args.size(); ++index) {
                expectCallableValue(args[index], "branch");
            }
            return makeNativeCallable(CallableData::BRANCH, args, "branch");
        }

        if (name == "guard") {
            if (args.size() < 2 || args.size() > 3) {
                runtimeError("guard expects predicate, true route, and optional false route");
            }
            for (size_t index = 0; index < args.size(); ++index) {
                expectCallableValue(args[index], "guard");
            }
            return makeNativeCallable(CallableData::GUARD, args, "guard");
        }

        std::unordered_map<std::string, std::shared_ptr<CompiledFunction> >::const_iterator compiledFlowIt =
            compiledFlows_.find(name);
        if (compiledFlowIt != compiledFlows_.end() && compiledFlowIt->second.get() != nullptr) {
            return BytecodeVirtualMachine::runFlow(this, *compiledFlowIt->second, args, nullptr);
        }

        if (!args.empty()) {
            std::vector<Value> stageArgs(args.begin() + 1, args.end());
            return runNamedStage(name, args.front(), stageArgs);
        }

        runtimeError("unknown callable '" + name + "'");
    }

    Value evalPipeTarget(const Expr* target, const Value& input) {
        if (containsPlaceholder(target)) {
            return evalExpr(target, &input);
        }

        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(target)) {
            return invokePipeCallable(Value(identifier->name), input, std::vector<Value>(), "pipeline target");
        }

        if (const CallExpr* call = dynamic_cast<const CallExpr*>(target)) {
            const std::vector<Value> args = evalArgs(call->args);
            if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(call->callee.get())) {
                return invokePipeCallable(Value(identifier->name), input, args, "pipeline target");
            }
            return invokePipeCallable(evalExpr(call->callee.get()), input, args, "pipeline target");
        }

        return invokePipeCallable(evalExpr(target), input, std::vector<Value>(), "pipeline target");
    }

    Value runNamedStage(const std::string& name, const Value& input, const std::vector<Value>& args) {

        if (name == "into" || name == "store") {
            if (args.size() != 1) {
                runtimeError("into(name) expects exactly one argument");
            }
            storeVariable(requireSymbol(args[0], name), input);
            return input;
        }

        if (name == "emit" || name == "print" || name == "show") {
            if (!args.empty()) {
                runtimeError("emit expects no arguments");
            }
            if (isStreamValue(input)) {
                outputHandler_(Value(materializeStream(input, name)).toString() + "\n");
            } else {
                outputHandler_(input.toString() + "\n");
            }
            return input;
        }

        if (name == "drop") {
            if (!args.empty()) {
                runtimeError("drop expects no arguments");
            }
            return Value();
        }

        if (name == "give") {
            if (!args.empty()) {
                runtimeError("give stage expects no arguments");
            }
            if (callDepth_ == 0) {
                runtimeError("give stage can only be used inside flow, stream/stage, or method bodies");
            }
            throw ReturnSignal(input);
        }

        if (name == "add") {
            expectArity(args, 1, name);
            return Value(requireInt(input, name) + requireInt(args[0], name));
        }

        if (name == "sub") {
            expectArity(args, 1, name);
            return Value(requireInt(input, name) - requireInt(args[0], name));
        }

        if (name == "mul") {
            expectArity(args, 1, name);
            return Value(requireInt(input, name) * requireInt(args[0], name));
        }

        if (name == "div") {
            expectArity(args, 1, name);
            const int divisor = requireInt(args[0], name);
            if (divisor == 0) {
                runtimeError("div does not allow zero");
            }
            return Value(requireInt(input, name) / divisor);
        }

        if (name == "mod") {
            expectArity(args, 1, name);
            const int divisor = requireInt(args[0], name);
            if (divisor == 0) {
                runtimeError("mod does not allow zero");
            }
            return Value(requireInt(input, name) % divisor);
        }

        if (name == "min") {
            expectArity(args, 1, name);
            return Value(std::min(requireInt(input, name), requireInt(args[0], name)));
        }

        if (name == "max") {
            expectArity(args, 1, name);
            return Value(std::max(requireInt(input, name), requireInt(args[0], name)));
        }

        if (name == "eq") {
            expectArity(args, 1, name);
            return Value(input.equals(args[0]));
        }

        if (name == "ne") {
            expectArity(args, 1, name);
            return Value(!input.equals(args[0]));
        }

        if (name == "gt") {
            expectArity(args, 1, name);
            return Value(requireInt(input, name) > requireInt(args[0], name));
        }

        if (name == "gte") {
            expectArity(args, 1, name);
            return Value(requireInt(input, name) >= requireInt(args[0], name));
        }

        if (name == "lt") {
            expectArity(args, 1, name);
            return Value(requireInt(input, name) < requireInt(args[0], name));
        }

        if (name == "lte") {
            expectArity(args, 1, name);
            return Value(requireInt(input, name) <= requireInt(args[0], name));
        }

        if (name == "not") {
            expectArity(args, 0, name);
            return Value(!input.isTruthy());
        }

        if (name == "default") {
            expectArity(args, 1, name);
            return input.type == Value::NIL ? args[0] : input;
        }

        if (name == "choose") {
            expectArity(args, 2, name);
            return input.isTruthy() ? args[0] : args[1];
        }

        if (name == "trim") {
            expectArity(args, 0, name);
            return Value(trimCopy(requireString(input, name)));
        }

        if (name == "upper" || name == "to_upper") {
            expectArity(args, 0, name);
            return Value(toUpperCopy(requireString(input, name)));
        }

        if (name == "lower" || name == "to_lower") {
            expectArity(args, 0, name);
            return Value(toLowerCopy(requireString(input, name)));
        }

        if (name == "substring") {
            if (args.size() != 2) {
                runtimeError("substring expects start and length");
            }
            const std::string text = requireString(input, name);
            const int start = requireInt(args[0], name);
            const int length = requireInt(args[1], name);
            if (start < 0 || length < 0 || static_cast<size_t>(start) >= text.size()) {
                return Value("");
            }
            return Value(text.substr(static_cast<size_t>(start), static_cast<size_t>(length)));
        }

        if (name == "concat") {
            std::string output = input.toString();
            for (size_t index = 0; index < args.size(); ++index) {
                output += args[index].toString();
            }
            return Value(output);
        }

        if (name == "split") {
            expectArity(args, 1, name);
            return Value(splitString(requireString(input, name), requireString(args[0], name)));
        }

        if (name == "contains" || name == "has") {
            expectArity(args, 1, name);
            return Value(containsValue(materializeStreamValue(input, name), args[0], name));
        }

        if (name == "starts_with") {
            expectArity(args, 1, name);
            return Value(stringsStartsWith(requireString(input, name), requireString(args[0], name)));
        }

        if (name == "ends_with") {
            expectArity(args, 1, name);
            return Value(stringsEndsWith(requireString(input, name), requireString(args[0], name)));
        }

        if (name == "replace") {
            expectArity(args, 2, name);
            const std::string from = requireString(args[0], name);
            if (from.empty()) {
                runtimeError("replace does not allow an empty search string");
            }
            return Value(replaceAll(requireString(input, name), from, requireString(args[1], name)));
        }

        if (name == "join") {
            expectArity(args, 1, name);
            const Value base = materializeStreamValue(input, name);
            if (base.type != Value::ARRAY) {
                runtimeError("join expects an array input");
            }
            return Value(joinArray(base.asArray(), requireString(args[0], name)));
        }

        if (name == "slice") {
            expectArity(args, 2, name);
            return sliceValue(materializeStreamValue(input, name), requireInt(args[0], name), requireInt(args[1], name), name);
        }

        if (name == "reverse") {
            expectArity(args, 0, name);
            return reverseValue(materializeStreamValue(input, name), name);
        }

        if (name == "index_of") {
            expectArity(args, 1, name);
            return Value(indexOfValue(materializeStreamValue(input, name), args[0], name));
        }

        if (name == "repeat") {
            expectArity(args, 1, name);
            return repeatValue(materializeStreamValue(input, name), requireInt(args[0], name), name);
        }

        if (name == "sum") {
            expectArity(args, 0, name);
            if (isStreamValue(input)) {
                int total = 0;
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    total += requireInt(item, name);
                }
                return Value(total);
            }
            return Value(sumValue(input, name));
        }

        if (name == "sum_by") {
            expectArity(args, 1, name);
            return Value(sumByField(materializeStreamValue(input, name), args[0], name));
        }

        if (name == "flatten") {
            expectArity(args, 0, name);
            return flattenValue(materializeStreamValue(input, name), name);
        }

        if (name == "take") {
            expectArity(args, 1, name);
            if (isStreamValue(input)) {
                return Value(takeFromStream(input, requireInt(args[0], name), name));
            }
            return takeValue(input, requireInt(args[0], name), name);
        }

        if (name == "skip") {
            expectArity(args, 1, name);
            if (isStreamValue(input)) {
                const int count = requireInt(args[0], name);
                if (count < 0) {
                    runtimeError("stage '" + name + "' does not allow negative counts");
                }
                StreamData::Operation operation(StreamData::SKIP);
                operation.count = count;
                return appendStreamOperation(input, operation, name);
            }
            return skipValue(input, requireInt(args[0], name), name);
        }

        if (name == "distinct") {
            expectArity(args, 0, name);
            return distinctValue(materializeStreamValue(input, name), name);
        }

        if (name == "distinct_by") {
            expectArity(args, 1, name);
            return distinctByField(materializeStreamValue(input, name), args[0], name);
        }

        if (name == "sort") {
            expectArity(args, 0, name);
            return sortValue(materializeStreamValue(input, name), false, name);
        }

        if (name == "sort_desc") {
            expectArity(args, 0, name);
            return sortValue(materializeStreamValue(input, name), true, name);
        }

        if (name == "sort_by") {
            expectArity(args, 1, name);
            return sortByField(materializeStreamValue(input, name), args[0], false, name);
        }

        if (name == "sort_desc_by") {
            expectArity(args, 1, name);
            return sortByField(materializeStreamValue(input, name), args[0], true, name);
        }

        if (name == "chunk") {
            expectArity(args, 1, name);
            return chunkValue(materializeStreamValue(input, name), requireInt(args[0], name), name);
        }

        if (name == "bind") {
            if (args.empty()) {
                runtimeError("bind expects a callable and optional bound arguments");
            }
            expectCallableValue(args[0], name);
            std::vector<Value> extra(args.begin() + 1, args.end());
            return bindCallable(input, args[0], extra, name);
        }

        if (name == "chain") {
            if (args.empty()) {
                runtimeError("chain expects at least one callable");
            }
            for (size_t index = 0; index < args.size(); ++index) {
                expectCallableValue(args[index], name);
            }
            return chainCallables(input, args, name);
        }

        if (name == "branch") {
            if (args.empty()) {
                runtimeError("branch expects at least one callable");
            }
            for (size_t index = 0; index < args.size(); ++index) {
                expectCallableValue(args[index], name);
            }
            return branchCallables(input, args, name);
        }

        if (name == "guard") {
            if (args.size() < 2 || args.size() > 3) {
                runtimeError("guard expects predicate, true route, and optional false route");
            }
            expectCallableValue(args[0], name);
            expectCallableValue(args[1], name);
            const Value* onFalse = nullptr;
            if (args.size() == 3) {
                expectCallableValue(args[2], name);
                onFalse = &args[2];
            }
            return guardCallable(input, args[0], args[1], onFalse, name);
        }

        if (name == "zip") {
            expectArity(args, 1, name);
            return zipValue(materializeStreamValue(input, name), materializeStreamValue(args[0], name), name);
        }

        if (name == "tap") {
            if (args.empty()) {
                runtimeError("tap expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                expectCallableValue(args[0], name);
                StreamData::Operation operation(StreamData::TAP);
                operation.callable = args[0];
                operation.extraArgs = extra;
                return appendStreamOperation(input, operation, name);
            }
            return tapValue(input, args[0], extra, name);
        }

        if (name == "map") {
            if (args.empty()) {
                runtimeError("map expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                expectCallableValue(args[0], name);
                StreamData::Operation operation(StreamData::MAP);
                operation.callable = args[0];
                operation.extraArgs = extra;
                return appendStreamOperation(input, operation, name);
            }
            if (input.type != Value::ARRAY) {
                runtimeError("map expects an array input");
            }
            std::vector<Value> result;
            const std::vector<Value>& items = input.asArray();
            result.reserve(items.size());
            for (size_t index = 0; index < items.size(); ++index) {
                result.push_back(invokePipeCallable(args[0], items[index], extra, name));
            }
            return Value(result);
        }

        if (name == "pmap") {
            if (args.empty()) {
                runtimeError("pmap expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            return parallelMapValue(input, args[0], extra, name);
        }

        if (name == "flat_map") {
            if (args.empty()) {
                runtimeError("flat_map expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                return flatMapValue(Value(materializeStream(input, name)), args[0], extra, name);
            }
            return flatMapValue(input, args[0], extra, name);
        }

        if (name == "filter") {
            if (args.empty()) {
                runtimeError("filter expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                expectCallableValue(args[0], name);
                StreamData::Operation operation(StreamData::FILTER);
                operation.callable = args[0];
                operation.extraArgs = extra;
                return appendStreamOperation(input, operation, name);
            }
            if (input.type != Value::ARRAY) {
                runtimeError("filter expects an array input");
            }
            std::vector<Value> result;
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                if (invokePipeCallable(args[0], items[index], extra, name).isTruthy()) {
                    result.push_back(items[index]);
                }
            }
            return Value(result);
        }

        if (name == "find") {
            if (args.empty()) {
                runtimeError("find expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    if (invokePipeCallable(args[0], item, extra, name).isTruthy()) {
                        return item;
                    }
                }
                return Value();
            }
            if (input.type != Value::ARRAY) {
                runtimeError("find expects an array input");
            }
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                if (invokePipeCallable(args[0], items[index], extra, name).isTruthy()) {
                    return items[index];
                }
            }
            return Value();
        }

        if (name == "each") {
            if (args.empty()) {
                runtimeError("each expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    invokePipeCallable(args[0], item, extra, name);
                }
                return input;
            }
            if (input.type != Value::ARRAY) {
                runtimeError("each expects an array input");
            }
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                invokePipeCallable(args[0], items[index], extra, name);
            }
            return input;
        }

        if (name == "all") {
            if (args.empty()) {
                runtimeError("all expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    if (!invokePipeCallable(args[0], item, extra, name).isTruthy()) {
                        return Value(false);
                    }
                }
                return Value(true);
            }
            if (input.type != Value::ARRAY) {
                runtimeError("all expects an array input");
            }
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                if (!invokePipeCallable(args[0], items[index], extra, name).isTruthy()) {
                    return Value(false);
                }
            }
            return Value(true);
        }

        if (name == "group_by") {
            if (args.empty()) {
                runtimeError("group_by expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            return groupByValue(materializeStreamValue(input, name), args[0], extra, name);
        }

        if (name == "index_by") {
            expectArity(args, 1, name);
            return indexByField(materializeStreamValue(input, name), args[0], name);
        }

        if (name == "count_by") {
            expectArity(args, 1, name);
            return countByField(materializeStreamValue(input, name), args[0], name);
        }

        if (name == "pluck") {
            expectArity(args, 1, name);
            return pluckValues(materializeStreamValue(input, name), args[0], name);
        }

        if (name == "where") {
            expectArity(args, 2, name);
            return whereEntries(materializeStreamValue(input, name), args[0], args[1], name);
        }

        if (name == "any") {
            if (args.empty()) {
                runtimeError("any expects a callable");
            }
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (isStreamValue(input)) {
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    if (invokePipeCallable(args[0], item, extra, name).isTruthy()) {
                        return Value(true);
                    }
                }
                return Value(false);
            }
            if (input.type != Value::ARRAY) {
                runtimeError("any expects an array input");
            }
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                if (invokePipeCallable(args[0], items[index], extra, name).isTruthy()) {
                    return Value(true);
                }
            }
            return Value(false);
        }

        if (name == "reduce") {
            if (args.size() < 2) {
                runtimeError("reduce expects callable and initial value");
            }
            if (isStreamValue(input)) {
                Value acc = args[1];
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    acc = invokePipeCallable(args[0], acc, std::vector<Value>(1, item), name);
                }
                return acc;
            }
            if (input.type != Value::ARRAY) {
                runtimeError("reduce expects an array input");
            }
            Value acc = args[1];
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                acc = invokePipeCallable(args[0], acc, std::vector<Value>(1, items[index]), name);
            }
            return acc;
        }

        if (name == "scan") {
            if (args.size() < 2) {
                runtimeError("scan expects callable and initial value");
            }
            if (isStreamValue(input)) {
                Value acc = args[1];
                std::vector<Value> history;
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    acc = invokePipeCallable(args[0], acc, std::vector<Value>(1, item), name);
                    history.push_back(acc);
                }
                return Value(history);
            }
            if (input.type != Value::ARRAY) {
                runtimeError("scan expects an array input");
            }
            Value acc = args[1];
            std::vector<Value> history;
            const std::vector<Value>& items = input.asArray();
            history.reserve(items.size());
            for (size_t index = 0; index < items.size(); ++index) {
                acc = invokePipeCallable(args[0], acc, std::vector<Value>(1, items[index]), name);
                history.push_back(acc);
            }
            return Value(history);
        }

        if (name == "size" || name == "count") {
            expectArity(args, 0, name);
            if (isStreamValue(input)) {
                int total = 0;
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                while (cursor.next(this, &item, name)) {
                    ++total;
                }
                return Value(total);
            }
            return Value(containerSize(input, name));
        }

        if (name == "append" || name == "push") {
            expectArity(args, 1, name);
            const Value base = materializeStreamValue(input, name);
            if (base.type != Value::ARRAY) {
                runtimeError("append expects an array input");
            }
            std::vector<Value> result = base.asArray();
            result.push_back(args[0]);
            return Value(result);
        }

        if (name == "prepend") {
            expectArity(args, 1, name);
            const Value base = materializeStreamValue(input, name);
            if (base.type != Value::ARRAY) {
                runtimeError("prepend expects an array input");
            }
            std::vector<Value> result;
            const std::vector<Value>& items = base.asArray();
            result.reserve(items.size() + 1);
            result.push_back(args[0]);
            result.insert(result.end(), items.begin(), items.end());
            return Value(result);
        }

        if (name == "get" || name == "field" || name == "at") {
            expectArity(args, 1, name);
            return readIndexedValue(materializeStreamValue(input, name), args[0], name);
        }

        if (name == "set") {
            expectArity(args, 2, name);
            return writeIndexedValue(materializeStreamValue(input, name), args[0], args[1], name);
        }

        if (name == "update") {
            if (args.size() < 2) {
                runtimeError("update expects key/index and callable");
            }
            std::vector<Value> extra(args.begin() + 2, args.end());
            return updateIndexedValue(materializeStreamValue(input, name), args[0], args[1], extra, name);
        }

        if (name == "insert") {
            expectArity(args, 2, name);
            return insertIndexedValue(materializeStreamValue(input, name), requireInt(args[0], name), args[1], name);
        }

        if (name == "remove") {
            expectArity(args, 1, name);
            return removeIndexedValue(materializeStreamValue(input, name), args[0], name);
        }

        if (name == "keys") {
            expectArity(args, 0, name);
            return readKeys(materializeStreamValue(input, name), name);
        }

        if (name == "values") {
            expectArity(args, 0, name);
            return readValues(materializeStreamValue(input, name), name);
        }

        if (name == "entries") {
            expectArity(args, 0, name);
            return readEntries(materializeStreamValue(input, name), name);
        }

        if (name == "pick") {
            if (args.empty()) {
                runtimeError("pick expects at least one key");
            }
            return pickEntries(materializeStreamValue(input, name), args, name);
        }

        if (name == "omit") {
            if (args.empty()) {
                runtimeError("omit expects at least one key");
            }
            return omitEntries(materializeStreamValue(input, name), args, name);
        }

        if (name == "merge") {
            expectArity(args, 1, name);
            return mergeEntries(materializeStreamValue(input, name), materializeStreamValue(args[0], name), name);
        }

        if (name == "rename") {
            expectArity(args, 2, name);
            return renameEntry(materializeStreamValue(input, name), args[0], args[1], name);
        }

        if (name == "evolve") {
            if (args.size() < 2) {
                runtimeError("evolve expects field name and callable");
            }
            std::vector<Value> extra(args.begin() + 2, args.end());
            return evolveField(materializeStreamValue(input, name), args[0], args[1], extra, name);
        }

        if (name == "derive") {
            if (args.size() < 2) {
                runtimeError("derive expects field name and callable");
            }
            std::vector<Value> extra(args.begin() + 2, args.end());
            return deriveField(materializeStreamValue(input, name), args[0], args[1], extra, name);
        }

        if (name == "head") {
            expectArity(args, 0, name);
            if (isStreamValue(input)) {
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                return cursor.next(this, &item, name) ? item : Value();
            }
            return readBoundary(input, true, name);
        }

        if (name == "last") {
            expectArity(args, 0, name);
            if (isStreamValue(input)) {
                StreamCursorState cursor = makeStreamCursor(input, name);
                Value item;
                Value last;
                bool hasAny = false;
                while (cursor.next(this, &item, name)) {
                    last = item;
                    hasAny = true;
                }
                return hasAny ? last : Value();
            }
            return readBoundary(input, false, name);
        }

        if (name == "window") {
            expectArity(args, 1, name);
            return windowValue(materializeStreamValue(input, name), requireInt(args[0], name), name);
        }

        std::unordered_map<std::string, std::shared_ptr<CompiledFunction> >::const_iterator compiledStageIt =
            compiledStages_.find(name);
        if (compiledStageIt != compiledStages_.end() && compiledStageIt->second.get() != nullptr) {
            return BytecodeVirtualMachine::runStage(this, *compiledStageIt->second, input, args);
        }

        std::unordered_map<std::string, std::shared_ptr<CompiledFunction> >::const_iterator compiledFlowIt =
            compiledFlows_.find(name);
        if (compiledFlowIt != compiledFlows_.end() && compiledFlowIt->second.get() != nullptr) {
            std::vector<Value> flowArgs;
            flowArgs.reserve(args.size() + 1);
            flowArgs.push_back(input);
            flowArgs.insert(flowArgs.end(), args.begin(), args.end());
            return BytecodeVirtualMachine::runFlow(this, *compiledFlowIt->second, flowArgs, nullptr);
        }

        if (isDirectBuiltinCallableName(name)) {
            std::vector<Value> callArgs;
            callArgs.reserve(args.size() + 1);
            callArgs.push_back(input);
            callArgs.insert(callArgs.end(), args.begin(), args.end());
            return invokeIdentifierCall(name, callArgs);
        }

        runtimeError("unknown stage '" + name + "'");
    }

    Value invokeCallableValue(const Value& callable, const std::vector<Value>& args, const std::string& context) {
        if (callable.type == Value::STRING) {
            return invokeIdentifierCall(callable.stringValue, args);
        }

        if (callable.type == Value::CALLABLE && callable.callableValue.get() != nullptr) {
            return invokeAnonymousCallable(*callable.callableValue, args);
        }

        runtimeError(context + " target must be a callable name or pipe value");
    }

    Value invokePipeCallable(const Value& callable,
                             const Value& input,
                             const std::vector<Value>& args,
                             const std::string& context) {
        if (callable.type == Value::STRING) {
            return runNamedStage(callable.stringValue, input, args);
        }

        if (callable.type == Value::CALLABLE && callable.callableValue.get() != nullptr) {
            std::vector<Value> callArgs;
            callArgs.reserve(args.size() + 1);
            callArgs.push_back(input);
            callArgs.insert(callArgs.end(), args.begin(), args.end());
            return invokeAnonymousCallable(*callable.callableValue, callArgs);
        }

        runtimeError(context + " expects a callable name or pipe value");
    }

    Value invokeFlow(const Statement& flow, const std::vector<Value>& args, const Value* self) {
        if (args.size() != flow.params.size()) {
            runtimeError("flow '" + flow.name + "' expected " + std::to_string(flow.params.size()) +
                         " arguments but received " + std::to_string(args.size()));
        }

        pushScope();
        ++callDepth_;
        if (self != nullptr) {
            currentScope().vars["self"] = *self;
        }
        for (size_t index = 0; index < flow.params.size(); ++index) {
            currentScope().vars[flow.params[index]] = args[index];
        }

        try {
            executeStatements(flow.body);
            --callDepth_;
            popScope();
            return Value();
        } catch (const ReturnSignal& signal) {
            --callDepth_;
            popScope();
            return signal.value;
        } catch (...) {
            --callDepth_;
            popScope();
            throw;
        }
    }

    Value invokeAnonymousCallable(const CallableData& callable, const std::vector<Value>& args) {
        if (callable.kind != CallableData::PIPE_BODY) {
            const std::string label = callable.label.empty() ? "pipe" : callable.label;
            if (args.size() != 1) {
                runtimeError(label + " pipe expected 1 argument but received " + std::to_string(args.size()));
            }
            return runNativeCallable(callable, args[0]);
        }

        if (callable.compiledBody.get() != nullptr) {
            if (args.size() != callable.params.size()) {
                runtimeError("pipe expected " + std::to_string(callable.params.size()) +
                             " arguments but received " + std::to_string(args.size()));
            }
            return BytecodeVirtualMachine::runPipe(this, *callable.compiledBody, callable.captured, args);
        }

        runtimeError("pipe value has no compiled body");
    }

    Value runNativeCallable(const CallableData& callable, const Value& input) {
        switch (callable.kind) {
            case CallableData::BOUND: {
                if (callable.parts.empty()) {
                    runtimeError("bind pipe has no target");
                }
                std::vector<Value> extra;
                if (callable.parts.size() > 1) {
                    extra.assign(callable.parts.begin() + 1, callable.parts.end());
                }
                return bindCallable(input, callable.parts[0], extra, callable.label);
            }
            case CallableData::CHAIN:
                return chainCallables(input, callable.parts, callable.label);
            case CallableData::BRANCH:
                return branchCallables(input, callable.parts, callable.label);
            case CallableData::GUARD: {
                if (callable.parts.size() < 2 || callable.parts.size() > 3) {
                    runtimeError("guard pipe has invalid shape");
                }
                const Value* onFalse = callable.parts.size() == 3 ? &callable.parts[2] : nullptr;
                return guardCallable(input, callable.parts[0], callable.parts[1], onFalse, callable.label);
            }
            case CallableData::PIPE_BODY:
                break;
            default:
                runtimeError("unknown native pipe kind");
        }

        runtimeError("unknown native pipe kind");
    }

    Value invokeStage(const Statement& stage, const Value& input, const std::vector<Value>& args) {
        if (args.size() != stage.params.size()) {
            runtimeError("stage '" + stage.name + "' expected " + std::to_string(stage.params.size()) +
                         " arguments but received " + std::to_string(args.size()));
        }

        pushScope();
        ++callDepth_;
        currentScope().vars["it"] = input;
        for (size_t index = 0; index < stage.params.size(); ++index) {
            currentScope().vars[stage.params[index]] = args[index];
        }

        try {
            executeStatements(stage.body);
            --callDepth_;
            popScope();
            return input;
        } catch (const ReturnSignal& signal) {
            --callDepth_;
            popScope();
            return signal.value;
        } catch (...) {
            --callDepth_;
            popScope();
            throw;
        }
    }

    Value invokeMethod(const Value& object, const std::string& name, const std::vector<Value>& args) {
        if (object.type != Value::OBJECT || object.objectValue.get() == nullptr) {
            runtimeError("method call requires an object");
        }

        std::unordered_map<std::string, TypeInfo>::const_iterator typeIt = types_.find(object.objectValue->typeName);
        if (typeIt == types_.end()) {
            runtimeError("unknown object type '" + object.objectValue->typeName + "'");
        }

        std::unordered_map<std::string, const Statement*>::const_iterator methodIt = typeIt->second.methods.find(name);
        if (methodIt == typeIt->second.methods.end()) {
            runtimeError("type '" + object.objectValue->typeName + "' has no method '" + name + "'");
        }

        return invokeFlow(*methodIt->second, args, &object);
    }

    Value constructObject(const std::string& typeName, const TypeInfo& typeInfo, const std::vector<Value>& args) {
        if (args.size() != typeInfo.fields.size()) {
            runtimeError("type '" + typeName + "' expected " + std::to_string(typeInfo.fields.size()) +
                         " constructor arguments but received " + std::to_string(args.size()));
        }

        std::shared_ptr<ObjectData> object(new ObjectData(typeName));
        for (size_t index = 0; index < typeInfo.fields.size(); ++index) {
            object->fields[typeInfo.fields[index]] = args[index];
        }
        return Value(object);
    }

    bool hasMethod(const std::string& typeName, const std::string& name) const {
        std::unordered_map<std::string, TypeInfo>::const_iterator typeIt = types_.find(typeName);
        return typeIt != types_.end() && typeIt->second.methods.find(name) != typeIt->second.methods.end();
    }

    void pushScope() {
        scopes_.push_back(ScopeFrame());
    }

    void popScope() {
        while (!scopes_.back().defers.empty()) {
            const Statement* deferred = scopes_.back().defers.back();
            scopes_.back().defers.pop_back();
            executeStatements(deferred->body);
        }
        scopes_.pop_back();
    }

    ScopeFrame& currentScope() {
        return scopes_.back();
    }

    std::unordered_map<std::string, Value> captureVisibleVars() const {
        std::unordered_map<std::string, Value> captured;
        for (size_t index = 0; index < scopes_.size(); ++index) {
            for (std::unordered_map<std::string, Value>::const_iterator it = scopes_[index].vars.begin();
                 it != scopes_[index].vars.end();
                 ++it) {
                captured[it->first] = it->second;
            }
        }
        return captured;
    }

    Value getVariable(const std::string& name) const {
        for (size_t index = scopes_.size(); index > 0; --index) {
            std::unordered_map<std::string, Value>::const_iterator it = scopes_[index - 1].vars.find(name);
            if (it != scopes_[index - 1].vars.end()) {
                return it->second;
            }
        }
        runtimeError("unknown variable $" + name);
    }

    void storeVariable(const std::string& name, const Value& value) {
        for (size_t index = scopes_.size(); index > 0; --index) {
            std::unordered_map<std::string, Value>::iterator it = scopes_[index - 1].vars.find(name);
            if (it != scopes_[index - 1].vars.end()) {
                it->second = value;
                return;
            }
        }
        currentScope().vars[name] = value;
    }

    void expectArity(const std::vector<Value>& args, size_t expected, const std::string& name) const {
        if (args.size() != expected) {
            runtimeError("stage '" + name + "' expected " + std::to_string(expected) +
                         " arguments but received " + std::to_string(args.size()));
        }
    }

    void expectCallableValue(const Value& value, const std::string& context) const {
        if (value.type == Value::STRING) {
            return;
        }
        if (value.type == Value::CALLABLE && value.callableValue.get() != nullptr) {
            return;
        }
        runtimeError(context + " expects a callable name or pipe value");
    }

    bool isDirectBuiltinCallableName(const std::string& name) const {
        return name == "range" ||
               name == "str" ||
               name == "int" ||
               name == "bool" ||
               name == "type_of" ||
               name == "input" ||
               name == "bind" ||
               name == "chain" ||
               name == "branch" ||
               name == "guard";
    }

    int requireInt(const Value& value, const std::string& context) const {
        if (value.type != Value::INT) {
            runtimeError(context + " expected int but received " + value.typeName());
        }
        return value.intValue;
    }

    std::string requireString(const Value& value, const std::string& context) const {
        if (value.type != Value::STRING) {
            runtimeError(context + " expected string but received " + value.typeName());
        }
        return value.stringValue;
    }

    std::string requireSymbol(const Value& value, const std::string& context) const {
        return requireString(value, context);
    }

    int toInt(const Value& value) const {
        if (value.type == Value::INT) {
            return value.intValue;
        }
        if (value.type == Value::BOOL) {
            return value.boolValue ? 1 : 0;
        }
        if (value.type == Value::STRING) {
            std::istringstream stream(value.stringValue);
            int result = 0;
            stream >> result;
            if (!stream || !stream.eof()) {
                runtimeError("cannot convert string '" + value.stringValue + "' to int");
            }
            return result;
        }
        runtimeError("cannot convert " + value.typeName() + " to int");
    }

    bool isStreamValue(const Value& value) const {
        return value.type == Value::STREAM && value.streamValue.get() != nullptr;
    }

    struct StreamCursorState {
        explicit StreamCursorState(const std::shared_ptr<StreamData>& streamData)
            : data(streamData),
              current(streamData.get() == nullptr ? 0 : streamData->start),
              exhausted(streamData.get() == nullptr),
              skipRemaining() {
            if (data.get() != nullptr) {
                skipRemaining.reserve(data->operations.size());
                for (size_t index = 0; index < data->operations.size(); ++index) {
                    const StreamData::Operation& operation = data->operations[index];
                    skipRemaining.push_back(operation.kind == StreamData::SKIP ? operation.count : 0);
                }
            }
        }

        bool next(Interpreter* interpreter, Value* out, const std::string& stageName) {
            if (data.get() == nullptr || exhausted) {
                return false;
            }

            if (data->step == 0) {
                interpreter->runtimeError("range stream step cannot be zero");
            }

            while (true) {
                if (data->hasEnd) {
                    if ((data->step > 0 && current >= data->end) ||
                        (data->step < 0 && current <= data->end)) {
                        exhausted = true;
                        return false;
                    }
                }

                Value candidate(current);
                current += data->step;

                bool rejected = false;
                for (size_t index = 0; index < data->operations.size(); ++index) {
                    const StreamData::Operation& operation = data->operations[index];
                    if (operation.kind == StreamData::SKIP) {
                        if (skipRemaining[index] > 0) {
                            --skipRemaining[index];
                            rejected = true;
                            break;
                        }
                        continue;
                    }

                    if (operation.kind == StreamData::MAP) {
                        candidate = interpreter->invokePipeCallable(operation.callable, candidate, operation.extraArgs, stageName);
                        continue;
                    }

                    if (operation.kind == StreamData::FILTER) {
                        if (!interpreter->invokePipeCallable(operation.callable, candidate, operation.extraArgs, stageName).isTruthy()) {
                            rejected = true;
                            break;
                        }
                        continue;
                    }

                    if (operation.kind == StreamData::TAP) {
                        interpreter->invokePipeCallable(operation.callable, candidate, operation.extraArgs, stageName);
                        continue;
                    }

                    interpreter->runtimeError("unknown stream operation");
                }

                if (!rejected) {
                    *out = candidate;
                    return true;
                }
            }
        }

        std::shared_ptr<StreamData> data;
        int current;
        bool exhausted;
        std::vector<int> skipRemaining;
    };

    Value makeRange(int start, int end, bool hasEnd = true, int stepHint = 0) const {
        int step = stepHint;
        if (step == 0) {
            if (hasEnd) {
                step = start <= end ? 1 : -1;
            } else {
                step = 1;
            }
        }
        if (step == 0) {
            runtimeError("range step cannot be zero");
        }

        std::shared_ptr<StreamData> stream(new StreamData());
        stream->start = start;
        stream->end = end;
        stream->hasEnd = hasEnd;
        stream->step = step;
        return Value(stream);
    }

    StreamCursorState makeStreamCursor(const Value& input, const std::string& stageName) const {
        if (!isStreamValue(input)) {
            runtimeError("stage '" + stageName + "' expects a stream input");
        }
        return StreamCursorState(input.streamValue);
    }

    Value appendStreamOperation(const Value& input,
                                const StreamData::Operation& operation,
                                const std::string& stageName) const {
        if (!isStreamValue(input)) {
            runtimeError("stage '" + stageName + "' expects a stream input");
        }
        std::shared_ptr<StreamData> next(new StreamData(*input.streamValue));
        next->operations.push_back(operation);
        return Value(next);
    }

    std::vector<Value> takeFromStream(const Value& input, int count, const std::string& stageName) {
        if (count < 0) {
            runtimeError("stage '" + stageName + "' does not allow negative counts");
        }
        StreamCursorState cursor = makeStreamCursor(input, stageName);
        std::vector<Value> result;
        result.reserve(static_cast<size_t>(count));
        Value item;
        for (int index = 0; index < count; ++index) {
            if (!cursor.next(this, &item, stageName)) {
                break;
            }
            result.push_back(item);
        }
        return result;
    }

    std::vector<Value> materializeStream(const Value& input, const std::string& stageName) {
        StreamCursorState cursor = makeStreamCursor(input, stageName);
        std::vector<Value> result;
        Value item;
        while (cursor.next(this, &item, stageName)) {
            result.push_back(item);
        }
        return result;
    }

    Value materializeStreamValue(const Value& input, const std::string& stageName) {
        return isStreamValue(input) ? Value(materializeStream(input, stageName)) : input;
    }

    Value makeNativeCallable(CallableData::Kind kind, const std::vector<Value>& parts, const std::string& label) const {
        return Value(std::shared_ptr<CallableData>(new CallableData(kind, parts, label)));
    }

    std::vector<Value> copyArrayRange(const std::vector<Value>& items, size_t begin, size_t end) const {
        return std::vector<Value>(
            items.begin() + static_cast<std::vector<Value>::difference_type>(begin),
            items.begin() + static_cast<std::vector<Value>::difference_type>(end));
    }

    Value bindCallable(const Value& input,
                       const Value& callable,
                       const std::vector<Value>& extra,
                       const std::string& context) {
        return invokePipeCallable(callable, input, extra, context);
    }

    Value chainCallables(const Value& input,
                         const std::vector<Value>& callables,
                         const std::string& context) {
        Value current = input;
        for (size_t index = 0; index < callables.size(); ++index) {
            current = invokePipeCallable(callables[index], current, std::vector<Value>(), context);
        }
        return current;
    }

    Value branchCallables(const Value& input,
                          const std::vector<Value>& callables,
                          const std::string& context) {
        std::vector<Value> result;
        result.reserve(callables.size());
        for (size_t index = 0; index < callables.size(); ++index) {
            result.push_back(invokePipeCallable(callables[index], input, std::vector<Value>(), context));
        }
        return Value(result);
    }

    Value guardCallable(const Value& input,
                        const Value& predicate,
                        const Value& onTrue,
                        const Value* onFalse,
                        const std::string& context) {
        if (invokePipeCallable(predicate, input, std::vector<Value>(), context).isTruthy()) {
            return invokePipeCallable(onTrue, input, std::vector<Value>(), context);
        }
        if (onFalse != nullptr) {
            return invokePipeCallable(*onFalse, input, std::vector<Value>(), context);
        }
        return input;
    }

    std::string trimCopy(const std::string& input) const {
        size_t start = 0;
        while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
            ++start;
        }

        size_t end = input.size();
        while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
            --end;
        }

        return input.substr(start, end - start);
    }

    std::string toUpperCopy(const std::string& input) const {
        std::string result = input;
        for (size_t index = 0; index < result.size(); ++index) {
            result[index] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[index])));
        }
        return result;
    }

    std::string toLowerCopy(const std::string& input) const {
        std::string result = input;
        for (size_t index = 0; index < result.size(); ++index) {
            result[index] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[index])));
        }
        return result;
    }

    bool stringsStartsWith(const std::string& input, const std::string& prefix) const {
        return input.size() >= prefix.size() && input.compare(0, prefix.size(), prefix) == 0;
    }

    bool stringsEndsWith(const std::string& input, const std::string& suffix) const {
        return input.size() >= suffix.size() &&
               input.compare(input.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    std::string replaceAll(const std::string& input, const std::string& from, const std::string& to) const {
        std::string result;
        size_t start = 0;
        while (start < input.size()) {
            const size_t found = input.find(from, start);
            if (found == std::string::npos) {
                result.append(input, start, input.size() - start);
                break;
            }
            result.append(input, start, found - start);
            result += to;
            start = found + from.size();
        }
        return result;
    }

    std::vector<Value> splitString(const std::string& text, const std::string& separator) const {
        std::vector<Value> parts;
        if (separator.empty()) {
            for (size_t index = 0; index < text.size(); ++index) {
                parts.push_back(Value(std::string(1, text[index])));
            }
            return parts;
        }

        size_t start = 0;
        while (start <= text.size()) {
            const size_t found = text.find(separator, start);
            if (found == std::string::npos) {
                parts.push_back(Value(text.substr(start)));
                break;
            }
            parts.push_back(Value(text.substr(start, found - start)));
            start = found + separator.size();
        }
        return parts;
    }

    std::string joinArray(const std::vector<Value>& values, const std::string& separator) const {
        std::ostringstream stream;
        for (size_t index = 0; index < values.size(); ++index) {
            if (index > 0) {
                stream << separator;
            }
            stream << values[index].toString();
        }
        return stream.str();
    }

    bool containsValue(const Value& input, const Value& needle, const std::string& stageName) const {
        if (input.type == Value::STRING) {
            return input.stringValue.find(requireString(needle, stageName)) != std::string::npos;
        }

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                if (items[index].equals(needle)) {
                    return true;
                }
            }
            return false;
        }

        if (input.type == Value::DICT) {
            return input.asDict().find(requireString(needle, stageName)) != input.asDict().end();
        }

        if (input.type == Value::OBJECT && input.objectValue.get() != nullptr) {
            return input.objectValue->fields.find(requireString(needle, stageName)) != input.objectValue->fields.end();
        }

        runtimeError("stage '" + stageName + "' expects string, array, dict, or object input");
    }

    Value sliceValue(const Value& input, int start, int length, const std::string& stageName) const {
        if (input.type == Value::STRING) {
            if (start < 0 || length < 0 || static_cast<size_t>(start) >= input.stringValue.size()) {
                return Value("");
            }
            return Value(input.stringValue.substr(static_cast<size_t>(start), static_cast<size_t>(length)));
        }

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            if (start < 0 || length < 0 || static_cast<size_t>(start) >= items.size()) {
                return Value(std::vector<Value>());
            }
            const size_t begin = static_cast<size_t>(start);
            const size_t end = begin + std::min(static_cast<size_t>(length), items.size() - begin);
            return Value(copyArrayRange(items, begin, end));
        }

        runtimeError("stage '" + stageName + "' expects string or array input");
    }

    Value reverseValue(const Value& input, const std::string& stageName) const {
        if (input.type == Value::STRING) {
            std::string result = input.stringValue;
            std::reverse(result.begin(), result.end());
            return Value(result);
        }

        if (input.type == Value::ARRAY) {
            std::vector<Value> result = input.asArray();
            std::reverse(result.begin(), result.end());
            return Value(result);
        }

        runtimeError("stage '" + stageName + "' expects string or array input");
    }

    int indexOfValue(const Value& input, const Value& needle, const std::string& stageName) const {
        if (input.type == Value::STRING) {
            const size_t found = input.stringValue.find(requireString(needle, stageName));
            return found == std::string::npos ? -1 : static_cast<int>(found);
        }

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                if (items[index].equals(needle)) {
                    return static_cast<int>(index);
                }
            }
            return -1;
        }

        runtimeError("stage '" + stageName + "' expects string or array input");
    }

    Value repeatValue(const Value& input, int count, const std::string& stageName) const {
        if (count < 0) {
            runtimeError("stage '" + stageName + "' does not allow negative counts");
        }

        if (input.type == Value::STRING) {
            std::string result;
            for (int index = 0; index < count; ++index) {
                result += input.stringValue;
            }
            return Value(result);
        }

        if (input.type == Value::ARRAY) {
            std::vector<Value> result;
            const std::vector<Value>& items = input.asArray();
            result.reserve(items.size() * static_cast<size_t>(count));
            for (int index = 0; index < count; ++index) {
                result.insert(result.end(), items.begin(), items.end());
            }
            return Value(result);
        }

        runtimeError("stage '" + stageName + "' expects string or array input");
    }

    int sumValue(const Value& input, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        int total = 0;
        const std::vector<Value>& items = input.asArray();
        for (size_t index = 0; index < items.size(); ++index) {
            total += requireInt(items[index], stageName);
        }
        return total;
    }

    int sumByField(const Value& input, const Value& key, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const Value symbol(requireString(key, stageName));
        const std::vector<Value>& items = input.asArray();
        int total = 0;
        for (size_t index = 0; index < items.size(); ++index) {
            total += requireInt(readRecordField(items[index], symbol, stageName), stageName);
        }
        return total;
    }

    Value flattenValue(const Value& input, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        std::vector<Value> result;
        const std::vector<Value>& groups = input.asArray();
        for (size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
            if (groups[groupIndex].type != Value::ARRAY) {
                runtimeError("stage '" + stageName + "' expects an array of arrays");
            }
            const std::vector<Value>& items = groups[groupIndex].asArray();
            result.insert(result.end(), items.begin(), items.end());
        }
        return Value(result);
    }

    Value takeValue(const Value& input, int count, const std::string& stageName) const {
        if (count < 0) {
            runtimeError("stage '" + stageName + "' does not allow negative counts");
        }

        if (input.type == Value::STRING) {
            return Value(input.stringValue.substr(0, std::min(static_cast<size_t>(count), input.stringValue.size())));
        }

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            const size_t end = std::min(static_cast<size_t>(count), items.size());
            return Value(copyArrayRange(items, 0, end));
        }

        runtimeError("stage '" + stageName + "' expects string or array input");
    }

    Value skipValue(const Value& input, int count, const std::string& stageName) const {
        if (count < 0) {
            runtimeError("stage '" + stageName + "' does not allow negative counts");
        }

        if (input.type == Value::STRING) {
            const size_t start = std::min(static_cast<size_t>(count), input.stringValue.size());
            return Value(input.stringValue.substr(start));
        }

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            const size_t start = std::min(static_cast<size_t>(count), items.size());
            return Value(copyArrayRange(items, start, items.size()));
        }

        runtimeError("stage '" + stageName + "' expects string or array input");
    }

    Value distinctValue(const Value& input, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        std::vector<Value> result;
        const std::vector<Value>& items = input.asArray();
        for (size_t index = 0; index < items.size(); ++index) {
            bool exists = false;
            for (size_t seen = 0; seen < result.size(); ++seen) {
                if (result[seen].equals(items[index])) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                result.push_back(items[index]);
            }
        }
        return Value(result);
    }

    Value distinctByField(const Value& input, const Value& key, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const Value symbol(requireString(key, stageName));
        const std::vector<Value>& items = input.asArray();
        std::vector<Value> result;
        std::vector<Value> seenKeys;
        for (size_t index = 0; index < items.size(); ++index) {
            const Value currentKey = readRecordField(items[index], symbol, stageName);
            bool exists = false;
            for (size_t seen = 0; seen < seenKeys.size(); ++seen) {
                if (seenKeys[seen].equals(currentKey)) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                seenKeys.push_back(currentKey);
                result.push_back(items[index]);
            }
        }
        return Value(result);
    }

    bool compareSortableValues(const Value& left, const Value& right, const std::string& stageName) const {
        if (left.type != right.type) {
            runtimeError("stage '" + stageName + "' requires all array elements to have the same type");
        }

        if (left.type == Value::INT) {
            return left.intValue < right.intValue;
        }

        if (left.type == Value::STRING) {
            return left.stringValue < right.stringValue;
        }

        if (left.type == Value::BOOL) {
            return static_cast<int>(left.boolValue) < static_cast<int>(right.boolValue);
        }

        runtimeError("stage '" + stageName + "' supports sorting only int, string, or bool arrays");
    }

    Value sortValue(const Value& input, bool descending, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        std::vector<Value> result = input.asArray();
        if (result.empty()) {
            return Value(result);
        }

        const Value::Type baseType = result.front().type;
        if (baseType != Value::INT && baseType != Value::STRING && baseType != Value::BOOL) {
            runtimeError("stage '" + stageName + "' supports sorting only int, string, or bool arrays");
        }

        for (size_t index = 1; index < result.size(); ++index) {
            if (result[index].type != baseType) {
                runtimeError("stage '" + stageName + "' requires all array elements to have the same type");
            }
        }

        std::sort(result.begin(), result.end(),
                  [this, &stageName, descending](const Value& left, const Value& right) {
                      return descending ? compareSortableValues(right, left, stageName)
                                        : compareSortableValues(left, right, stageName);
                  });
        return Value(result);
    }

    Value sortByField(const Value& input, const Value& key, bool descending, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const Value symbol(requireString(key, stageName));
        std::vector<Value> result = input.asArray();
        if (result.empty()) {
            return Value(result);
        }

        const Value baseKey = readRecordField(result.front(), symbol, stageName);
        if (baseKey.type != Value::INT && baseKey.type != Value::STRING && baseKey.type != Value::BOOL) {
            runtimeError("stage '" + stageName + "' supports sorting only int, string, or bool record fields");
        }

        for (size_t index = 1; index < result.size(); ++index) {
            const Value currentKey = readRecordField(result[index], symbol, stageName);
            if (currentKey.type != baseKey.type) {
                runtimeError("stage '" + stageName + "' requires all record fields to have the same type");
            }
        }

        std::sort(result.begin(), result.end(),
                  [this, &symbol, &stageName, descending](const Value& left, const Value& right) {
                      const Value leftKey = readRecordField(left, symbol, stageName);
                      const Value rightKey = readRecordField(right, symbol, stageName);
                      return descending ? compareSortableValues(rightKey, leftKey, stageName)
                                        : compareSortableValues(leftKey, rightKey, stageName);
                  });
        return Value(result);
    }

    Value chunkValue(const Value& input, int size, const std::string& stageName) const {
        if (size <= 0) {
            runtimeError("stage '" + stageName + "' expects a positive chunk size");
        }

        if (input.type == Value::STRING) {
            std::vector<Value> result;
            for (size_t offset = 0; offset < input.stringValue.size(); offset += static_cast<size_t>(size)) {
                result.push_back(Value(input.stringValue.substr(offset, static_cast<size_t>(size))));
            }
            return Value(result);
        }

        if (input.type == Value::ARRAY) {
            std::vector<Value> result;
            const std::vector<Value>& items = input.asArray();
            for (size_t offset = 0; offset < items.size(); offset += static_cast<size_t>(size)) {
                const size_t end = std::min(offset + static_cast<size_t>(size), items.size());
                result.push_back(Value(copyArrayRange(items, offset, end)));
            }
            return Value(result);
        }

        runtimeError("stage '" + stageName + "' expects string or array input");
    }

    Value zipValue(const Value& input, const Value& other, const std::string& stageName) const {
        if (input.type != Value::ARRAY || other.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects array input and array argument");
        }

        const std::vector<Value>& left = input.asArray();
        const std::vector<Value>& right = other.asArray();
        const size_t limit = std::min(left.size(), right.size());
        std::vector<Value> result;
        result.reserve(limit);

        for (size_t index = 0; index < limit; ++index) {
            std::vector<Value> pair;
            pair.push_back(left[index]);
            pair.push_back(right[index]);
            result.push_back(Value(pair));
        }

        return Value(result);
    }

    Value tapValue(const Value& input, const Value& callable, const std::vector<Value>& extra, const std::string& stageName) {
        invokePipeCallable(callable, input, extra, stageName);
        return input;
    }

    Value parallelMapValue(const Value& input,
                           const Value& callable,
                           const std::vector<Value>& extra,
                           const std::string& stageName) {
        expectCallableValue(callable, stageName);

        Value arrayInput = input;
        if (isStreamValue(input)) {
            arrayInput = Value(materializeStream(input, stageName));
        }
        if (arrayInput.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const std::vector<Value>& items = arrayInput.asArray();
        std::vector<Value> result(items.size());
        if (items.empty()) {
            return Value(result);
        }

#if AETHE_ENABLE_THREADS
        const size_t workerCount = std::max<size_t>(1, std::min(sharedThreadPool().size(), items.size()));
        if (workerCount <= 1 || items.size() < 2) {
            for (size_t index = 0; index < items.size(); ++index) {
                result[index] = invokePipeCallable(callable, items[index], extra, stageName);
            }
            return Value(result);
        }

        const std::unordered_map<std::string, const Statement*> flowSnapshot = flows_;
        const std::unordered_map<std::string, const Statement*> stageSnapshot = stages_;
        const std::unordered_map<std::string, TypeInfo> typeSnapshot = types_;
        const std::unordered_map<std::string, Value> visibleSnapshot = captureVisibleVars();
        const size_t chunkSize = (items.size() + workerCount - 1) / workerCount;

        std::vector<std::future<void> > futures;
        futures.reserve(workerCount);
        for (size_t worker = 0; worker < workerCount; ++worker) {
            const size_t begin = worker * chunkSize;
            if (begin >= items.size()) {
                break;
            }
            const size_t end = std::min(items.size(), begin + chunkSize);
            futures.push_back(sharedThreadPool().submit(
                [begin, end, &items, &result, callable, extra, stageName, flowSnapshot, stageSnapshot, typeSnapshot, visibleSnapshot]() {
                    Interpreter workerInterpreter(
                        [](const std::string&) {},
                        [](const std::string&, std::string*) { return false; });
                    workerInterpreter.flows_ = flowSnapshot;
                    workerInterpreter.stages_ = stageSnapshot;
                    workerInterpreter.types_ = typeSnapshot;
                    workerInterpreter.scopes_.clear();
                    workerInterpreter.scopes_.push_back(ScopeFrame());
                    workerInterpreter.scopes_.back().vars = visibleSnapshot;

                    for (size_t index = begin; index < end; ++index) {
                        result[index] = workerInterpreter.invokePipeCallable(callable, items[index], extra, stageName);
                    }
                }));
        }

        std::exception_ptr firstError;
        for (size_t index = 0; index < futures.size(); ++index) {
            try {
                futures[index].get();
            } catch (...) {
                if (!firstError) {
                    firstError = std::current_exception();
                }
            }
        }
        if (firstError) {
            std::rethrow_exception(firstError);
        }
#else
        for (size_t index = 0; index < items.size(); ++index) {
            result[index] = invokePipeCallable(callable, items[index], extra, stageName);
        }
#endif
        return Value(result);
    }

    Value flatMapValue(const Value& input,
                       const Value& callable,
                       const std::vector<Value>& extra,
                       const std::string& stageName) {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        std::vector<Value> result;
        const std::vector<Value>& items = input.asArray();
        for (size_t index = 0; index < items.size(); ++index) {
            Value mapped = invokePipeCallable(callable, items[index], extra, stageName);
            if (mapped.type != Value::ARRAY) {
                runtimeError("stage '" + stageName + "' expects callable to return an array");
            }
            const std::vector<Value>& group = mapped.asArray();
            result.insert(result.end(), group.begin(), group.end());
        }

        return Value(result);
    }

    Value groupByValue(const Value& input,
                       const Value& callable,
                       const std::vector<Value>& extra,
                       const std::string& stageName) {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        std::unordered_map<std::string, std::vector<Value> > grouped;
        const std::vector<Value>& items = input.asArray();
        for (size_t index = 0; index < items.size(); ++index) {
            const Value keyValue = invokePipeCallable(callable, items[index], extra, stageName);
            const std::string key = requireString(keyValue, stageName);
            grouped[key].push_back(items[index]);
        }

        std::unordered_map<std::string, Value> result;
        for (std::unordered_map<std::string, std::vector<Value> >::const_iterator it = grouped.begin();
             it != grouped.end();
             ++it) {
            result[it->first] = Value(it->second);
        }
        return Value(result);
    }

    int containerSize(const Value& value, const std::string& name) const {
        if (value.type == Value::STRING) {
            return static_cast<int>(value.stringValue.size());
        }
        if (value.type == Value::ARRAY) {
            return static_cast<int>(value.asArray().size());
        }
        if (value.type == Value::DICT) {
            return static_cast<int>(value.asDict().size());
        }
        if (value.type == Value::OBJECT && value.objectValue.get() != nullptr) {
            return static_cast<int>(value.objectValue->fields.size());
        }
        runtimeError("stage '" + name + "' expects string, array, dict, or object input");
    }

    std::unordered_map<std::string, Value> copyRecordEntries(const Value& value, const std::string& stageName) const {
        if (value.type == Value::DICT) {
            return value.asDict();
        }

        if (value.type == Value::OBJECT && value.objectValue.get() != nullptr) {
            return value.objectValue->fields;
        }

        runtimeError("stage '" + stageName + "' expects dict or object input");
    }

    Value readMember(const Value& object, const std::string& memberName) const {
        if (object.type == Value::DICT) {
            const std::unordered_map<std::string, Value>& entries = object.asDict();
            std::unordered_map<std::string, Value>::const_iterator it = entries.find(memberName);
            return it == entries.end() ? Value() : it->second;
        }

        if (object.type == Value::OBJECT && object.objectValue.get() != nullptr) {
            std::unordered_map<std::string, Value>::const_iterator it = object.objectValue->fields.find(memberName);
            return it == object.objectValue->fields.end() ? Value() : it->second;
        }

        runtimeError("member access requires dict or object input");
    }

    Value readIndexedValue(const Value& container, const Value& index, const std::string& stageName) const {
        if (container.type == Value::ARRAY) {
            const int offset = requireInt(index, stageName);
            const std::vector<Value>& items = container.asArray();
            if (offset < 0 || static_cast<size_t>(offset) >= items.size()) {
                return Value();
            }
            return items[static_cast<size_t>(offset)];
        }

        if (container.type == Value::STRING) {
            const int offset = requireInt(index, stageName);
            if (offset < 0 || static_cast<size_t>(offset) >= container.stringValue.size()) {
                return Value();
            }
            return Value(std::string(1, container.stringValue[static_cast<size_t>(offset)]));
        }

        if (container.type == Value::DICT) {
            const std::string key = requireString(index, stageName);
            const std::unordered_map<std::string, Value>& entries = container.asDict();
            std::unordered_map<std::string, Value>::const_iterator it = entries.find(key);
            return it == entries.end() ? Value() : it->second;
        }

        if (container.type == Value::OBJECT && container.objectValue.get() != nullptr) {
            const std::string key = requireString(index, stageName);
            std::unordered_map<std::string, Value>::const_iterator it = container.objectValue->fields.find(key);
            return it == container.objectValue->fields.end() ? Value() : it->second;
        }

        runtimeError("stage '" + stageName + "' expects array, string, dict, or object input");
    }

    Value writeIndexedValue(const Value& container, const Value& index, const Value& value, const std::string& stageName) const {
        if (container.type == Value::ARRAY) {
            const int offset = requireInt(index, stageName);
            if (offset < 0 || static_cast<size_t>(offset) >= container.asArray().size()) {
                runtimeError("stage '" + stageName + "' array index out of range");
            }
            std::vector<Value> result = container.asArray();
            result[static_cast<size_t>(offset)] = value;
            return Value(result);
        }

        if (container.type == Value::STRING) {
            const int offset = requireInt(index, stageName);
            if (offset < 0 || static_cast<size_t>(offset) >= container.stringValue.size()) {
                runtimeError("stage '" + stageName + "' string index out of range");
            }
            std::string result = container.stringValue;
            result.replace(static_cast<size_t>(offset), 1, requireString(value, stageName));
            return Value(result);
        }

        if (container.type == Value::DICT) {
            std::unordered_map<std::string, Value> result = container.asDict();
            result[requireString(index, stageName)] = value;
            return Value(result);
        }

        if (container.type == Value::OBJECT && container.objectValue.get() != nullptr) {
            container.objectValue->fields[requireString(index, stageName)] = value;
            return container;
        }

        runtimeError("stage '" + stageName + "' expects array, string, dict, or object input");
    }

    Value insertIndexedValue(const Value& container, int index, const Value& value, const std::string& stageName) const {
        if (index < 0) {
            runtimeError("stage '" + stageName + "' does not allow negative indexes");
        }

        if (container.type == Value::ARRAY) {
            std::vector<Value> result = container.asArray();
            const size_t offset = static_cast<size_t>(index);
            if (offset > result.size()) {
                runtimeError("stage '" + stageName + "' array index out of range");
            }
            result.insert(result.begin() + static_cast<std::vector<Value>::difference_type>(offset), value);
            return Value(result);
        }

        if (container.type == Value::STRING) {
            std::string result = container.stringValue;
            const size_t offset = static_cast<size_t>(index);
            if (offset > result.size()) {
                runtimeError("stage '" + stageName + "' string index out of range");
            }
            result.insert(offset, requireString(value, stageName));
            return Value(result);
        }

        runtimeError("stage '" + stageName + "' expects array or string input");
    }

    Value updateIndexedValue(const Value& container,
                             const Value& index,
                             const Value& callable,
                             const std::vector<Value>& extra,
                             const std::string& stageName) {
        expectCallableValue(callable, stageName);
        const Value current = readIndexedValue(container, index, stageName);
        const Value updated = invokePipeCallable(callable, current, extra, stageName);
        return writeIndexedValue(container, index, updated, stageName);
    }

    Value removeIndexedValue(const Value& container, const Value& index, const std::string& stageName) const {
        if (container.type == Value::ARRAY) {
            const int offset = requireInt(index, stageName);
            std::vector<Value> result = container.asArray();
            if (offset < 0 || static_cast<size_t>(offset) >= result.size()) {
                runtimeError("stage '" + stageName + "' array index out of range");
            }
            result.erase(result.begin() + static_cast<std::vector<Value>::difference_type>(offset));
            return Value(result);
        }

        if (container.type == Value::STRING) {
            const int offset = requireInt(index, stageName);
            std::string result = container.stringValue;
            if (offset < 0 || static_cast<size_t>(offset) >= result.size()) {
                runtimeError("stage '" + stageName + "' string index out of range");
            }
            result.erase(static_cast<size_t>(offset), 1);
            return Value(result);
        }

        if (container.type == Value::DICT) {
            std::unordered_map<std::string, Value> result = container.asDict();
            result.erase(requireString(index, stageName));
            return Value(result);
        }

        if (container.type == Value::OBJECT && container.objectValue.get() != nullptr) {
            container.objectValue->fields.erase(requireString(index, stageName));
            return container;
        }

        runtimeError("stage '" + stageName + "' expects array, string, dict, or object input");
    }

    Value readKeys(const Value& input, const std::string& stageName) const {
        std::vector<std::string> keys;

        if (input.type == Value::DICT) {
            const std::unordered_map<std::string, Value>& entries = input.asDict();
            for (std::unordered_map<std::string, Value>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                keys.push_back(it->first);
            }
        } else if (input.type == Value::OBJECT && input.objectValue.get() != nullptr) {
            for (std::unordered_map<std::string, Value>::const_iterator it = input.objectValue->fields.begin(); it != input.objectValue->fields.end(); ++it) {
                keys.push_back(it->first);
            }
        } else {
            runtimeError("stage '" + stageName + "' expects dict or object input");
        }

        std::sort(keys.begin(), keys.end());
        std::vector<Value> result;
        for (size_t index = 0; index < keys.size(); ++index) {
            result.push_back(Value(keys[index]));
        }
        return Value(result);
    }

    Value readValues(const Value& input, const std::string& stageName) const {
        std::vector<Value> result;

        if (input.type == Value::DICT) {
            std::vector<std::string> keys;
            const std::unordered_map<std::string, Value>& entries = input.asDict();
            for (std::unordered_map<std::string, Value>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                keys.push_back(it->first);
            }
            std::sort(keys.begin(), keys.end());
            for (size_t index = 0; index < keys.size(); ++index) {
                result.push_back(entries.find(keys[index])->second);
            }
            return Value(result);
        }

        if (input.type == Value::OBJECT && input.objectValue.get() != nullptr) {
            std::vector<std::string> keys;
            for (std::unordered_map<std::string, Value>::const_iterator it = input.objectValue->fields.begin(); it != input.objectValue->fields.end(); ++it) {
                keys.push_back(it->first);
            }
            std::sort(keys.begin(), keys.end());
            for (size_t index = 0; index < keys.size(); ++index) {
                result.push_back(input.objectValue->fields.find(keys[index])->second);
            }
            return Value(result);
        }

        runtimeError("stage '" + stageName + "' expects dict or object input");
    }

    Value readEntries(const Value& input, const std::string& stageName) const {
        std::unordered_map<std::string, Value> entries = copyRecordEntries(input, stageName);
        std::vector<std::string> keys;
        keys.reserve(entries.size());
        for (std::unordered_map<std::string, Value>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
            keys.push_back(it->first);
        }
        std::sort(keys.begin(), keys.end());

        std::vector<Value> result;
        result.reserve(keys.size());
        for (size_t index = 0; index < keys.size(); ++index) {
            std::unordered_map<std::string, Value> pair;
            pair["key"] = Value(keys[index]);
            pair["value"] = entries.find(keys[index])->second;
            result.push_back(Value(pair));
        }
        return Value(result);
    }

    Value pickEntries(const Value& input, const std::vector<Value>& keys, const std::string& stageName) const {
        const std::unordered_map<std::string, Value> entries = copyRecordEntries(input, stageName);
        std::unordered_map<std::string, Value> result;
        for (size_t index = 0; index < keys.size(); ++index) {
            const std::string key = requireString(keys[index], stageName);
            std::unordered_map<std::string, Value>::const_iterator it = entries.find(key);
            if (it != entries.end()) {
                result[key] = it->second;
            }
        }
        return Value(result);
    }

    Value omitEntries(const Value& input, const std::vector<Value>& keys, const std::string& stageName) const {
        std::unordered_map<std::string, Value> result = copyRecordEntries(input, stageName);
        for (size_t index = 0; index < keys.size(); ++index) {
            result.erase(requireString(keys[index], stageName));
        }
        return Value(result);
    }

    Value mergeEntries(const Value& input, const Value& other, const std::string& stageName) const {
        std::unordered_map<std::string, Value> result = copyRecordEntries(input, stageName);
        const std::unordered_map<std::string, Value> additions = copyRecordEntries(other, stageName);
        for (std::unordered_map<std::string, Value>::const_iterator it = additions.begin(); it != additions.end(); ++it) {
            result[it->first] = it->second;
        }
        return Value(result);
    }

    Value renameEntry(const Value& input, const Value& from, const Value& to, const std::string& stageName) const {
        std::unordered_map<std::string, Value> result = copyRecordEntries(input, stageName);
        const std::string fromKey = requireString(from, stageName);
        const std::string toKey = requireString(to, stageName);
        std::unordered_map<std::string, Value>::iterator it = result.find(fromKey);
        if (it == result.end()) {
            return Value(result);
        }

        if (fromKey == toKey) {
            return Value(result);
        }

        result[toKey] = it->second;
        result.erase(it);
        return Value(result);
    }

    Value evolveField(const Value& input,
                      const Value& key,
                      const Value& callable,
                      const std::vector<Value>& extra,
                      const std::string& stageName) {
        const std::string fieldName = requireString(key, stageName);

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            std::vector<Value> result;
            result.reserve(items.size());
            for (size_t index = 0; index < items.size(); ++index) {
                result.push_back(evolveSingleRecord(items[index], fieldName, callable, extra, stageName));
            }
            return Value(result);
        }

        return evolveSingleRecord(input, fieldName, callable, extra, stageName);
    }

    Value deriveField(const Value& input,
                      const Value& key,
                      const Value& callable,
                      const std::vector<Value>& extra,
                      const std::string& stageName) {
        const std::string fieldName = requireString(key, stageName);

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            std::vector<Value> result;
            result.reserve(items.size());
            for (size_t index = 0; index < items.size(); ++index) {
                result.push_back(deriveSingleRecord(items[index], fieldName, callable, extra, stageName));
            }
            return Value(result);
        }

        return deriveSingleRecord(input, fieldName, callable, extra, stageName);
    }

    Value readRecordField(const Value& item, const Value& key, const std::string& stageName) const {
        if (item.type != Value::DICT &&
            (item.type != Value::OBJECT || item.objectValue.get() == nullptr)) {
            runtimeError("stage '" + stageName + "' expects an array of dict or object values");
        }
        return readIndexedValue(item, key, stageName);
    }

    Value evolveSingleRecord(const Value& item,
                             const std::string& fieldName,
                             const Value& callable,
                             const std::vector<Value>& extra,
                             const std::string& stageName) {
        std::unordered_map<std::string, Value> entries = copyRecordEntries(item, stageName);
        const Value current = readRecordField(item, Value(fieldName), stageName);
        entries[fieldName] = invokePipeCallable(callable, current, extra, stageName);
        return Value(entries);
    }

    Value deriveSingleRecord(const Value& item,
                             const std::string& fieldName,
                             const Value& callable,
                             const std::vector<Value>& extra,
                             const std::string& stageName) {
        std::unordered_map<std::string, Value> entries = copyRecordEntries(item, stageName);
        entries[fieldName] = invokePipeCallable(callable, item, extra, stageName);
        return Value(entries);
    }

    Value indexByField(const Value& input, const Value& key, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const Value symbol(requireString(key, stageName));
        const std::vector<Value>& items = input.asArray();
        std::unordered_map<std::string, Value> result;
        for (size_t index = 0; index < items.size(); ++index) {
            const Value indexedKey = readRecordField(items[index], symbol, stageName);
            result[requireString(indexedKey, stageName)] = items[index];
        }
        return Value(result);
    }

    Value countByField(const Value& input, const Value& key, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const Value symbol(requireString(key, stageName));
        const std::vector<Value>& items = input.asArray();
        std::unordered_map<std::string, Value> result;
        for (size_t index = 0; index < items.size(); ++index) {
            const std::string indexedKey = requireString(readRecordField(items[index], symbol, stageName), stageName);
            std::unordered_map<std::string, Value>::iterator it = result.find(indexedKey);
            if (it == result.end()) {
                result[indexedKey] = Value(1);
            } else {
                it->second = Value(requireInt(it->second, stageName) + 1);
            }
        }
        return Value(result);
    }

    Value pluckValues(const Value& input, const Value& key, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const Value symbol(requireString(key, stageName));
        const std::vector<Value>& items = input.asArray();
        std::vector<Value> result;
        result.reserve(items.size());
        for (size_t index = 0; index < items.size(); ++index) {
            result.push_back(readRecordField(items[index], symbol, stageName));
        }
        return Value(result);
    }

    Value whereEntries(const Value& input, const Value& key, const Value& expected, const std::string& stageName) const {
        if (input.type != Value::ARRAY) {
            runtimeError("stage '" + stageName + "' expects an array input");
        }

        const Value symbol(requireString(key, stageName));
        const std::vector<Value>& items = input.asArray();
        std::vector<Value> result;
        for (size_t index = 0; index < items.size(); ++index) {
            if (readRecordField(items[index], symbol, stageName).equals(expected)) {
                result.push_back(items[index]);
            }
        }
        return Value(result);
    }

    Value readBoundary(const Value& input, bool first, const std::string& stageName) const {
        if (input.type == Value::ARRAY) {
            if (input.asArray().empty()) {
                return Value();
            }
            return first ? input.asArray().front() : input.asArray().back();
        }

        if (input.type == Value::STRING) {
            if (input.stringValue.empty()) {
                return Value();
            }
            return Value(std::string(1, first ? input.stringValue.front() : input.stringValue.back()));
        }

        runtimeError("stage '" + stageName + "' expects array or string input");
    }

    Value windowValue(const Value& input, int size, const std::string& stageName) const {
        if (size <= 0) {
            runtimeError("stage '" + stageName + "' expects a positive window size");
        }

        if (input.type == Value::ARRAY) {
            const std::vector<Value>& items = input.asArray();
            const size_t width = static_cast<size_t>(size);
            std::vector<Value> result;
            if (width > items.size()) {
                return Value(result);
            }
            result.reserve(items.size() - width + 1);
            for (size_t offset = 0; offset + width <= items.size(); ++offset) {
                result.push_back(Value(copyArrayRange(items, offset, offset + width)));
            }
            return Value(result);
        }

        if (input.type == Value::STRING) {
            const size_t width = static_cast<size_t>(size);
            std::vector<Value> result;
            if (width > input.stringValue.size()) {
                return Value(result);
            }
            result.reserve(input.stringValue.size() - width + 1);
            for (size_t offset = 0; offset + width <= input.stringValue.size(); ++offset) {
                result.push_back(Value(input.stringValue.substr(offset, width)));
            }
            return Value(result);
        }

        runtimeError("stage '" + stageName + "' expects array or string input");
    }

    [[noreturn]] void runtimeError(const std::string& message) const {
        throw std::runtime_error("Runtime Error: " + message);
    }

    static void consoleOutput(const std::string& text) {
        std::cout << text;
        std::cout.flush();
    }

    static bool consoleInput(const std::string& prompt, std::string* line) {
        if (!prompt.empty()) {
            std::cout << prompt;
            std::cout.flush();
        }
        return static_cast<bool>(std::getline(std::cin, *line));
    }

    std::vector<ScopeFrame> scopes_;
    std::unordered_map<std::string, const Statement*> flows_;
    std::unordered_map<std::string, const Statement*> stages_;
    std::unordered_map<std::string, std::shared_ptr<CompiledFunction> > compiledFlows_;
    std::unordered_map<std::string, std::shared_ptr<CompiledFunction> > compiledStages_;
    std::unordered_map<std::string, TypeInfo> types_;
    int callDepth_;
    int loopDepth_;
    OutputHandler outputHandler_;
    InputHandler inputHandler_;
};

struct BytecodeInstruction {
    enum Op {
        PUSH_CONST,
        LOAD_VAR,
        STORE_VAR_KEEP,
        MAKE_ARRAY,
        MAKE_DICT,
        UNARY_NOT,
        UNARY_NEG,
        BINARY_OP,
        ASSIGN_OP,
        GET_MEMBER,
        GET_INDEX,
        CALL_NAMED,
        CALL_MEMBER,
        CALL_VALUE,
        MAKE_PIPE,
        PUSH_SCOPE,
        POP_SCOPE,
        JUMP,
        JUMP_IF_FALSE,
        JUMP_IF_TRUE,
        TRUTHY,
        PIPE_NAMED,
        PIPE_VALUE,
        PIPE_INLINE_EXPR,
        LOOP_ENTER,
        LOOP_EXIT,
        LOOP_BREAK,
        LOOP_CONTINUE,
        RETURN_VALUE,
        POP
    };

    explicit BytecodeInstruction(Op opcode = POP)
        : op(opcode),
          operand(0),
          operand2(0),
          flag(false),
          text(),
          constant(),
          names(),
          expr(nullptr),
          statement(nullptr),
          function() {
    }

    Op op;
    int operand;
    int operand2;
    bool flag;
    std::string text;
    Value constant;
    std::vector<std::string> names;
    const Expr* expr;
    const Statement* statement;
    std::shared_ptr<CompiledFunction> function;
};

struct CompiledFunction {
    enum Kind {
        SCRIPT,
        FLOW,
        STAGE,
        PIPE,
        INLINE_EXPR
    };

    CompiledFunction()
        : kind(SCRIPT), name(), params(), code(), syntheticId(0) {
    }

    Kind kind;
    std::string name;
    std::vector<std::string> params;
    std::vector<BytecodeInstruction> code;
    int syntheticId;
};

struct CompiledProgram {
    CompiledProgram()
        : entry(), flows(), stages(), functions() {
    }

    std::shared_ptr<CompiledFunction> entry;
    std::unordered_map<std::string, std::shared_ptr<CompiledFunction> > flows;
    std::unordered_map<std::string, std::shared_ptr<CompiledFunction> > stages;
    std::vector<std::shared_ptr<CompiledFunction> > functions;
};

namespace bytecode_optimizer {

inline bool requiresIntOperands(const std::string& op) {
    return op == "-" || op == "*" || op == "/" || op == "%" ||
           op == ">" || op == ">=" || op == "<" || op == "<=";
}

inline bool tryFoldUnary(const BytecodeInstruction& valueInstruction,
                         const BytecodeInstruction& opInstruction,
                         BytecodeInstruction* foldedInstruction) {
    if (foldedInstruction == nullptr || valueInstruction.op != BytecodeInstruction::PUSH_CONST) {
        return false;
    }

    if (opInstruction.op == BytecodeInstruction::UNARY_NOT) {
        *foldedInstruction = BytecodeInstruction(BytecodeInstruction::PUSH_CONST);
        foldedInstruction->constant = Value(!valueInstruction.constant.isTruthy());
        return true;
    }

    if (opInstruction.op == BytecodeInstruction::UNARY_NEG &&
        valueInstruction.constant.type == Value::INT) {
        *foldedInstruction = BytecodeInstruction(BytecodeInstruction::PUSH_CONST);
        foldedInstruction->constant = Value(-valueInstruction.constant.intValue);
        return true;
    }

    if (opInstruction.op == BytecodeInstruction::TRUTHY) {
        *foldedInstruction = BytecodeInstruction(BytecodeInstruction::PUSH_CONST);
        foldedInstruction->constant = Value(valueInstruction.constant.isTruthy());
        return true;
    }

    return false;
}

inline bool tryFoldBinary(const BytecodeInstruction& leftInstruction,
                          const BytecodeInstruction& rightInstruction,
                          const BytecodeInstruction& opInstruction,
                          BytecodeInstruction* foldedInstruction) {
    if (foldedInstruction == nullptr ||
        leftInstruction.op != BytecodeInstruction::PUSH_CONST ||
        rightInstruction.op != BytecodeInstruction::PUSH_CONST ||
        opInstruction.op != BytecodeInstruction::BINARY_OP) {
        return false;
    }

    const Value& left = leftInstruction.constant;
    const Value& right = rightInstruction.constant;
    const std::string& op = opInstruction.text;

    if (op == "+") {
        *foldedInstruction = BytecodeInstruction(BytecodeInstruction::PUSH_CONST);
        if (left.type == Value::STRING || right.type == Value::STRING) {
            foldedInstruction->constant = Value(left.toString() + right.toString());
            return true;
        }
        if (left.type == Value::INT && right.type == Value::INT) {
            foldedInstruction->constant = Value(left.intValue + right.intValue);
            return true;
        }
        return false;
    }

    if (requiresIntOperands(op)) {
        if (left.type != Value::INT || right.type != Value::INT) {
            return false;
        }

        if ((op == "/" || op == "%") && right.intValue == 0) {
            return false;
        }

        *foldedInstruction = BytecodeInstruction(BytecodeInstruction::PUSH_CONST);
        if (op == "-") {
            foldedInstruction->constant = Value(left.intValue - right.intValue);
            return true;
        }
        if (op == "*") {
            foldedInstruction->constant = Value(left.intValue * right.intValue);
            return true;
        }
        if (op == "/") {
            foldedInstruction->constant = Value(left.intValue / right.intValue);
            return true;
        }
        if (op == "%") {
            foldedInstruction->constant = Value(left.intValue % right.intValue);
            return true;
        }
        if (op == ">") {
            foldedInstruction->constant = Value(left.intValue > right.intValue);
            return true;
        }
        if (op == ">=") {
            foldedInstruction->constant = Value(left.intValue >= right.intValue);
            return true;
        }
        if (op == "<") {
            foldedInstruction->constant = Value(left.intValue < right.intValue);
            return true;
        }
        if (op == "<=") {
            foldedInstruction->constant = Value(left.intValue <= right.intValue);
            return true;
        }
        return false;
    }

    if (op == "==") {
        *foldedInstruction = BytecodeInstruction(BytecodeInstruction::PUSH_CONST);
        foldedInstruction->constant = Value(left.equals(right));
        return true;
    }

    if (op == "!=") {
        *foldedInstruction = BytecodeInstruction(BytecodeInstruction::PUSH_CONST);
        foldedInstruction->constant = Value(!left.equals(right));
        return true;
    }

    return false;
}

inline int translateTarget(const std::vector<int>& firstNewIndexAtOld,
                           int oldTarget,
                           int oldSize,
                           int newSize) {
    if (oldTarget >= oldSize) {
        return newSize;
    }

    int target = oldTarget;
    if (target < 0) {
        target = 0;
    }

    for (int probe = target; probe <= oldSize; ++probe) {
        if (firstNewIndexAtOld[static_cast<size_t>(probe)] != -1) {
            return firstNewIndexAtOld[static_cast<size_t>(probe)];
        }
    }

    return newSize;
}

inline void remapTargets(std::vector<BytecodeInstruction>* code,
                         const std::vector<int>& origins,
                         int oldSize) {
    if (code == nullptr) {
        return;
    }

    std::vector<int> firstNewIndexAtOld(static_cast<size_t>(oldSize) + 1, -1);
    for (size_t newIndex = 0; newIndex < origins.size(); ++newIndex) {
        const int origin = origins[newIndex];
        if (origin >= 0 &&
            origin <= oldSize &&
            firstNewIndexAtOld[static_cast<size_t>(origin)] == -1) {
            firstNewIndexAtOld[static_cast<size_t>(origin)] = static_cast<int>(newIndex);
        }
    }
    firstNewIndexAtOld[static_cast<size_t>(oldSize)] = static_cast<int>(code->size());

    for (size_t index = 0; index < code->size(); ++index) {
        BytecodeInstruction& instruction = (*code)[index];
        if (instruction.op == BytecodeInstruction::JUMP ||
            instruction.op == BytecodeInstruction::JUMP_IF_FALSE ||
            instruction.op == BytecodeInstruction::JUMP_IF_TRUE) {
            instruction.operand = translateTarget(firstNewIndexAtOld,
                                                  instruction.operand,
                                                  oldSize,
                                                  static_cast<int>(code->size()));
            continue;
        }

        if (instruction.op == BytecodeInstruction::LOOP_ENTER) {
            instruction.operand = translateTarget(firstNewIndexAtOld,
                                                  instruction.operand,
                                                  oldSize,
                                                  static_cast<int>(code->size()));
            instruction.operand2 = translateTarget(firstNewIndexAtOld,
                                                   instruction.operand2,
                                                   oldSize,
                                                   static_cast<int>(code->size()));
        }
    }
}

inline bool optimizeFunctionPass(const std::shared_ptr<CompiledFunction>& function) {
    if (function.get() == nullptr || function->code.empty()) {
        return false;
    }

    const std::vector<BytecodeInstruction> oldCode = function->code;
    std::vector<BytecodeInstruction> newCode;
    std::vector<int> origins;
    newCode.reserve(oldCode.size());
    origins.reserve(oldCode.size());

    bool changed = false;
    size_t index = 0;
    while (index < oldCode.size()) {
        BytecodeInstruction folded;

        if (index + 2 < oldCode.size() &&
            tryFoldBinary(oldCode[index], oldCode[index + 1], oldCode[index + 2], &folded)) {
            newCode.push_back(folded);
            origins.push_back(static_cast<int>(index));
            index += 3;
            changed = true;
            continue;
        }

        if (index + 1 < oldCode.size() &&
            tryFoldUnary(oldCode[index], oldCode[index + 1], &folded)) {
            newCode.push_back(folded);
            origins.push_back(static_cast<int>(index));
            index += 2;
            changed = true;
            continue;
        }

        if (index + 1 < oldCode.size() &&
            oldCode[index].op == BytecodeInstruction::TRUTHY &&
            (oldCode[index + 1].op == BytecodeInstruction::TRUTHY ||
             oldCode[index + 1].op == BytecodeInstruction::UNARY_NOT ||
             oldCode[index + 1].op == BytecodeInstruction::JUMP_IF_FALSE ||
             oldCode[index + 1].op == BytecodeInstruction::JUMP_IF_TRUE)) {
            newCode.push_back(oldCode[index + 1]);
            origins.push_back(static_cast<int>(index + 1));
            index += 2;
            changed = true;
            continue;
        }

        if (index + 1 < oldCode.size() &&
            oldCode[index].op == BytecodeInstruction::PUSH_CONST &&
            (oldCode[index + 1].op == BytecodeInstruction::JUMP_IF_FALSE ||
             oldCode[index + 1].op == BytecodeInstruction::JUMP_IF_TRUE)) {
            const bool shouldJump =
                oldCode[index + 1].op == BytecodeInstruction::JUMP_IF_FALSE
                    ? !oldCode[index].constant.isTruthy()
                    : oldCode[index].constant.isTruthy();
            if (shouldJump) {
                BytecodeInstruction jump(BytecodeInstruction::JUMP);
                jump.operand = oldCode[index + 1].operand;
                newCode.push_back(jump);
                origins.push_back(static_cast<int>(index));
            }
            index += 2;
            changed = true;
            continue;
        }

        if (index + 1 < oldCode.size() &&
            oldCode[index].op == BytecodeInstruction::PUSH_CONST &&
            oldCode[index + 1].op == BytecodeInstruction::POP) {
            index += 2;
            changed = true;
            continue;
        }

        if (index + 1 < oldCode.size() &&
            oldCode[index].op == BytecodeInstruction::PUSH_SCOPE &&
            oldCode[index + 1].op == BytecodeInstruction::POP_SCOPE) {
            index += 2;
            changed = true;
            continue;
        }

        if (oldCode[index].op == BytecodeInstruction::JUMP &&
            oldCode[index].operand == static_cast<int>(index + 1)) {
            ++index;
            changed = true;
            continue;
        }

        newCode.push_back(oldCode[index]);
        origins.push_back(static_cast<int>(index));
        ++index;
    }

    if (!changed) {
        return false;
    }

    remapTargets(&newCode, origins, static_cast<int>(oldCode.size()));
    function->code.swap(newCode);
    return true;
}

inline bool eliminateUnreachableCode(const std::shared_ptr<CompiledFunction>& function) {
    if (function.get() == nullptr || function->code.empty()) {
        return false;
    }

    const std::vector<BytecodeInstruction> oldCode = function->code;
    std::vector<bool> reachable(oldCode.size(), false);
    std::vector<int> worklist(1, 0);

    while (!worklist.empty()) {
        const int ip = worklist.back();
        worklist.pop_back();

        if (ip < 0 || ip >= static_cast<int>(oldCode.size()) || reachable[static_cast<size_t>(ip)]) {
            continue;
        }

        reachable[static_cast<size_t>(ip)] = true;
        const BytecodeInstruction& instruction = oldCode[static_cast<size_t>(ip)];

        if (instruction.op == BytecodeInstruction::JUMP) {
            worklist.push_back(instruction.operand);
            continue;
        }

        if (instruction.op == BytecodeInstruction::JUMP_IF_FALSE ||
            instruction.op == BytecodeInstruction::JUMP_IF_TRUE) {
            worklist.push_back(ip + 1);
            worklist.push_back(instruction.operand);
            continue;
        }

        if (instruction.op == BytecodeInstruction::RETURN_VALUE) {
            continue;
        }

        if (instruction.op == BytecodeInstruction::LOOP_BREAK ||
            instruction.op == BytecodeInstruction::LOOP_CONTINUE) {
            worklist.push_back(ip + 1);
            continue;
        }

        worklist.push_back(ip + 1);
    }

    bool changed = false;
    std::vector<BytecodeInstruction> newCode;
    std::vector<int> origins;
    newCode.reserve(oldCode.size());
    origins.reserve(oldCode.size());

    for (size_t index = 0; index < oldCode.size(); ++index) {
        if (!reachable[index]) {
            changed = true;
            continue;
        }
        newCode.push_back(oldCode[index]);
        origins.push_back(static_cast<int>(index));
    }

    if (!changed) {
        return false;
    }

    remapTargets(&newCode, origins, static_cast<int>(oldCode.size()));
    function->code.swap(newCode);
    return true;
}

inline void optimizeFunction(const std::shared_ptr<CompiledFunction>& function) {
    for (int pass = 0; pass < 8; ++pass) {
        const bool peepholeChanged = optimizeFunctionPass(function);
        const bool dceChanged = eliminateUnreachableCode(function);
        if (!peepholeChanged && !dceChanged) {
            break;
        }
    }
}

}

inline void optimizeCompiledProgram(CompiledProgram* program) {
    if (program == nullptr) {
        return;
    }

    for (size_t index = 0; index < program->functions.size(); ++index) {
        bytecode_optimizer::optimizeFunction(program->functions[index]);
    }
}

static bool expressionContainsPlaceholder(const Expr* expr) {
    if (dynamic_cast<const PlaceholderExpr*>(expr) != nullptr) {
        return true;
    }

    if (const ArrayExpr* array = dynamic_cast<const ArrayExpr*>(expr)) {
        for (size_t index = 0; index < array->elements.size(); ++index) {
            if (expressionContainsPlaceholder(array->elements[index].get())) {
                return true;
            }
        }
        return false;
    }

    if (const DictExpr* dict = dynamic_cast<const DictExpr*>(expr)) {
        for (size_t index = 0; index < dict->entries.size(); ++index) {
            if (expressionContainsPlaceholder(dict->entries[index].second.get())) {
                return true;
            }
        }
        return false;
    }

    if (const UnaryExpr* unary = dynamic_cast<const UnaryExpr*>(expr)) {
        return expressionContainsPlaceholder(unary->right.get());
    }

    if (const BinaryExpr* binary = dynamic_cast<const BinaryExpr*>(expr)) {
        return expressionContainsPlaceholder(binary->left.get()) ||
               expressionContainsPlaceholder(binary->right.get());
    }

    if (const AssignExpr* assign = dynamic_cast<const AssignExpr*>(expr)) {
        return expressionContainsPlaceholder(assign->target.get()) ||
               expressionContainsPlaceholder(assign->value.get());
    }

    if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
        return expressionContainsPlaceholder(member->object.get());
    }

    if (const IndexExpr* index = dynamic_cast<const IndexExpr*>(expr)) {
        return expressionContainsPlaceholder(index->container.get()) ||
               expressionContainsPlaceholder(index->index.get());
    }

    if (const CallExpr* call = dynamic_cast<const CallExpr*>(expr)) {
        if (expressionContainsPlaceholder(call->callee.get())) {
            return true;
        }
        for (size_t index = 0; index < call->args.size(); ++index) {
            if (expressionContainsPlaceholder(call->args[index].get())) {
                return true;
            }
        }
        return false;
    }

    if (const PipelineExpr* pipeline = dynamic_cast<const PipelineExpr*>(expr)) {
        if (expressionContainsPlaceholder(pipeline->source.get())) {
            return true;
        }
        for (size_t index = 0; index < pipeline->stages.size(); ++index) {
            if (expressionContainsPlaceholder(pipeline->stages[index].expr.get())) {
                return true;
            }
        }
        return false;
    }

    if (dynamic_cast<const PipeExpr*>(expr) != nullptr) {
        return false;
    }

    return false;
}

class BytecodeCompiler {
public:
    BytecodeCompiler()
        : program_(), nextSyntheticId_(0) {
    }

    CompiledProgram compileProgram(const std::vector<std::unique_ptr<Statement> >& program) {
        program_ = CompiledProgram();

        for (size_t index = 0; index < program.size(); ++index) {
            if (program[index]->type == Statement::FLOW_DEF) {
                std::shared_ptr<CompiledFunction> function =
                    compileCallableBody(program[index]->name, CompiledFunction::FLOW, program[index]->params, program[index]->body);
                program_.flows[program[index]->name] = function;
            } else if (program[index]->type == Statement::STAGE_DEF) {
                std::shared_ptr<CompiledFunction> function =
                    compileCallableBody(program[index]->name, CompiledFunction::STAGE, program[index]->params, program[index]->body);
                program_.stages[program[index]->name] = function;
            } else if (program[index]->type == Statement::TYPE_DEF) {
                compileError("type definitions are not supported in bytecode mode", program[index]->line);
            }
        }

        program_.entry = createFunction("<script>", CompiledFunction::SCRIPT, std::vector<std::string>());
        FunctionContext context(program_.entry, false);
        for (size_t index = 0; index < program.size(); ++index) {
            if (program[index]->type == Statement::FLOW_DEF ||
                program[index]->type == Statement::STAGE_DEF) {
                continue;
            }
            compileStatement(*program[index], &context);
        }

        optimizeCompiledProgram(&program_);
        return program_;
    }

private:
    struct FunctionContext {
        FunctionContext(const std::shared_ptr<CompiledFunction>& compiledFunction, bool placeholderAllowed)
            : function(compiledFunction), allowPlaceholder(placeholderAllowed), scopeDepth(0), loopDepth(0) {
        }

        std::shared_ptr<CompiledFunction> function;
        bool allowPlaceholder;
        int scopeDepth;
        int loopDepth;
    };

    std::shared_ptr<CompiledFunction> createFunction(const std::string& name,
                                                     CompiledFunction::Kind kind,
                                                     const std::vector<std::string>& params) {
        std::shared_ptr<CompiledFunction> function(new CompiledFunction());
        function->kind = kind;
        function->name = name;
        function->params = params;
        function->syntheticId = ++nextSyntheticId_;
        program_.functions.push_back(function);
        return function;
    }

    std::string syntheticName(const std::string& prefix) {
        std::ostringstream buffer;
        buffer << prefix << "#" << (nextSyntheticId_ + 1);
        return buffer.str();
    }

    int emit(FunctionContext* context, const BytecodeInstruction& instruction) {
        context->function->code.push_back(instruction);
        return static_cast<int>(context->function->code.size()) - 1;
    }

    int emitJump(FunctionContext* context, BytecodeInstruction::Op op) {
        BytecodeInstruction instruction(op);
        instruction.operand = -1;
        return emit(context, instruction);
    }

    void patchJump(FunctionContext* context, int location, int target) {
        context->function->code[static_cast<size_t>(location)].operand = target;
    }

    [[noreturn]] void compileError(const std::string& message, int line = -1) const {
        if (line > 0) {
            throw std::runtime_error("Compile Error(line " + std::to_string(line) + "): " + message);
        }
        throw std::runtime_error("Compile Error: " + message);
    }

    void compileScopedBlock(const std::vector<std::unique_ptr<Statement> >& body, FunctionContext* context) {
        emit(context, BytecodeInstruction(BytecodeInstruction::PUSH_SCOPE));
        ++context->scopeDepth;
        for (size_t index = 0; index < body.size(); ++index) {
            compileStatement(*body[index], context);
        }
        emit(context, BytecodeInstruction(BytecodeInstruction::POP_SCOPE));
        --context->scopeDepth;
    }

    std::shared_ptr<CompiledFunction> compileCallableBody(const std::string& name,
                                                          CompiledFunction::Kind kind,
                                                          const std::vector<std::string>& params,
                                                          const std::vector<std::unique_ptr<Statement> >& body) {
        std::shared_ptr<CompiledFunction> function = createFunction(name, kind, params);
        FunctionContext context(function, false);
        for (size_t index = 0; index < body.size(); ++index) {
            compileStatement(*body[index], &context);
        }
        return function;
    }

    std::shared_ptr<CompiledFunction> compilePipeExpr(const PipeExpr& pipe) {
        return compileCallableBody(syntheticName("pipe"), CompiledFunction::PIPE, pipe.params, pipe.body);
    }

    std::shared_ptr<CompiledFunction> compileInlineExpression(const Expr* expr) {
        std::shared_ptr<CompiledFunction> function =
            createFunction(syntheticName("pipe_expr"), CompiledFunction::INLINE_EXPR, std::vector<std::string>(1, "__pipe_input__"));
        FunctionContext context(function, true);
        compileExpr(expr, &context);
        emit(&context, BytecodeInstruction(BytecodeInstruction::RETURN_VALUE));
        return function;
    }

    bool canCompileExpr(const Expr* expr, bool allowPlaceholder) const {
        if (dynamic_cast<const LiteralExpr*>(expr) != nullptr ||
            dynamic_cast<const VariableExpr*>(expr) != nullptr ||
            dynamic_cast<const IdentifierExpr*>(expr) != nullptr) {
            return true;
        }

        if (dynamic_cast<const PlaceholderExpr*>(expr) != nullptr) {
            return allowPlaceholder;
        }

        if (const ArrayExpr* array = dynamic_cast<const ArrayExpr*>(expr)) {
            for (size_t index = 0; index < array->elements.size(); ++index) {
                if (!canCompileExpr(array->elements[index].get(), allowPlaceholder)) {
                    return false;
                }
            }
            return true;
        }

        if (const DictExpr* dict = dynamic_cast<const DictExpr*>(expr)) {
            for (size_t index = 0; index < dict->entries.size(); ++index) {
                if (!canCompileExpr(dict->entries[index].second.get(), allowPlaceholder)) {
                    return false;
                }
            }
            return true;
        }

        if (const UnaryExpr* unary = dynamic_cast<const UnaryExpr*>(expr)) {
            return (unary->op == "!" || unary->op == "-") &&
                   canCompileExpr(unary->right.get(), allowPlaceholder);
        }

        if (const BinaryExpr* binary = dynamic_cast<const BinaryExpr*>(expr)) {
            return canCompileExpr(binary->left.get(), allowPlaceholder) &&
                   canCompileExpr(binary->right.get(), allowPlaceholder);
        }

        if (const AssignExpr* assign = dynamic_cast<const AssignExpr*>(expr)) {
            return dynamic_cast<const VariableExpr*>(assign->target.get()) != nullptr &&
                   canCompileExpr(assign->value.get(), allowPlaceholder);
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
            return canCompileExpr(member->object.get(), allowPlaceholder);
        }

        if (const IndexExpr* index = dynamic_cast<const IndexExpr*>(expr)) {
            return canCompileExpr(index->container.get(), allowPlaceholder) &&
                   canCompileExpr(index->index.get(), allowPlaceholder);
        }

        if (const CallExpr* call = dynamic_cast<const CallExpr*>(expr)) {
            if (!canCompileExpr(call->callee.get(), allowPlaceholder)) {
                return false;
            }
            for (size_t index = 0; index < call->args.size(); ++index) {
                if (!canCompileExpr(call->args[index].get(), allowPlaceholder)) {
                    return false;
                }
            }
            return true;
        }

        if (const PipelineExpr* pipeline = dynamic_cast<const PipelineExpr*>(expr)) {
            if (!canCompileExpr(pipeline->source.get(), false)) {
                return false;
            }
            for (size_t index = 0; index < pipeline->stages.size(); ++index) {
                const bool stageHasPlaceholder = expressionContainsPlaceholder(pipeline->stages[index].expr.get());
                if (!canCompileExpr(pipeline->stages[index].expr.get(), stageHasPlaceholder)) {
                    return false;
                }
            }
            return true;
        }

        if (dynamic_cast<const PipeExpr*>(expr) != nullptr) {
            return true;
        }

        return false;
    }

    void compileStatement(const Statement& statement, FunctionContext* context) {
        switch (statement.type) {
            case Statement::EXPR:
                compileExpr(statement.expr.get(), context);
                emit(context, BytecodeInstruction(BytecodeInstruction::POP));
                return;
            case Statement::WHEN: {
                compileExpr(statement.expr.get(), context);
                const int jumpToElse = emitJump(context, BytecodeInstruction::JUMP_IF_FALSE);
                compileScopedBlock(statement.body, context);
                const int jumpToEnd = emitJump(context, BytecodeInstruction::JUMP);
                patchJump(context, jumpToElse, static_cast<int>(context->function->code.size()));
                compileScopedBlock(statement.elseBody, context);
                patchJump(context, jumpToEnd, static_cast<int>(context->function->code.size()));
                return;
            }
            case Statement::WHILE_LOOP: {
                const int loopStart = static_cast<int>(context->function->code.size());
                compileExpr(statement.expr.get(), context);
                const int jumpToEnd = emitJump(context, BytecodeInstruction::JUMP_IF_FALSE);

                BytecodeInstruction enter(BytecodeInstruction::LOOP_ENTER);
                const int loopEnter = emit(context, enter);

                ++context->loopDepth;
                compileScopedBlock(statement.body, context);
                --context->loopDepth;

                const int continueTarget = static_cast<int>(context->function->code.size());
                emit(context, BytecodeInstruction(BytecodeInstruction::LOOP_EXIT));

                BytecodeInstruction loopJump(BytecodeInstruction::JUMP);
                loopJump.operand = loopStart;
                emit(context, loopJump);

                const int breakTarget = static_cast<int>(context->function->code.size());
                emit(context, BytecodeInstruction(BytecodeInstruction::LOOP_EXIT));

                const int endTarget = static_cast<int>(context->function->code.size());
                patchJump(context, jumpToEnd, endTarget);
                context->function->code[static_cast<size_t>(loopEnter)].operand = breakTarget;
                context->function->code[static_cast<size_t>(loopEnter)].operand2 = continueTarget;
                return;
            }
            case Statement::RETURN: {
                if (statement.expr.get() == nullptr) {
                    BytecodeInstruction nilValue(BytecodeInstruction::PUSH_CONST);
                    nilValue.constant = Value();
                    emit(context, nilValue);
                } else {
                    compileExpr(statement.expr.get(), context);
                }
                emit(context, BytecodeInstruction(BytecodeInstruction::RETURN_VALUE));
                return;
            }
            case Statement::BREAK_LOOP:
                if (context->loopDepth > 0) {
                    emit(context, BytecodeInstruction(BytecodeInstruction::LOOP_BREAK));
                } else {
                    compileError("break can only be used inside while", statement.line);
                }
                return;
            case Statement::CONTINUE_LOOP:
                if (context->loopDepth > 0) {
                    emit(context, BytecodeInstruction(BytecodeInstruction::LOOP_CONTINUE));
                } else {
                    compileError("continue can only be used inside while", statement.line);
                }
                return;
            case Statement::FLOW_DEF:
            case Statement::STAGE_DEF:
                compileError("nested flow or stage definitions are not supported in bytecode mode", statement.line);
            case Statement::TYPE_DEF:
                compileError("type definitions are not supported in bytecode mode", statement.line);
            case Statement::MATCH:
                compileError("match is not supported in bytecode mode", statement.line);
            case Statement::FOR_LOOP:
                compileError("for loops are not supported in bytecode mode", statement.line);
            case Statement::DEFER:
                compileError("defer is not supported in bytecode mode", statement.line);
            default:
                compileError("unsupported statement in bytecode mode", statement.line);
        }
    }

    void compileCall(const CallExpr& call, FunctionContext* context) {
        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(call.callee.get())) {
            if (identifier->name == "read_file") {
                compileError("read_file is disabled in sandbox mode");
            }
            for (size_t index = 0; index < call.args.size(); ++index) {
                compileExpr(call.args[index].get(), context);
            }
            BytecodeInstruction instruction(BytecodeInstruction::CALL_NAMED);
            instruction.text = identifier->name;
            instruction.operand = static_cast<int>(call.args.size());
            emit(context, instruction);
            return;
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(call.callee.get())) {
            compileExpr(member->object.get(), context);
            for (size_t index = 0; index < call.args.size(); ++index) {
                compileExpr(call.args[index].get(), context);
            }
            BytecodeInstruction instruction(BytecodeInstruction::CALL_MEMBER);
            instruction.text = member->memberName;
            instruction.operand = static_cast<int>(call.args.size());
            emit(context, instruction);
            return;
        }

        compileExpr(call.callee.get(), context);
        for (size_t index = 0; index < call.args.size(); ++index) {
            compileExpr(call.args[index].get(), context);
        }
        BytecodeInstruction instruction(BytecodeInstruction::CALL_VALUE);
        instruction.operand = static_cast<int>(call.args.size());
        emit(context, instruction);
    }

    void compilePipelineStage(const PipelineExpr::Stage& stage, FunctionContext* context) {
        if (expressionContainsPlaceholder(stage.expr.get())) {
            if (canCompileExpr(stage.expr.get(), true)) {
                BytecodeInstruction instruction(BytecodeInstruction::PIPE_INLINE_EXPR);
                instruction.function = compileInlineExpression(stage.expr.get());
                instruction.flag = stage.op == "|?";
                emit(context, instruction);
            } else {
                compileError("pipeline placeholder stage uses an unsupported expression");
            }
            return;
        }

        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(stage.expr.get())) {
            if (identifier->name == "read_file") {
                compileError("read_file is disabled in sandbox mode");
            }
            BytecodeInstruction instruction(BytecodeInstruction::PIPE_NAMED);
            instruction.text = identifier->name;
            instruction.operand = 0;
            instruction.flag = stage.op == "|?";
            emit(context, instruction);
            return;
        }

        if (const CallExpr* call = dynamic_cast<const CallExpr*>(stage.expr.get())) {
            if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(call->callee.get())) {
                if (identifier->name == "read_file") {
                    compileError("read_file is disabled in sandbox mode");
                }
                for (size_t index = 0; index < call->args.size(); ++index) {
                    compileExpr(call->args[index].get(), context);
                }
                BytecodeInstruction instruction(BytecodeInstruction::PIPE_NAMED);
                instruction.text = identifier->name;
                instruction.operand = static_cast<int>(call->args.size());
                instruction.flag = stage.op == "|?";
                emit(context, instruction);
                return;
            }

            compileExpr(call->callee.get(), context);
            for (size_t index = 0; index < call->args.size(); ++index) {
                compileExpr(call->args[index].get(), context);
            }
            BytecodeInstruction instruction(BytecodeInstruction::PIPE_VALUE);
            instruction.operand = static_cast<int>(call->args.size());
            instruction.flag = stage.op == "|?";
            emit(context, instruction);
            return;
        }

        compileExpr(stage.expr.get(), context);
        BytecodeInstruction instruction(BytecodeInstruction::PIPE_VALUE);
        instruction.operand = 0;
        instruction.flag = stage.op == "|?";
        emit(context, instruction);
    }

    void compileExpr(const Expr* expr, FunctionContext* context) {
        if (const LiteralExpr* literal = dynamic_cast<const LiteralExpr*>(expr)) {
            BytecodeInstruction instruction(BytecodeInstruction::PUSH_CONST);
            instruction.constant = literal->value;
            emit(context, instruction);
            return;
        }

        if (const VariableExpr* variable = dynamic_cast<const VariableExpr*>(expr)) {
            BytecodeInstruction instruction(BytecodeInstruction::LOAD_VAR);
            instruction.text = variable->name;
            emit(context, instruction);
            return;
        }

        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(expr)) {
            BytecodeInstruction instruction(BytecodeInstruction::PUSH_CONST);
            instruction.constant = Value(identifier->name);
            emit(context, instruction);
            return;
        }

        if (dynamic_cast<const PlaceholderExpr*>(expr) != nullptr) {
            if (!context->allowPlaceholder) {
                compileError("placeholder '_' can only be used inside the right side of a pipeline");
            }
            BytecodeInstruction instruction(BytecodeInstruction::LOAD_VAR);
            instruction.text = "__pipe_input__";
            emit(context, instruction);
            return;
        }

        if (const ArrayExpr* array = dynamic_cast<const ArrayExpr*>(expr)) {
            for (size_t index = 0; index < array->elements.size(); ++index) {
                compileExpr(array->elements[index].get(), context);
            }
            BytecodeInstruction instruction(BytecodeInstruction::MAKE_ARRAY);
            instruction.operand = static_cast<int>(array->elements.size());
            emit(context, instruction);
            return;
        }

        if (const DictExpr* dict = dynamic_cast<const DictExpr*>(expr)) {
            BytecodeInstruction instruction(BytecodeInstruction::MAKE_DICT);
            instruction.operand = static_cast<int>(dict->entries.size());
            instruction.names.reserve(dict->entries.size());
            for (size_t index = 0; index < dict->entries.size(); ++index) {
                compileExpr(dict->entries[index].second.get(), context);
                instruction.names.push_back(dict->entries[index].first);
            }
            emit(context, instruction);
            return;
        }

        if (const PipeExpr* pipe = dynamic_cast<const PipeExpr*>(expr)) {
            BytecodeInstruction instruction(BytecodeInstruction::MAKE_PIPE);
            instruction.function = compilePipeExpr(*pipe);
            emit(context, instruction);
            return;
        }

        if (const UnaryExpr* unary = dynamic_cast<const UnaryExpr*>(expr)) {
            compileExpr(unary->right.get(), context);
            if (unary->op == "!") {
                emit(context, BytecodeInstruction(BytecodeInstruction::UNARY_NOT));
                return;
            }
            if (unary->op == "-") {
                emit(context, BytecodeInstruction(BytecodeInstruction::UNARY_NEG));
                return;
            }
        }

        if (const BinaryExpr* binary = dynamic_cast<const BinaryExpr*>(expr)) {
            if (binary->op == "&&") {
                compileExpr(binary->left.get(), context);
                const int jumpToFalse = emitJump(context, BytecodeInstruction::JUMP_IF_FALSE);
                compileExpr(binary->right.get(), context);
                emit(context, BytecodeInstruction(BytecodeInstruction::TRUTHY));
                const int jumpToEnd = emitJump(context, BytecodeInstruction::JUMP);
                patchJump(context, jumpToFalse, static_cast<int>(context->function->code.size()));
                BytecodeInstruction pushFalse(BytecodeInstruction::PUSH_CONST);
                pushFalse.constant = Value(false);
                emit(context, pushFalse);
                patchJump(context, jumpToEnd, static_cast<int>(context->function->code.size()));
                return;
            }

            if (binary->op == "||") {
                compileExpr(binary->left.get(), context);
                const int jumpToTrue = emitJump(context, BytecodeInstruction::JUMP_IF_TRUE);
                compileExpr(binary->right.get(), context);
                emit(context, BytecodeInstruction(BytecodeInstruction::TRUTHY));
                const int jumpToEnd = emitJump(context, BytecodeInstruction::JUMP);
                patchJump(context, jumpToTrue, static_cast<int>(context->function->code.size()));
                BytecodeInstruction pushTrue(BytecodeInstruction::PUSH_CONST);
                pushTrue.constant = Value(true);
                emit(context, pushTrue);
                patchJump(context, jumpToEnd, static_cast<int>(context->function->code.size()));
                return;
            }

            compileExpr(binary->left.get(), context);
            compileExpr(binary->right.get(), context);
            BytecodeInstruction instruction(BytecodeInstruction::BINARY_OP);
            instruction.text = binary->op;
            emit(context, instruction);
            return;
        }

        if (const AssignExpr* assign = dynamic_cast<const AssignExpr*>(expr)) {
            if (const VariableExpr* variable = dynamic_cast<const VariableExpr*>(assign->target.get())) {
                if (assign->op == "=") {
                    compileExpr(assign->value.get(), context);
                } else {
                    BytecodeInstruction load(BytecodeInstruction::LOAD_VAR);
                    load.text = variable->name;
                    emit(context, load);
                    compileExpr(assign->value.get(), context);
                    BytecodeInstruction combine(BytecodeInstruction::ASSIGN_OP);
                    combine.text = assign->op;
                    emit(context, combine);
                }
                BytecodeInstruction store(BytecodeInstruction::STORE_VAR_KEEP);
                store.text = variable->name;
                emit(context, store);
                return;
            }
            compileError("only variable assignment is supported in bytecode mode");
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
            compileExpr(member->object.get(), context);
            BytecodeInstruction instruction(BytecodeInstruction::GET_MEMBER);
            instruction.text = member->memberName;
            emit(context, instruction);
            return;
        }

        if (const IndexExpr* index = dynamic_cast<const IndexExpr*>(expr)) {
            compileExpr(index->container.get(), context);
            compileExpr(index->index.get(), context);
            emit(context, BytecodeInstruction(BytecodeInstruction::GET_INDEX));
            return;
        }

        if (const CallExpr* call = dynamic_cast<const CallExpr*>(expr)) {
            compileCall(*call, context);
            return;
        }

        if (const PipelineExpr* pipeline = dynamic_cast<const PipelineExpr*>(expr)) {
            compileExpr(pipeline->source.get(), context);
            for (size_t index = 0; index < pipeline->stages.size(); ++index) {
                compilePipelineStage(pipeline->stages[index], context);
            }
            return;
        }

        compileError("unsupported expression in bytecode mode");
    }

    CompiledProgram program_;
    int nextSyntheticId_;
};

static std::string compiledFunctionKindName(CompiledFunction::Kind kind) {
    switch (kind) {
        case CompiledFunction::SCRIPT:
            return "script";
        case CompiledFunction::FLOW:
            return "flow";
        case CompiledFunction::STAGE:
            return "stage";
        case CompiledFunction::PIPE:
            return "pipe";
        case CompiledFunction::INLINE_EXPR:
            return "inline-expr";
        default:
            return "unknown";
    }
}

static std::string bytecodeInstructionSummary(const BytecodeInstruction& instruction) {
    std::ostringstream buffer;
    switch (instruction.op) {
        case BytecodeInstruction::PUSH_CONST:
            buffer << "PUSH_CONST " << instruction.constant.toString();
            break;
        case BytecodeInstruction::LOAD_VAR:
            buffer << "LOAD_VAR $" << instruction.text;
            break;
        case BytecodeInstruction::STORE_VAR_KEEP:
            buffer << "STORE_VAR_KEEP $" << instruction.text;
            break;
        case BytecodeInstruction::MAKE_ARRAY:
            buffer << "MAKE_ARRAY " << instruction.operand;
            break;
        case BytecodeInstruction::MAKE_DICT:
            buffer << "MAKE_DICT " << instruction.operand;
            break;
        case BytecodeInstruction::UNARY_NOT:
            buffer << "UNARY_NOT";
            break;
        case BytecodeInstruction::UNARY_NEG:
            buffer << "UNARY_NEG";
            break;
        case BytecodeInstruction::BINARY_OP:
            buffer << "BINARY_OP " << instruction.text;
            break;
        case BytecodeInstruction::ASSIGN_OP:
            buffer << "ASSIGN_OP " << instruction.text;
            break;
        case BytecodeInstruction::GET_MEMBER:
            buffer << "GET_MEMBER ." << instruction.text;
            break;
        case BytecodeInstruction::GET_INDEX:
            buffer << "GET_INDEX";
            break;
        case BytecodeInstruction::CALL_NAMED:
            buffer << "CALL_NAMED " << instruction.text << " argc=" << instruction.operand;
            break;
        case BytecodeInstruction::CALL_MEMBER:
            buffer << "CALL_MEMBER ." << instruction.text << " argc=" << instruction.operand;
            break;
        case BytecodeInstruction::CALL_VALUE:
            buffer << "CALL_VALUE argc=" << instruction.operand;
            break;
        case BytecodeInstruction::MAKE_PIPE:
            buffer << "MAKE_PIPE " << (instruction.function.get() == nullptr ? "<null>" : instruction.function->name);
            break;
        case BytecodeInstruction::PUSH_SCOPE:
            buffer << "PUSH_SCOPE";
            break;
        case BytecodeInstruction::POP_SCOPE:
            buffer << "POP_SCOPE";
            break;
        case BytecodeInstruction::JUMP:
            buffer << "JUMP " << instruction.operand;
            break;
        case BytecodeInstruction::JUMP_IF_FALSE:
            buffer << "JUMP_IF_FALSE " << instruction.operand;
            break;
        case BytecodeInstruction::JUMP_IF_TRUE:
            buffer << "JUMP_IF_TRUE " << instruction.operand;
            break;
        case BytecodeInstruction::TRUTHY:
            buffer << "TRUTHY";
            break;
        case BytecodeInstruction::PIPE_NAMED:
            buffer << (instruction.flag ? "PIPE_NAMED_SAFE " : "PIPE_NAMED ")
                   << instruction.text << " argc=" << instruction.operand;
            break;
        case BytecodeInstruction::PIPE_VALUE:
            buffer << (instruction.flag ? "PIPE_VALUE_SAFE " : "PIPE_VALUE ")
                   << "argc=" << instruction.operand;
            break;
        case BytecodeInstruction::PIPE_INLINE_EXPR:
            buffer << (instruction.flag ? "PIPE_INLINE_EXPR_SAFE " : "PIPE_INLINE_EXPR ")
                   << (instruction.function.get() == nullptr ? "<null>" : instruction.function->name);
            break;
        case BytecodeInstruction::LOOP_ENTER:
            buffer << "LOOP_ENTER break=" << instruction.operand << " continue=" << instruction.operand2;
            break;
        case BytecodeInstruction::LOOP_EXIT:
            buffer << "LOOP_EXIT";
            break;
        case BytecodeInstruction::LOOP_BREAK:
            buffer << "LOOP_BREAK";
            break;
        case BytecodeInstruction::LOOP_CONTINUE:
            buffer << "LOOP_CONTINUE";
            break;
        case BytecodeInstruction::RETURN_VALUE:
            buffer << "RETURN_VALUE";
            break;
        case BytecodeInstruction::POP:
            buffer << "POP";
            break;
        default:
            buffer << "UNKNOWN";
            break;
    }
    return buffer.str();
}

static std::string disassembleCompiledProgram(const CompiledProgram& program) {
    std::ostringstream output;
    for (size_t fnIndex = 0; fnIndex < program.functions.size(); ++fnIndex) {
        const CompiledFunction& function = *program.functions[fnIndex];
        output << "== " << compiledFunctionKindName(function.kind) << " " << function.name;
        if (!function.params.empty()) {
            output << "(";
            for (size_t index = 0; index < function.params.size(); ++index) {
                if (index > 0) {
                    output << ", ";
                }
                output << function.params[index];
            }
            output << ")";
        }
        output << " ==\n";
        for (size_t ip = 0; ip < function.code.size(); ++ip) {
            output << ip << ": " << bytecodeInstructionSummary(function.code[ip]) << "\n";
        }
        if (fnIndex + 1 < program.functions.size()) {
            output << "\n";
        }
    }
    return output.str();
}

void BytecodeVirtualMachine::executeProgram(Interpreter* runtime,
                                           const CompiledProgram& compiled) {
    for (std::unordered_map<std::string, std::shared_ptr<CompiledFunction> >::const_iterator it = compiled.flows.begin();
         it != compiled.flows.end();
         ++it) {
        runtime->compiledFlows_[it->first] = it->second;
    }

    for (std::unordered_map<std::string, std::shared_ptr<CompiledFunction> >::const_iterator it = compiled.stages.begin();
         it != compiled.stages.end();
         ++it) {
        runtime->compiledStages_[it->first] = it->second;
    }

    if (compiled.entry.get() != nullptr) {
        runFunction(runtime, *compiled.entry, std::vector<Value>(), nullptr, nullptr, nullptr, false, false);
    }
}

Value BytecodeVirtualMachine::runFlow(Interpreter* runtime,
                                      const CompiledFunction& function,
                                      const std::vector<Value>& args,
                                      const Value* self) {
    return runFunction(runtime, function, args, nullptr, self, nullptr, true, true);
}

Value BytecodeVirtualMachine::runStage(Interpreter* runtime,
                                       const CompiledFunction& function,
                                       const Value& input,
                                       const std::vector<Value>& args) {
    return runFunction(runtime, function, args, nullptr, nullptr, &input, true, true);
}

Value BytecodeVirtualMachine::runPipe(Interpreter* runtime,
                                      const CompiledFunction& function,
                                      const std::unordered_map<std::string, Value>& captured,
                                      const std::vector<Value>& args) {
    return runFunction(runtime, function, args, &captured, nullptr, nullptr, true, true);
}

Value BytecodeVirtualMachine::runInlinePipeExpr(Interpreter* runtime,
                                                const CompiledFunction& function,
                                                const Value& input) {
    return runFunction(runtime,
                       function,
                       std::vector<Value>(1, input),
                       nullptr,
                       nullptr,
                       nullptr,
                       true,
                       false);
}

Value BytecodeVirtualMachine::runFunction(Interpreter* runtime,
                                          const CompiledFunction& function,
                                          const std::vector<Value>& args,
                                          const std::unordered_map<std::string, Value>* captured,
                                          const Value* self,
                                          const Value* stageInput,
                                          bool createScope,
                                          bool incrementCallDepth) {
    struct LoopState {
        int breakTarget;
        int continueTarget;
        int scopeDepthAtEntry;
    };

    struct FrameState {
        FrameState() : stack(), loops(), scopeDepth(0) {
        }

        std::vector<Value> stack;
        std::vector<LoopState> loops;
        int scopeDepth;
    };

    const std::string functionLabel = function.kind == CompiledFunction::FLOW ? "flow '" + function.name + "'" :
                                      function.kind == CompiledFunction::STAGE ? "stage '" + function.name + "'" :
                                      function.kind == CompiledFunction::PIPE ? "pipe" :
                                      function.kind == CompiledFunction::INLINE_EXPR ? "inline expression" :
                                      "script";

    if (function.kind != CompiledFunction::SCRIPT && args.size() != function.params.size()) {
        runtime->runtimeError(functionLabel + " expected " + std::to_string(function.params.size()) +
                              " arguments but received " + std::to_string(args.size()));
    }

    const Value defaultReturn = (function.kind == CompiledFunction::STAGE && stageInput != nullptr) ? *stageInput : Value();

    FrameState frame;
    bool pushedFunctionScope = false;
    bool incrementedCallDepth = false;

    struct ScopeExit {
        explicit ScopeExit(Interpreter* interpreter, bool* activeFlag)
            : runtime(interpreter), active(activeFlag) {
        }

        ~ScopeExit() {
            if (*active) {
                runtime->popScope();
            }
        }

        Interpreter* runtime;
        bool* active;
    };

    struct CallDepthExit {
        explicit CallDepthExit(Interpreter* interpreter, bool* activeFlag)
            : runtime(interpreter), active(activeFlag) {
        }

        ~CallDepthExit() {
            if (*active) {
                --runtime->callDepth_;
            }
        }

        Interpreter* runtime;
        bool* active;
    };

    ScopeExit scopeGuard(runtime, &pushedFunctionScope);
    CallDepthExit callDepthGuard(runtime, &incrementedCallDepth);

    if (createScope) {
        runtime->pushScope();
        pushedFunctionScope = true;
        if (captured != nullptr) {
            runtime->currentScope().vars = *captured;
        }
        if (self != nullptr) {
            runtime->currentScope().vars["self"] = *self;
        }
        if (stageInput != nullptr) {
            runtime->currentScope().vars["it"] = *stageInput;
        }
        for (size_t index = 0; index < function.params.size(); ++index) {
            runtime->currentScope().vars[function.params[index]] = args[index];
        }
    }

    if (incrementCallDepth) {
        ++runtime->callDepth_;
        incrementedCallDepth = true;
    }

    const auto popValue = [&frame, runtime]() -> Value {
        if (frame.stack.empty()) {
            runtime->runtimeError("bytecode stack underflow");
        }
        const Value value = frame.stack.back();
        frame.stack.pop_back();
        return value;
    };

    const auto popArgs = [&frame, &popValue](int count) -> std::vector<Value> {
        std::vector<Value> values(static_cast<size_t>(count));
        for (int index = count - 1; index >= 0; --index) {
            values[static_cast<size_t>(index)] = popValue();
        }
        return values;
    };

    const auto popScopesTo = [&frame, runtime](int targetDepth) {
        while (frame.scopeDepth > targetDepth) {
            runtime->popScope();
            --frame.scopeDepth;
        }
    };

    const auto runSafePipe = [runtime](const Value& value,
                                       const std::function<Value(const Value&)>& action) -> Value {
        if (value.type == Value::RESULT_ERR) {
            return value;
        }

        Value current = value;
        if (current.type == Value::RESULT_OK && current.resultValue.get() != nullptr) {
            current = *current.resultValue;
        }

        try {
            Value result = action(current);
            if (result.type != Value::RESULT_OK && result.type != Value::RESULT_ERR) {
                result = Value::Ok(result);
            }
            return result;
        } catch (const std::runtime_error& error) {
            return Value::Err(Value(std::string(error.what())));
        }
    };

    size_t ip = 0;
    try {
        while (ip < function.code.size()) {
            const BytecodeInstruction& instruction = function.code[ip];
            ++ip;

            try {
                switch (instruction.op) {
                    case BytecodeInstruction::PUSH_CONST:
                        frame.stack.push_back(instruction.constant);
                        break;
                    case BytecodeInstruction::LOAD_VAR:
                        frame.stack.push_back(runtime->getVariable(instruction.text));
                        break;
                    case BytecodeInstruction::STORE_VAR_KEEP:
                        if (frame.stack.empty()) {
                            runtime->runtimeError("bytecode stack underflow");
                        }
                        runtime->storeVariable(instruction.text, frame.stack.back());
                        break;
                    case BytecodeInstruction::MAKE_ARRAY: {
                        std::vector<Value> values(static_cast<size_t>(instruction.operand));
                        for (int index = instruction.operand - 1; index >= 0; --index) {
                            values[static_cast<size_t>(index)] = popValue();
                        }
                        frame.stack.push_back(Value(values));
                        break;
                    }
                    case BytecodeInstruction::MAKE_DICT: {
                        std::unordered_map<std::string, Value> values;
                        for (int index = instruction.operand - 1; index >= 0; --index) {
                            values[instruction.names[static_cast<size_t>(index)]] = popValue();
                        }
                        frame.stack.push_back(Value(values));
                        break;
                    }
                    case BytecodeInstruction::UNARY_NOT: {
                        const Value right = popValue();
                        frame.stack.push_back(Value(!right.isTruthy()));
                        break;
                    }
                    case BytecodeInstruction::UNARY_NEG: {
                        const Value right = popValue();
                        frame.stack.push_back(Value(-runtime->requireInt(right, "unary '-'")));
                        break;
                    }
                    case BytecodeInstruction::BINARY_OP: {
                        const Value right = popValue();
                        const Value left = popValue();
                        frame.stack.push_back(runtime->applyBinaryOperator(left, instruction.text, right));
                        break;
                    }
                    case BytecodeInstruction::ASSIGN_OP: {
                        const Value right = popValue();
                        const Value left = popValue();
                        frame.stack.push_back(runtime->applyAssignmentOperator(left, instruction.text, right));
                        break;
                    }
                    case BytecodeInstruction::GET_MEMBER: {
                        const Value object = popValue();
                        frame.stack.push_back(runtime->readMember(object, instruction.text));
                        break;
                    }
                    case BytecodeInstruction::GET_INDEX: {
                        const Value key = popValue();
                        const Value container = popValue();
                        frame.stack.push_back(runtime->readIndexedValue(container, key, "index access"));
                        break;
                    }
                    case BytecodeInstruction::CALL_NAMED: {
                        const std::vector<Value> callArgs = popArgs(instruction.operand);
                        frame.stack.push_back(runtime->invokeIdentifierCall(instruction.text, callArgs));
                        break;
                    }
                    case BytecodeInstruction::CALL_MEMBER: {
                        const std::vector<Value> callArgs = popArgs(instruction.operand);
                        const Value object = popValue();
                        frame.stack.push_back(
                            runtime->invokeCallableValue(runtime->readMember(object, instruction.text), callArgs, "call"));
                        break;
                    }
                    case BytecodeInstruction::CALL_VALUE: {
                        const std::vector<Value> callArgs = popArgs(instruction.operand);
                        const Value callee = popValue();
                        frame.stack.push_back(runtime->invokeCallableValue(callee, callArgs, "call"));
                        break;
                    }
                    case BytecodeInstruction::MAKE_PIPE: {
                        if (instruction.function.get() == nullptr) {
                            runtime->runtimeError("compiled pipe has no body");
                        }
                        const std::unordered_map<std::string, Value> capturedValues = runtime->captureVisibleVars();
                        frame.stack.push_back(Value(std::shared_ptr<CallableData>(
                            new CallableData(instruction.function->params, instruction.function, capturedValues))));
                        break;
                    }
                    case BytecodeInstruction::PUSH_SCOPE:
                        runtime->pushScope();
                        ++frame.scopeDepth;
                        break;
                    case BytecodeInstruction::POP_SCOPE:
                        if (frame.scopeDepth <= 0) {
                            runtime->runtimeError("bytecode scope underflow");
                        }
                        runtime->popScope();
                        --frame.scopeDepth;
                        break;
                    case BytecodeInstruction::JUMP:
                        ip = static_cast<size_t>(instruction.operand);
                        break;
                    case BytecodeInstruction::JUMP_IF_FALSE: {
                        const Value condition = popValue();
                        if (!condition.isTruthy()) {
                            ip = static_cast<size_t>(instruction.operand);
                        }
                        break;
                    }
                    case BytecodeInstruction::JUMP_IF_TRUE: {
                        const Value condition = popValue();
                        if (condition.isTruthy()) {
                            ip = static_cast<size_t>(instruction.operand);
                        }
                        break;
                    }
                    case BytecodeInstruction::TRUTHY: {
                        const Value condition = popValue();
                        frame.stack.push_back(Value(condition.isTruthy()));
                        break;
                    }
                    case BytecodeInstruction::PIPE_NAMED: {
                        const std::vector<Value> stageArgs = popArgs(instruction.operand);
                        const Value input = popValue();
                        const std::function<Value(const Value&)> action =
                            [runtime, &instruction, &stageArgs](const Value& current) -> Value {
                                return runtime->runNamedStage(instruction.text, current, stageArgs);
                            };
                        frame.stack.push_back(instruction.flag ? runSafePipe(input, action) : action(input));
                        break;
                    }
                    case BytecodeInstruction::PIPE_VALUE: {
                        const std::vector<Value> stageArgs = popArgs(instruction.operand);
                        const Value callee = popValue();
                        const Value input = popValue();
                        const std::function<Value(const Value&)> action =
                            [runtime, &callee, &stageArgs](const Value& current) -> Value {
                                return runtime->invokePipeCallable(callee, current, stageArgs, "pipeline target");
                            };
                        frame.stack.push_back(instruction.flag ? runSafePipe(input, action) : action(input));
                        break;
                    }
                    case BytecodeInstruction::PIPE_INLINE_EXPR: {
                        if (instruction.function.get() == nullptr) {
                            runtime->runtimeError("compiled pipe expression has no body");
                        }
                        const Value input = popValue();
                        const std::function<Value(const Value&)> action =
                            [runtime, &instruction](const Value& current) -> Value {
                                return runInlinePipeExpr(runtime, *instruction.function, current);
                            };
                        frame.stack.push_back(instruction.flag ? runSafePipe(input, action) : action(input));
                        break;
                    }
                    case BytecodeInstruction::LOOP_ENTER: {
                        LoopState state;
                        state.breakTarget = instruction.operand;
                        state.continueTarget = instruction.operand2;
                        state.scopeDepthAtEntry = frame.scopeDepth;
                        frame.loops.push_back(state);
                        break;
                    }
                    case BytecodeInstruction::LOOP_EXIT:
                        if (frame.loops.empty()) {
                            runtime->runtimeError("bytecode loop underflow");
                        }
                        frame.loops.pop_back();
                        break;
                    case BytecodeInstruction::LOOP_BREAK:
                        if (frame.loops.empty()) {
                            runtime->runtimeError("break can only be used inside while or for");
                        }
                        popScopesTo(frame.loops.back().scopeDepthAtEntry);
                        ip = static_cast<size_t>(frame.loops.back().breakTarget);
                        break;
                    case BytecodeInstruction::LOOP_CONTINUE:
                        if (frame.loops.empty()) {
                            runtime->runtimeError("continue can only be used inside while or for");
                        }
                        popScopesTo(frame.loops.back().scopeDepthAtEntry);
                        ip = static_cast<size_t>(frame.loops.back().continueTarget);
                        break;
                    case BytecodeInstruction::RETURN_VALUE: {
                        const Value value = popValue();
                        if (!incrementCallDepth && function.kind != CompiledFunction::INLINE_EXPR) {
                            runtime->runtimeError("give can only be used inside flow, stream/stage, or method bodies");
                        }
                        popScopesTo(0);
                        throw ReturnSignal(value);
                    }
                    case BytecodeInstruction::POP:
                        popValue();
                        break;
                    default:
                        runtime->runtimeError("unknown bytecode instruction");
                }
            } catch (const ContinueSignal&) {
                if (frame.loops.empty()) {
                    throw;
                }
                popScopesTo(frame.loops.back().scopeDepthAtEntry);
                ip = static_cast<size_t>(frame.loops.back().continueTarget);
            } catch (const BreakSignal&) {
                if (frame.loops.empty()) {
                    throw;
                }
                popScopesTo(frame.loops.back().scopeDepthAtEntry);
                ip = static_cast<size_t>(frame.loops.back().breakTarget);
            }
        }
    } catch (const ReturnSignal& signal) {
        popScopesTo(0);
        return signal.value;
    } catch (...) {
        popScopesTo(0);
        throw;
    }

    popScopesTo(0);
    return defaultReturn;
}

}

namespace {

std::vector<std::unique_ptr<aethe::Statement> > parseProgram(const std::string& source) {
    aethe::Lexer lexer(source);
    const std::vector<aethe::Token> tokens = lexer.tokenize();
    aethe::Parser parser(tokens);
    return parser.parseProgram();
}

void executeCompiledProgram(const std::vector<std::unique_ptr<aethe::Statement> >& program,
                            aethe::Interpreter* interpreter,
                            std::string* bytecodeDump = nullptr) {
    aethe::BytecodeCompiler compiler;
    const aethe::CompiledProgram compiled = compiler.compileProgram(program);
    if (bytecodeDump != nullptr) {
        *bytecodeDump = aethe::disassembleCompiledProgram(compiled);
    }
    aethe::BytecodeVirtualMachine::executeProgram(interpreter, compiled);
}

bool isOnlyWhitespace(const std::string& text) {
    for (size_t index = 0; index < text.size(); ++index) {
        if (!std::isspace(static_cast<unsigned char>(text[index]))) {
            return false;
        }
    }
    return true;
}

constexpr int ctrlKey(int value) {
    return value & 0x1f;
}

std::string normalizeEditorLine(const std::string& line) {
    std::string normalized;
    for (size_t index = 0; index < line.size(); ++index) {
        if (line[index] == '\t') {
            normalized.append("    ");
        } else if (line[index] != '\r') {
            normalized.push_back(line[index]);
        }
    }
    return normalized;
}

std::vector<std::string> splitEditorLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    for (size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '\n') {
            lines.push_back(normalizeEditorLine(current));
            current.clear();
            continue;
        }
        current.push_back(text[index]);
    }
    lines.push_back(normalizeEditorLine(current));
    if (lines.empty()) {
        lines.push_back("");
    }
    return lines;
}

std::string normalizeEditorText(const std::string& text) {
    std::string normalized;
    for (size_t index = 0; index < text.size(); ++index) {
        const char current = text[index];
        if (current == '\r') {
            if (index + 1 < text.size() && text[index + 1] == '\n') {
                continue;
            }
            normalized.push_back('\n');
            continue;
        }
        if (current == '\t') {
            normalized.append("    ");
            continue;
        }
        normalized.push_back(current);
    }
    return normalized;
}

size_t utf8CodeUnitCount(unsigned char leadByte) {
    if ((leadByte & 0x80u) == 0) {
        return 1;
    }
    if ((leadByte & 0xe0u) == 0xc0u) {
        return 2;
    }
    if ((leadByte & 0xf0u) == 0xe0u) {
        return 3;
    }
    if ((leadByte & 0xf8u) == 0xf0u) {
        return 4;
    }
    return 1;
}

int displayWidthOfUtf8Token(const std::string& text, size_t index, size_t* nextIndex) {
    const size_t tokenBytes = std::min(utf8CodeUnitCount(static_cast<unsigned char>(text[index])), text.size() - index);
    std::mbstate_t state;
    std::memset(&state, 0, sizeof(state));
    wchar_t codepoint = 0;
    const size_t converted = std::mbrtowc(&codepoint, text.data() + index, tokenBytes, &state);
    if (converted == static_cast<size_t>(-1) || converted == static_cast<size_t>(-2)) {
        *nextIndex = index + 1;
        return 1;
    }
    if (converted == 0) {
        *nextIndex = index + 1;
        return 0;
    }

    *nextIndex = index + converted;
    const int width = ::wcwidth(codepoint);
    return width < 0 ? 1 : width;
}

std::vector<std::string> wrapTextForDisplay(const std::string& text, int width) {
    const std::string normalized = normalizeEditorLine(text);
    const int wrapWidth = std::max(1, width);
    std::vector<std::string> wrapped;
    if (normalized.empty()) {
        wrapped.push_back("");
        return wrapped;
    }

    std::string currentLine;
    int currentWidth = 0;
    size_t index = 0;
    while (index < normalized.size()) {
        size_t nextIndex = index;
        const int tokenWidth = displayWidthOfUtf8Token(normalized, index, &nextIndex);
        if (!currentLine.empty() && currentWidth + tokenWidth > wrapWidth) {
            wrapped.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0;
        }

        currentLine.append(normalized, index, nextIndex - index);
        currentWidth += std::max(0, tokenWidth);
        if (currentWidth >= wrapWidth) {
            wrapped.push_back(currentLine);
            currentLine.clear();
            currentWidth = 0;
        }
        index = nextIndex;
    }

    if (!currentLine.empty()) {
        wrapped.push_back(currentLine);
    }
    return wrapped;
}

std::string joinEditorLines(const std::vector<std::string>& lines) {
    std::ostringstream buffer;
    for (size_t index = 0; index < lines.size(); ++index) {
        if (index != 0) {
            buffer << '\n';
        }
        buffer << lines[index];
    }
    return buffer.str();
}

std::string readAllStdin() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    if (!std::cin.good() && !std::cin.eof()) {
        throw std::runtime_error("Input Error: failed while reading stdin");
    }
    return buffer.str();
}

int digitWidth(size_t value) {
    int width = 1;
    while (value >= 10) {
        value /= 10;
        ++width;
    }
    return width;
}

class TerminalIde {
public:
    TerminalIde()
        : breakSequenceBuffer_('\0'),
          originalTermios_(),
          rawModeEnabled_(false),
          quitRequested_(false),
          screenRows_(24),
          screenCols_(80),
          editorRows_(19),
          outputRows_(4),
          cursorX_(0),
          cursorY_(0),
          preferredColumn_(0),
          rowOffset_(0),
          colOffset_(0),
          statusMessage_(),
          statusMessageTime_(0),
          promptActive_(false),
          promptLabel_(),
          promptBuffer_(),
          dirty_(false),
          outputStoredBytes_(0),
          outputTruncated_(false),
          lines_(1, ""),
          outputLines_(),
          outputPartialLine_(),
          previousFrameRows_(),
          cachedScreenRows_(0),
          cachedScreenCols_(0),
          forceFullRefresh_(true) {
    }

    ~TerminalIde() {
        disableRawMode();
    }

    int run() {
        enableRawMode();
        updateWindowSize();
        setStatusMessage("Ctrl-R run | Ctrl-Q quit");

        while (!quitRequested_) {
            refreshScreen();
            processKeypress();
        }
        return 0;
    }

private:
    enum KeyCode {
        ARROW_LEFT = 1000,
        ARROW_RIGHT,
        ARROW_UP,
        ARROW_DOWN,
        DEL_KEY,
        HOME_KEY,
        END_KEY
    };

    enum HighlightType {
        HL_NORMAL = 0,
        HL_COMMENT,
        HL_STRING,
        HL_NUMBER,
        HL_KEYWORD,
        HL_BUILTIN,
        HL_VARIABLE,
        HL_OPERATOR,
        HL_PLACEHOLDER,
        HL_ERROR
    };

    size_t rowIndex(int row) const {
        return static_cast<size_t>(row);
    }

    const std::string& lineAt(int row) const {
        return lines_[rowIndex(row)];
    }

    std::string& lineAt(int row) {
        return lines_[rowIndex(row)];
    }

    const std::string& currentLine() const {
        return lineAt(cursorY_);
    }

    std::string& currentLine() {
        return lineAt(cursorY_);
    }

    void markDirty() {
        dirty_ = true;
    }

    void enableRawMode() {
        if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
            throw std::runtime_error("Terminal IDE requires an interactive TTY. Use '--stdin' instead.");
        }

        if (tcgetattr(STDIN_FILENO, &originalTermios_) == -1) {
            throw std::runtime_error("Terminal IDE failed to read terminal attributes");
        }

        struct termios raw = originalTermios_;
        raw.c_iflag &= static_cast<unsigned long>(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
        raw.c_oflag &= static_cast<unsigned long>(~(OPOST));
        raw.c_cflag |= CS8;
        raw.c_lflag &= static_cast<unsigned long>(~(ECHO | ICANON | IEXTEN | ISIG));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
            throw std::runtime_error("Terminal IDE failed to enable raw mode");
        }

        rawModeEnabled_ = true;
        std::cout << "\x1b[?1049h\x1b[H\x1b[?25l";
        std::cout.flush();
    }

    void disableRawMode() {
        if (!rawModeEnabled_) {
            return;
        }

        std::cout << "\x1b[0m\x1b[?25h\x1b[?1049l";
        std::cout.flush();
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios_);
        rawModeEnabled_ = false;
    }

    void updateWindowSize() {
        struct winsize size;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1 || size.ws_col == 0 || size.ws_row == 0) {
            int queriedRows = 0;
            int queriedCols = 0;
            if (queryScreenSize(&queriedRows, &queriedCols)) {
                screenRows_ = queriedRows;
                screenCols_ = queriedCols;
            } else {
                screenRows_ = 24;
                screenCols_ = 80;
            }
        } else {
            screenRows_ = static_cast<int>(size.ws_row);
            screenCols_ = static_cast<int>(size.ws_col);
        }

        outputRows_ = std::max(3, std::min(6, screenRows_ / 4));
        editorRows_ = std::max(1, screenRows_ - outputRows_ - 1);
    }

    bool queryScreenSize(int* rows, int* cols) const {
        if (!rawModeEnabled_) {
            return false;
        }

        const char* request = "\x1b[s\x1b[9999;9999H\x1b[6n\x1b[u";
        if (::write(STDOUT_FILENO, request, std::strlen(request)) == -1) {
            return false;
        }

        std::string response;
        while (response.size() < 32) {
            char ch = '\0';
            const ssize_t bytesRead = ::read(STDIN_FILENO, &ch, 1);
            if (bytesRead != 1) {
                return false;
            }
            response.push_back(ch);
            if (ch == 'R') {
                break;
            }
        }

        if (response.size() < 6 || response[0] != '\x1b' || response[1] != '[') {
            return false;
        }

        const size_t separator = response.find(';');
        const size_t terminator = response.find('R');
        if (separator == std::string::npos || terminator == std::string::npos || separator <= 2 || separator + 1 >= terminator) {
            return false;
        }

        *rows = std::atoi(response.substr(2, separator - 2).c_str());
        *cols = std::atoi(response.substr(separator + 1, terminator - separator - 1).c_str());
        return *rows > 0 && *cols > 0;
    }

    int readKey() {
        while (true) {
            char ch = '\0';
            const ssize_t bytesRead = ::read(STDIN_FILENO, &ch, 1);
            if (bytesRead == 1) {
                breakSequenceBuffer_ = ch;
                return decodeKey();
            }
            if (bytesRead == -1 && errno != EAGAIN) {
                throw std::runtime_error("Terminal IDE failed while reading input");
            }
        }
    }

    int decodeKey() {
        char ch = breakSequenceBuffer_;
        if (ch != '\x1b') {
            return static_cast<unsigned char>(ch);
        }

        char prefix = '\0';
        if (::read(STDIN_FILENO, &prefix, 1) != 1) {
            return '\x1b';
        }

        if (prefix != '[' && prefix != 'O') {
            return '\x1b';
        }

        std::string sequence;
        while (sequence.size() < 64) {
            char current = '\0';
            if (::read(STDIN_FILENO, &current, 1) != 1) {
                return '\x1b';
            }
            sequence.push_back(current);
            if ((current >= 'A' && current <= 'Z') || (current >= 'a' && current <= 'z') || current == '~') {
                break;
            }
        }

        if (sequence.empty()) {
            return '\x1b';
        }

        if (sequence == "A") return ARROW_UP;
        if (sequence == "B") return ARROW_DOWN;
        if (sequence == "C") return ARROW_RIGHT;
        if (sequence == "D") return ARROW_LEFT;
        if (sequence == "H" || sequence == "1~" || sequence == "7~") return HOME_KEY;
        if (sequence == "F" || sequence == "4~" || sequence == "8~") return END_KEY;
        if (sequence == "3~") return DEL_KEY;
        return '\x1b';
    }

    void refreshScreen() {
        updateWindowSize();
        scroll();
        const std::vector<std::string> frameRows = buildFrameRows();

        std::ostringstream buffer;
        buffer << "\x1b[?25l";
        const bool sizeChanged = cachedScreenRows_ != screenRows_ ||
                                 cachedScreenCols_ != screenCols_ ||
                                 previousFrameRows_.size() != frameRows.size();
        if (forceFullRefresh_ || sizeChanged) {
            buffer << "\x1b[2J";
            for (size_t index = 0; index < frameRows.size(); ++index) {
                buffer << "\x1b[" << (index + 1) << ";1H" << frameRows[index];
            }
            forceFullRefresh_ = false;
        } else {
            for (size_t index = 0; index < frameRows.size(); ++index) {
                if (frameRows[index] != previousFrameRows_[index]) {
                    buffer << "\x1b[" << (index + 1) << ";1H" << frameRows[index];
                }
            }
        }

        if (promptActive_) {
            const int cursorColumn = std::min(screenCols_, static_cast<int>(promptLabel_.size() + promptBuffer_.size()) + 1);
            buffer << "\x1b[" << screenRows_ << ';' << std::max(1, cursorColumn) << 'H';
        } else {
            const int lineNumberWidth = std::max(3, digitWidth(std::max<size_t>(1, lines_.size())));
            const int screenX = lineNumberWidth + 2 + (cursorX_ - colOffset_);
            const int screenY = (cursorY_ - rowOffset_) + 1;
            buffer << "\x1b[" << std::max(1, screenY) << ';' << std::max(1, screenX) << 'H';
        }

        buffer << "\x1b[?25h";
        const std::string screen = buffer.str();
        ::write(STDOUT_FILENO, screen.c_str(), screen.size());
        previousFrameRows_ = frameRows;
        cachedScreenRows_ = screenRows_;
        cachedScreenCols_ = screenCols_;
    }

    std::vector<std::string> buildFrameRows() const {
        std::ostringstream buffer;
        drawEditorRows(buffer);
        drawOutputRows(buffer);
        drawStatusBar(buffer);
        return splitFrameRows(buffer.str());
    }

    std::vector<std::string> splitFrameRows(const std::string& frame) const {
        std::vector<std::string> rows;
        size_t start = 0;
        while (start < frame.size()) {
            const size_t lineEnd = frame.find("\r\n", start);
            if (lineEnd == std::string::npos) {
                rows.push_back(frame.substr(start));
                break;
            }
            rows.push_back(frame.substr(start, lineEnd - start));
            start = lineEnd + 2;
        }
        if (rows.empty()) {
            rows.push_back("");
        }
        return rows;
    }

    void drawEditorRows(std::ostringstream& buffer) const {
        const int lineNumberWidth = std::max(3, digitWidth(std::max<size_t>(1, lines_.size())));
        const int textWidth = std::max(1, screenCols_ - lineNumberWidth - 2);

        for (int screenRow = 0; screenRow < editorRows_; ++screenRow) {
            const int fileRow = rowOffset_ + screenRow;
            if (fileRow >= static_cast<int>(lines_.size())) {
                drawEmptyEditorRow(buffer, screenRow);
                continue;
            }

            const std::string& line = lineAt(fileRow);
            const std::vector<int> highlights = highlightLine(line);
            buffer << "\x1b[90m";
            char numberBuffer[32];
            std::snprintf(numberBuffer, sizeof(numberBuffer), "%*d ", lineNumberWidth, fileRow + 1);
            buffer << numberBuffer << "\x1b[0m";

            int activeColor = HL_NORMAL;
            const int start = std::min(static_cast<int>(line.size()), colOffset_);
            const int end = std::min(static_cast<int>(line.size()), colOffset_ + textWidth);
            for (int index = start; index < end; ++index) {
                const int nextColor = highlights[static_cast<size_t>(index)];
                if (nextColor != activeColor) {
                    buffer << colorCode(nextColor);
                    activeColor = nextColor;
                }
                buffer << line[static_cast<size_t>(index)];
            }
            if (activeColor != HL_NORMAL) {
                buffer << "\x1b[0m";
            }
            buffer << "\x1b[K\r\n";
        }
    }

    void drawEmptyEditorRow(std::ostringstream& buffer, int screenRow) const {
        (void)screenRow;
        const int lineNumberWidth = std::max(3, digitWidth(std::max<size_t>(1, lines_.size())));
        buffer << std::string(static_cast<size_t>(lineNumberWidth + 1), ' ') << "~";
        buffer << "\x1b[K\r\n";
    }

    void drawStatusBar(std::ostringstream& buffer) const {
        buffer << "\x1b[7m";
        std::string left = "Aethe IDE";
        if (dirty_) {
            left += " * ";
        }
        if (promptActive_) {
            left += " | " + promptLabel_ + promptBuffer_;
        } else {
            const bool showMessage = !statusMessage_.empty() && std::time(nullptr) - statusMessageTime_ < 6;
            left += showMessage
                        ? " | " + statusMessage_
                        : " | Ctrl-R run | Ctrl-Q quit";
        }
        std::string right = "Ln " + std::to_string(cursorY_ + 1) + ", Col " + std::to_string(cursorX_ + 1);
        if (static_cast<int>(left.size() + right.size()) > screenCols_) {
            left = left.substr(0, static_cast<size_t>(std::max(0, screenCols_ - static_cast<int>(right.size()))));
        }
        if (static_cast<int>(left.size() + right.size()) < screenCols_) {
            left.append(static_cast<size_t>(screenCols_ - static_cast<int>(left.size()) - static_cast<int>(right.size())), ' ');
        }
        buffer << left << right;
        buffer << "\x1b[0m";
    }

    void drawOutputRows(std::ostringstream& buffer) const {
        const int bodyRows = std::max(1, outputRows_ - 1);
        buffer << "\x1b[7m";
        std::string title = " Output ";
        if (static_cast<int>(title.size()) < screenCols_) {
            title.append(static_cast<size_t>(screenCols_ - static_cast<int>(title.size())), ' ');
        }
        buffer << title.substr(0, static_cast<size_t>(screenCols_)) << "\x1b[0m\r\n";

        const std::vector<std::string> snapshot = wrappedOutputSnapshot(screenCols_);
        const int start = std::max(0, static_cast<int>(snapshot.size()) - bodyRows);
        static const char* kOutputPlaceholderLines[] = {
            "代码仓库请访问：https://github.com/QianCream/Aethe",
            "代码参考参照REFERENCE.md",
            "基础教程参照TUTORIAL.md"
        };
        const int placeholderCount = static_cast<int>(sizeof(kOutputPlaceholderLines) / sizeof(kOutputPlaceholderLines[0]));
        for (int row = 0; row < bodyRows; ++row) {
            const int index = start + row;
            if (index >= static_cast<int>(snapshot.size())) {
                if (snapshot.empty() && row < placeholderCount) {
                    buffer << "\x1b[2m" << kOutputPlaceholderLines[row] << "\x1b[0m";
                }
                buffer << "\x1b[K\r\n";
                continue;
            }

            const std::string& line = snapshot[static_cast<size_t>(index)];
            if (line.find("Error") != std::string::npos) {
                buffer << colorCode(HL_ERROR);
            }
            buffer << line;
            buffer << "\x1b[0m\x1b[K\r\n";
        }
    }

    void scroll() {
        const int maxRowOffset = std::max(0, cursorY_ - editorRows_ + 1);
        if (cursorY_ < rowOffset_) {
            rowOffset_ = cursorY_;
        }
        if (cursorY_ >= rowOffset_ + editorRows_) {
            rowOffset_ = maxRowOffset;
        }
        if (rowOffset_ > maxRowOffset) {
            rowOffset_ = maxRowOffset;
        }

        const int lineNumberWidth = std::max(3, digitWidth(std::max<size_t>(1, lines_.size())));
        const int textWidth = std::max(1, screenCols_ - lineNumberWidth - 2);
        const int maxColOffset = std::max(0, cursorX_ - textWidth + 1);
        if (cursorX_ < colOffset_) {
            colOffset_ = cursorX_;
        }
        if (cursorX_ >= colOffset_ + textWidth) {
            colOffset_ = maxColOffset;
        }
        if (colOffset_ > maxColOffset) {
            colOffset_ = maxColOffset;
        }
    }

    void processKeypress() {
        const int key = readKey();
        switch (key) {
            case ctrlKey('q'):
                quitRequested_ = true;
                return;
            case ctrlKey('r'):
                runBuffer();
                return;
            case ctrlKey('a'):
            case HOME_KEY:
                cursorX_ = 0;
                preferredColumn_ = cursorX_;
                return;
            case ctrlKey('e'):
            case END_KEY:
                cursorX_ = static_cast<int>(currentLine().size());
                preferredColumn_ = cursorX_;
                return;
            case ctrlKey('p'):
            case ARROW_UP:
            case ctrlKey('n'):
            case ARROW_DOWN:
            case ARROW_LEFT:
            case ARROW_RIGHT:
                moveCursor(key);
                return;
            case ctrlKey('d'):
            case DEL_KEY:
                deleteForwardChar();
                return;
            case 127:
            case ctrlKey('h'):
                deleteChar();
                return;
            case '\r':
            case '\n':
                insertNewline();
                return;
            case '\t':
                insertText("    ");
                return;
            case '\x1b':
                return;
            default:
                break;
        }

        if (key >= 32 && key <= 126) {
            insertChar(static_cast<char>(key));
        }
    }

    void moveCursor(int key) {
        switch (key) {
            case ARROW_LEFT:
                if (cursorX_ > 0) {
                    --cursorX_;
                } else if (cursorY_ > 0) {
                    --cursorY_;
                    cursorX_ = static_cast<int>(currentLine().size());
                }
                preferredColumn_ = cursorX_;
                break;
            case ARROW_RIGHT:
                if (cursorX_ < static_cast<int>(currentLine().size())) {
                    ++cursorX_;
                } else if (cursorY_ + 1 < static_cast<int>(lines_.size())) {
                    ++cursorY_;
                    cursorX_ = 0;
                }
                preferredColumn_ = cursorX_;
                break;
            case ARROW_UP:
            case ctrlKey('p'):
                moveVertical(-1);
                break;
            case ARROW_DOWN:
            case ctrlKey('n'):
                moveVertical(1);
                break;
            default:
                break;
        }
        clampCursorX();
    }

    void moveVertical(int delta) {
        const int targetColumn = preferredColumn_;
        if (delta < 0) {
            if (cursorY_ > 0) {
                --cursorY_;
            }
        } else {
            if (cursorY_ + 1 < static_cast<int>(lines_.size())) {
                ++cursorY_;
            }
        }
        clampCursorX(targetColumn);
    }

    void clampCursorX(int targetColumn = -1) {
        const int desiredColumn = targetColumn >= 0 ? targetColumn : cursorX_;
        const int lineLength = static_cast<int>(currentLine().size());
        cursorX_ = std::min(desiredColumn, lineLength);
    }

    void insertChar(char ch) {
        currentLine().insert(static_cast<size_t>(cursorX_), 1, ch);
        ++cursorX_;
        preferredColumn_ = cursorX_;
        markDirty();
    }

    void insertText(const std::string& text) {
        const std::string normalized = normalizeEditorText(text);
        if (normalized.empty()) {
            return;
        }

        const std::vector<std::string> pastedLines = splitEditorLines(normalized);
        const std::string head = currentLine().substr(0, static_cast<size_t>(cursorX_));
        const std::string tail = currentLine().substr(static_cast<size_t>(cursorX_));
        currentLine() = head + pastedLines[0];

        if (pastedLines.size() == 1) {
            currentLine() += tail;
            cursorX_ = static_cast<int>(head.size() + pastedLines[0].size());
            preferredColumn_ = cursorX_;
            markDirty();
            return;
        }

        size_t insertRow = static_cast<size_t>(cursorY_ + 1);
        for (size_t index = 1; index < pastedLines.size(); ++index) {
            lines_.insert(lines_.begin() + static_cast<std::vector<std::string>::difference_type>(insertRow), pastedLines[index]);
            ++insertRow;
        }

        cursorY_ += static_cast<int>(pastedLines.size()) - 1;
        currentLine() += tail;
        cursorX_ = static_cast<int>(pastedLines.back().size());
        preferredColumn_ = cursorX_;
        markDirty();
    }

    void insertNewline() {
        const std::string tail = currentLine().substr(static_cast<size_t>(cursorX_));
        currentLine().erase(static_cast<size_t>(cursorX_));
        lines_.insert(lines_.begin() + static_cast<std::vector<std::string>::difference_type>(cursorY_ + 1), tail);
        ++cursorY_;
        cursorX_ = 0;
        preferredColumn_ = cursorX_;
        markDirty();
    }

    void deleteChar() {
        if (cursorX_ == 0 && cursorY_ == 0) {
            return;
        }
        if (cursorX_ > 0) {
            currentLine().erase(static_cast<size_t>(cursorX_ - 1), 1);
            --cursorX_;
        } else {
            cursorX_ = static_cast<int>(lineAt(cursorY_ - 1).size());
            lineAt(cursorY_ - 1) += currentLine();
            lines_.erase(lines_.begin() + static_cast<std::vector<std::string>::difference_type>(cursorY_));
            --cursorY_;
        }
        preferredColumn_ = cursorX_;
        markDirty();
    }

    void deleteForwardChar() {
        if (cursorX_ < static_cast<int>(currentLine().size())) {
            currentLine().erase(static_cast<size_t>(cursorX_), 1);
            preferredColumn_ = cursorX_;
            markDirty();
            return;
        }
        if (cursorY_ + 1 >= static_cast<int>(lines_.size())) {
            return;
        }
        currentLine() += lineAt(cursorY_ + 1);
        lines_.erase(lines_.begin() + static_cast<std::vector<std::string>::difference_type>(cursorY_ + 1));
        preferredColumn_ = cursorX_;
        markDirty();
    }

    bool prompt(const std::string& label, std::string* value) {
        promptActive_ = true;
        promptLabel_ = label;
        promptBuffer_ = value == nullptr ? std::string() : *value;

        while (true) {
            refreshScreen();
            const int key = readKey();
            if (key == '\r' || key == '\n') {
                if (value != nullptr) {
                    *value = promptBuffer_;
                }
                promptActive_ = false;
                promptLabel_.clear();
                promptBuffer_.clear();
                return true;
            }
            if (key == '\x1b') {
                promptActive_ = false;
                promptLabel_.clear();
                promptBuffer_.clear();
                setStatusMessage("Cancelled.");
                return false;
            }
            if (key == 127 || key == ctrlKey('h') || key == DEL_KEY) {
                if (!promptBuffer_.empty()) {
                    promptBuffer_.erase(promptBuffer_.size() - 1);
                }
                continue;
            }
            if (key >= 32 && key <= 126) {
                promptBuffer_.push_back(static_cast<char>(key));
            }
        }
    }

    void runBuffer() {
        clearOutput();
        const std::string source = joinEditorLines(lines_);
        if (isOnlyWhitespace(source)) {
            appendOutput("No code to run.\n");
            setStatusMessage("Buffer is empty.");
            return;
        }

        setStatusMessage("Running...");
        refreshScreen();

        try {
            std::vector<std::unique_ptr<aethe::Statement> > program = parseProgram(source);
            aethe::Interpreter interpreter(
                [this](const std::string& text) {
                    appendOutput(text);
                    refreshScreen();
                },
                [this](const std::string& promptText, std::string* line) {
                    std::string value;
                    const std::string label = promptText.empty() ? "input> " : promptText;
                    const bool ok = prompt(label, &value);
                    if (ok) {
                        *line = value;
                        appendOutput(label + value + "\n");
                        refreshScreen();
                    }
                    return ok;
                });
            executeCompiledProgram(program, &interpreter);
            if (outputLines_.empty() && outputPartialLine_.empty()) {
                appendOutput("[program finished with no output]\n");
            }
            setStatusMessage(outputTruncated_ ? "Run finished (output truncated)." : "Run finished.");
        } catch (const std::exception& error) {
            appendOutput(std::string(error.what()) + "\n");
            setStatusMessage("Run failed.");
        }
    }

    void clearOutput() {
        outputLines_.clear();
        outputPartialLine_.clear();
        outputStoredBytes_ = 0;
        outputTruncated_ = false;
    }

    void appendOutput(const std::string& text) {
        static const size_t kMaxOutputBytes = 65536;
        for (size_t index = 0; index < text.size(); ++index) {
            if (outputTruncated_) {
                return;
            }
            if (outputStoredBytes_ >= kMaxOutputBytes) {
                if (!outputPartialLine_.empty()) {
                    outputLines_.push_back(outputPartialLine_);
                    outputPartialLine_.clear();
                }
                outputLines_.push_back("[output truncated]");
                outputTruncated_ = true;
                break;
            }

            const char current = text[index];
            if (current == '\r') {
                continue;
            }
            if (current == '\n') {
                outputLines_.push_back(outputPartialLine_);
                outputPartialLine_.clear();
                continue;
            }
            outputPartialLine_.push_back(current);
            ++outputStoredBytes_;
        }
        while (outputLines_.size() > 500) {
            outputStoredBytes_ -= std::min(outputStoredBytes_, outputLines_.front().size());
            outputLines_.erase(outputLines_.begin());
        }
    }

    std::vector<std::string> outputSnapshot() const {
        std::vector<std::string> snapshot = outputLines_;
        if (!outputPartialLine_.empty()) {
            snapshot.push_back(outputPartialLine_);
        }
        return snapshot;
    }

    std::vector<std::string> wrappedOutputSnapshot(int width) const {
        std::vector<std::string> wrapped;
        const std::vector<std::string> snapshot = outputSnapshot();
        for (size_t index = 0; index < snapshot.size(); ++index) {
            const std::vector<std::string> lineWraps = wrapTextForDisplay(snapshot[index], width);
            wrapped.insert(wrapped.end(), lineWraps.begin(), lineWraps.end());
        }
        return wrapped;
    }

    std::vector<int> highlightLine(const std::string& line) const {
        std::vector<int> highlight(line.size(), HL_NORMAL);
        size_t index = 0;
        while (index < line.size()) {
            if (index + 1 < line.size() && line[index] == '/' && line[index + 1] == '/') {
                for (size_t tail = index; tail < line.size(); ++tail) {
                    highlight[tail] = HL_COMMENT;
                }
                break;
            }

            if (line[index] == '"') {
                highlight[index] = HL_STRING;
                ++index;
                while (index < line.size()) {
                    highlight[index] = HL_STRING;
                    if (line[index] == '\\' && index + 1 < line.size()) {
                        highlight[index + 1] = HL_STRING;
                        index += 2;
                        continue;
                    }
                    if (line[index] == '"') {
                        ++index;
                        break;
                    }
                    ++index;
                }
                continue;
            }

            if (line[index] == '$') {
                highlight[index] = HL_VARIABLE;
                size_t tail = index + 1;
                while (tail < line.size() &&
                       (std::isalnum(static_cast<unsigned char>(line[tail])) || line[tail] == '_')) {
                    highlight[tail] = HL_VARIABLE;
                    ++tail;
                }
                index = tail;
                continue;
            }

            if (index + 1 < line.size() && line[index] == '|' && (line[index + 1] == '>' || line[index + 1] == '?')) {
                highlight[index] = HL_OPERATOR;
                highlight[index + 1] = HL_OPERATOR;
                index += 2;
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(line[index])) &&
                (index == 0 || !std::isalnum(static_cast<unsigned char>(line[index - 1])))) {
                size_t tail = index;
                while (tail < line.size() && std::isdigit(static_cast<unsigned char>(line[tail]))) {
                    highlight[tail] = HL_NUMBER;
                    ++tail;
                }
                index = tail;
                continue;
            }

            if (line[index] == '_') {
                highlight[index] = HL_PLACEHOLDER;
                ++index;
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(line[index])) || line[index] == '_') {
                size_t tail = index;
                while (tail < line.size() &&
                       (std::isalnum(static_cast<unsigned char>(line[tail])) || line[tail] == '_')) {
                    ++tail;
                }
                const std::string token = line.substr(index, tail - index);
                const int kind = classifyIdentifier(token);
                for (size_t mark = index; mark < tail; ++mark) {
                    highlight[mark] = kind;
                }
                index = tail;
                continue;
            }

            ++index;
        }
        return highlight;
    }

    int classifyIdentifier(const std::string& token) const {
        static const char* keywords[] = {
            "fn", "flow", "stage", "stream", "type", "let", "return", "give", "else", "while", "for", "in",
            "break", "continue", "defer", "when", "match", "case", "pipe", "true", "false", "nil"
        };
        static const char* builtins[] = {
            "range", "str", "int", "bool", "type_of", "input",
            "Ok", "Err", "is_ok", "is_err", "unwrap",
            "bind", "chain", "branch", "guard",
            "emit", "print", "show", "into", "store", "drop",
            "add", "sub", "mul", "div", "mod", "min", "max",
            "eq", "ne", "gt", "gte", "lt", "lte", "not", "default", "choose",
            "trim", "upper", "to_upper", "lower", "to_lower", "substring", "concat",
            "split", "contains", "has", "starts_with", "ends_with", "replace", "join",
            "slice", "reverse", "index_of", "repeat", "size", "count", "head", "last",
            "sum", "sum_by", "flatten", "take", "skip", "distinct", "distinct_by",
            "sort", "sort_desc", "sort_by", "sort_desc_by", "chunk", "zip",
            "tap", "map", "pmap", "flat_map", "filter", "find", "each", "all", "any", "reduce", "scan",
            "group_by", "index_by", "count_by", "pluck", "where",
            "append", "push", "prepend", "get", "field", "at", "set", "update", "insert", "remove",
            "keys", "values", "entries", "pick", "omit", "merge", "rename",
            "evolve", "derive", "window"
        };

        for (size_t index = 0; index < sizeof(keywords) / sizeof(keywords[0]); ++index) {
            if (token == keywords[index]) {
                return HL_KEYWORD;
            }
        }
        for (size_t index = 0; index < sizeof(builtins) / sizeof(builtins[0]); ++index) {
            if (token == builtins[index]) {
                return HL_BUILTIN;
            }
        }
        return HL_NORMAL;
    }

    const char* colorCode(int highlight) const {
        switch (highlight) {
            case HL_COMMENT:
                return "\x1b[90m";
            case HL_STRING:
                return "\x1b[32m";
            case HL_NUMBER:
                return "\x1b[33m";
            case HL_KEYWORD:
                return "\x1b[36m";
            case HL_BUILTIN:
                return "\x1b[35m";
            case HL_VARIABLE:
                return "\x1b[34m";
            case HL_OPERATOR:
                return "\x1b[31m";
            case HL_PLACEHOLDER:
                return "\x1b[93m";
            case HL_ERROR:
                return "\x1b[31m";
            default:
                return "\x1b[0m";
        }
    }

    void setStatusMessage(const std::string& message) {
        statusMessage_ = message;
        statusMessageTime_ = std::time(nullptr);
    }

    char breakSequenceBuffer_;
    struct termios originalTermios_;
    bool rawModeEnabled_;
    bool quitRequested_;
    int screenRows_;
    int screenCols_;
    int editorRows_;
    int outputRows_;
    int cursorX_;
    int cursorY_;
    int preferredColumn_;
    int rowOffset_;
    int colOffset_;
    std::string statusMessage_;
    std::time_t statusMessageTime_;
    bool promptActive_;
    std::string promptLabel_;
    std::string promptBuffer_;
    bool dirty_;
    size_t outputStoredBytes_;
    bool outputTruncated_;
    std::vector<std::string> lines_;
    std::vector<std::string> outputLines_;
    std::string outputPartialLine_;
    std::vector<std::string> previousFrameRows_;
    int cachedScreenRows_;
    int cachedScreenCols_;
    bool forceFullRefresh_;
};

int runSourceOnce(const std::string& source, bool mentionStdinRequirement = false) {
    try {
        if (isOnlyWhitespace(source)) {
            std::cerr << (mentionStdinRequirement
                              ? "Input Error: no source provided on stdin; positional arguments are ignored in sandbox mode\n"
                              : "Input Error: no source provided\n");
            return 1;
        }
        std::vector<std::unique_ptr<aethe::Statement> > program = parseProgram(source);
        aethe::Interpreter interpreter;
        executeCompiledProgram(program, &interpreter);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

int runStdinOnce(bool mentionStdinRequirement = false) {
    try {
        return runSourceOnce(readAllStdin(), mentionStdinRequirement);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

int dumpSourceBytecode(const std::string& source) {
    try {
        if (isOnlyWhitespace(source)) {
            std::cerr << "Input Error: no source provided\n";
            return 1;
        }
        std::vector<std::unique_ptr<aethe::Statement> > program = parseProgram(source);
        aethe::BytecodeCompiler compiler;
        const aethe::CompiledProgram compiled = compiler.compileProgram(program);
        std::cout << aethe::disassembleCompiledProgram(compiled);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

int dumpStdinBytecode() {
    try {
        return dumpSourceBytecode(readAllStdin());
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

void printUsage(const char* executable) {
    std::cout << "Usage:\n";
    std::cout << "  " << executable << "                  Launch the terminal editor\n";
    std::cout << "  " << executable << " --stdin          Compile and run source from stdin\n";
    std::cout << "  " << executable << " --dump-bytecode  Compile stdin and print custom bytecode\n";
}

}

int main(int argc, char** argv) {
    try {
        std::setlocale(LC_CTYPE, "");
        enum CliMode {
            CLI_MODE_DEFAULT = 0,
            CLI_MODE_STDIN,
            CLI_MODE_DUMP_BYTECODE
        };

        CliMode mode = CLI_MODE_DEFAULT;
        bool sawPositionalArgument = false;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--help" || argument == "-h") {
                printUsage(argv[0]);
                return 0;
            }
            if (argument == "--stdin") {
                if (mode == CLI_MODE_DUMP_BYTECODE) {
                    std::cerr << "Usage Error: '--stdin' cannot be combined with '--dump-bytecode'\n";
                    return 1;
                }
                mode = CLI_MODE_STDIN;
                continue;
            }
            if (argument == "--dump-bytecode") {
                if (mode == CLI_MODE_STDIN) {
                    std::cerr << "Usage Error: '--dump-bytecode' cannot be combined with '--stdin'\n";
                    return 1;
                }
                mode = CLI_MODE_DUMP_BYTECODE;
                continue;
            }
            if (!argument.empty() && argument[0] == '-') {
                std::cerr << "Usage Error: unknown option '" << argument << "'\n";
                printUsage(argv[0]);
                return 1;
            }
            sawPositionalArgument = true;
        }

        if (mode == CLI_MODE_STDIN) {
            return runStdinOnce(sawPositionalArgument);
        }
        if (mode == CLI_MODE_DUMP_BYTECODE) {
            return dumpStdinBytecode();
        }

        if (!isatty(STDIN_FILENO)) {
            return runStdinOnce(sawPositionalArgument);
        }
        if (!isatty(STDOUT_FILENO)) {
            std::cerr << "Usage Error: terminal IDE requires a TTY on stdout. Pipe source through stdin instead.\n";
            return 1;
        }

        TerminalIde ide;
        return ide.run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
