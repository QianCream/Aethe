#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

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

        if ((peek() == '=' && peekNext() == '=') ||
            (peek() == '!' && peekNext() == '=') ||
            (peek() == '<' && peekNext() == '=') ||
            (peek() == '>' && peekNext() == '=') ||
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
        return Token{TokenType::END, "", tokenLine};
    }

    [[noreturn]] void lexError(const std::string& message) const {
        throw std::runtime_error("Lex Error(line " + std::to_string(line_) + "): " + message);
    }

    std::string source_;
    size_t position_;
    int line_;
};

struct ObjectData;

class Value {
public:
    enum Type {
        INT,
        BOOL,
        STRING,
        NIL,
        ARRAY,
        DICT,
        OBJECT
    };

    Value();
    explicit Value(int value);
    explicit Value(bool value);
    explicit Value(const std::string& value);
    explicit Value(const char* value);
    explicit Value(const std::vector<Value>& value);
    explicit Value(const std::unordered_map<std::string, Value>& value);
    explicit Value(const std::shared_ptr<ObjectData>& value);

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
                return objectValue.get() != 0;
            case NIL:
                return false;
        }
        return false;
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
            case NIL:
                return "nil";
        }
        return "unknown";
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
};

Value::Value()
    : type(NIL), intValue(0), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue() {
}

Value::Value(int value)
    : type(INT), intValue(value), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue() {
}

Value::Value(bool value)
    : type(BOOL), intValue(0), boolValue(value), stringValue(), arrayValue(), dictValue(), objectValue() {
}

Value::Value(const std::string& value)
    : type(STRING), intValue(0), boolValue(false), stringValue(value), arrayValue(), dictValue(), objectValue() {
}

Value::Value(const char* value)
    : type(STRING), intValue(0), boolValue(false), stringValue(value == 0 ? "" : value), arrayValue(), dictValue(), objectValue() {
}

Value::Value(const std::vector<Value>& value)
    : type(ARRAY),
      intValue(0),
      boolValue(false),
      stringValue(),
      arrayValue(new std::vector<Value>(value)),
      dictValue(),
      objectValue() {
}

Value::Value(const std::unordered_map<std::string, Value>& value)
    : type(DICT),
      intValue(0),
      boolValue(false),
      stringValue(),
      arrayValue(),
      dictValue(new std::unordered_map<std::string, Value>(value)),
      objectValue() {
}

Value::Value(const std::shared_ptr<ObjectData>& value)
    : type(OBJECT), intValue(0), boolValue(false), stringValue(), arrayValue(), dictValue(), objectValue(value) {
}

const std::vector<Value>& Value::asArray() const {
    static const std::vector<Value> empty;
    return arrayValue.get() == 0 ? empty : *arrayValue;
}

std::vector<Value>& Value::mutableArray() {
    if (arrayValue.get() == 0) {
        arrayValue.reset(new std::vector<Value>());
    }
    return *arrayValue;
}

const std::unordered_map<std::string, Value>& Value::asDict() const {
    static const std::unordered_map<std::string, Value> empty;
    return dictValue.get() == 0 ? empty : *dictValue;
}

std::unordered_map<std::string, Value>& Value::mutableDict() {
    if (dictValue.get() == 0) {
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
            if (objectValue.get() == 0) {
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
    }
    return "nil";
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
        case OBJECT:
            if (objectValue.get() == 0 || other.objectValue.get() == 0) {
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
    }

    return false;
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

struct MemberExpr : Expr {
    MemberExpr(std::unique_ptr<Expr> objectExpr, const std::string& member)
        : object(std::move(objectExpr)), memberName(member) {
    }

    std::unique_ptr<Expr> object;
    std::string memberName;
};

struct CallExpr : Expr {
    CallExpr(std::unique_ptr<Expr> calleeExpr, std::vector<std::unique_ptr<Expr> > arguments)
        : callee(std::move(calleeExpr)), args(std::move(arguments)) {
    }

    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr> > args;
};

struct PipelineExpr : Expr {
    explicit PipelineExpr(std::unique_ptr<Expr> sourceExpr)
        : source(std::move(sourceExpr)), stages() {
    }

    std::unique_ptr<Expr> source;
    std::vector<std::unique_ptr<Expr> > stages;
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
        : type(statementType), line(statementLine), name(), params(), expr(), body(), elseBody(), methods(), arms(), armBodies() {
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
    std::vector<std::vector<std::unique_ptr<Statement> > > armBodies;
};

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

        if (matchKeyword("stage")) {
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
        expectKeyword("in", "expected 'in' in for statement");
        std::unique_ptr<Statement> statement(new Statement(Statement::FOR_LOOP, iterator.line));
        statement->name = iterator.text;
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
        return parsePipeline();
    }

    std::unique_ptr<Expr> parsePipeline() {
        std::unique_ptr<Expr> expr = parseLogicalOr();
        while (matchSymbol("|>")) {
            std::unique_ptr<PipelineExpr> pipeline;
            PipelineExpr* existing = dynamic_cast<PipelineExpr*>(expr.get());
            if (existing != 0) {
                pipeline.reset(static_cast<PipelineExpr*>(expr.release()));
            } else {
                pipeline.reset(new PipelineExpr(std::move(expr)));
            }
            pipeline->stages.push_back(parseLogicalOr());
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
        return std::unique_ptr<Expr>();
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

    std::unique_ptr<Expr> wrapIntoStage(std::unique_ptr<Expr> value, const std::string& name) {
        std::unique_ptr<PipelineExpr> pipeline(new PipelineExpr(std::move(value)));
        std::vector<std::unique_ptr<Expr> > args;
        args.push_back(std::unique_ptr<Expr>(static_cast<Expr*>(new IdentifierExpr(name))));
        pipeline->stages.push_back(
            std::unique_ptr<Expr>(
                static_cast<Expr*>(new CallExpr(
                    std::unique_ptr<Expr>(static_cast<Expr*>(new IdentifierExpr("into"))),
                    std::move(args)))));
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

class Interpreter {
public:
    Interpreter()
        : scopes_(1), flows_(), stages_(), types_(), callDepth_(0), loopDepth_(0) {
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
                    runtimeError("give can only be used inside flow, stage, or method bodies");
                }
                throw ReturnSignal(statement.expr.get() == 0 ? Value() : evalExpr(statement.expr.get()));
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
        }

        runtimeError("unknown statement");
        return Value();
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
            if (target.equals(evalExpr(statement.arms[index].get()))) {
                return executeScopedBlock(statement.armBodies[index]);
            }
        }
        return executeScopedBlock(statement.elseBody);
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

        if (iterable.type == Value::ARRAY) {
            const std::vector<Value>& items = iterable.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                ++loopDepth_;
                try {
                    last = executeLoopIteration(statement.name, items[index], statement.body);
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
                ++loopDepth_;
                try {
                    last = executeLoopIteration(statement.name, Value(std::string(1, iterable.stringValue[index])), statement.body);
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
                ++loopDepth_;
                try {
                    last = executeLoopIteration(statement.name, Value(pairValue), statement.body);
                } catch (const ContinueSignal&) {
                } catch (const BreakSignal&) {
                    --loopDepth_;
                    break;
                }
                --loopDepth_;
            }
            return last;
        }

        runtimeError("for expects an array, string, or dict iterable");
        return Value();
    }

    Value executeLoopIteration(const std::string& name, const Value& value, const std::vector<std::unique_ptr<Statement> >& body) {
        pushScope();
        currentScope().vars[name] = value;
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
        return evalExpr(expr, 0);
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

        if (dynamic_cast<const PlaceholderExpr*>(expr) != 0) {
            if (pipeInput == 0) {
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

        if (const UnaryExpr* unary = dynamic_cast<const UnaryExpr*>(expr)) {
            return evalUnary(*unary, pipeInput);
        }

        if (const BinaryExpr* binary = dynamic_cast<const BinaryExpr*>(expr)) {
            return evalBinary(*binary, pipeInput);
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
            return evalMember(*member, pipeInput);
        }

        if (const CallExpr* call = dynamic_cast<const CallExpr*>(expr)) {
            return evalCall(*call, pipeInput);
        }

        if (const PipelineExpr* pipeline = dynamic_cast<const PipelineExpr*>(expr)) {
            return evalPipeline(*pipeline);
        }

        runtimeError("unknown expression");
        return Value();
    }

    bool containsPlaceholder(const Expr* expr) const {
        if (dynamic_cast<const PlaceholderExpr*>(expr) != 0) {
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
        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(expr)) {
            return containsPlaceholder(member->object.get());
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
                if (containsPlaceholder(pipeline->stages[index].get())) {
                    return true;
                }
            }
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
        return Value();
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

        if (binary.op == "+") {
            if (left.type == Value::STRING || right.type == Value::STRING) {
                return Value(left.toString() + right.toString());
            }
            return Value(requireInt(left, "+") + requireInt(right, "+"));
        }

        if (binary.op == "-") {
            return Value(requireInt(left, "-") - requireInt(right, "-"));
        }

        if (binary.op == "*") {
            return Value(requireInt(left, "*") * requireInt(right, "*"));
        }

        if (binary.op == "/") {
            const int divisor = requireInt(right, "/");
            if (divisor == 0) {
                runtimeError("division by zero");
            }
            return Value(requireInt(left, "/") / divisor);
        }

        if (binary.op == "%") {
            const int divisor = requireInt(right, "%");
            if (divisor == 0) {
                runtimeError("modulo by zero");
            }
            return Value(requireInt(left, "%") % divisor);
        }

        if (binary.op == "==") {
            return Value(left.equals(right));
        }

        if (binary.op == "!=") {
            return Value(!left.equals(right));
        }

        if (binary.op == ">") {
            return Value(requireInt(left, ">") > requireInt(right, ">"));
        }

        if (binary.op == ">=") {
            return Value(requireInt(left, ">=") >= requireInt(right, ">="));
        }

        if (binary.op == "<") {
            return Value(requireInt(left, "<") < requireInt(right, "<"));
        }

        if (binary.op == "<=") {
            return Value(requireInt(left, "<=") <= requireInt(right, "<="));
        }

        runtimeError("unknown binary operator '" + binary.op + "'");
        return Value();
    }

    Value evalMember(const MemberExpr& member, const Value* pipeInput) {
        const Value object = evalExpr(member.object.get(), pipeInput);
        return readMember(object, member.memberName);
    }

    Value evalCall(const CallExpr& call, const Value* pipeInput) {
        std::vector<Value> args = evalArgs(call.args, pipeInput);

        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(call.callee.get())) {
            return invokeIdentifierCall(identifier->name, args);
        }

        if (const MemberExpr* member = dynamic_cast<const MemberExpr*>(call.callee.get())) {
            Value object = evalExpr(member->object.get(), pipeInput);
            return invokeMethod(object, member->memberName, args);
        }

        runtimeError("call target must be a flow, type, or method");
        return Value();
    }

    Value evalPipeline(const PipelineExpr& pipeline) {
        Value value = evalExpr(pipeline.source.get());
        for (size_t index = 0; index < pipeline.stages.size(); ++index) {
            value = evalPipeTarget(pipeline.stages[index].get(), value);
        }
        return value;
    }

    std::vector<Value> evalArgs(const std::vector<std::unique_ptr<Expr> >& args, const Value* pipeInput = 0) {
        std::vector<Value> values;
        values.reserve(args.size());
        for (size_t index = 0; index < args.size(); ++index) {
            values.push_back(evalExpr(args[index].get(), pipeInput));
        }
        return values;
    }

    Value invokeIdentifierCall(const std::string& name, const std::vector<Value>& args) {
        if (name == "range") {
            if (args.size() == 1) {
                return makeRange(0, requireInt(args[0], "range"));
            }
            if (args.size() == 2) {
                return makeRange(requireInt(args[0], "range"), requireInt(args[1], "range"));
            }
            runtimeError("range expects one or two integer arguments");
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

        std::unordered_map<std::string, const Statement*>::const_iterator flowIt = flows_.find(name);
        if (flowIt != flows_.end()) {
            return invokeFlow(*flowIt->second, args, 0);
        }

        std::unordered_map<std::string, TypeInfo>::const_iterator typeIt = types_.find(name);
        if (typeIt != types_.end()) {
            return constructObject(name, typeIt->second, args);
        }

        if (!args.empty()) {
            std::vector<Value> stageArgs(args.begin() + 1, args.end());
            return runNamedStage(name, args.front(), stageArgs);
        }

        runtimeError("unknown callable '" + name + "'");
        return Value();
    }

    Value evalPipeTarget(const Expr* target, const Value& input) {
        if (containsPlaceholder(target)) {
            return evalExpr(target, &input);
        }

        if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(target)) {
            return runNamedStage(identifier->name, input, std::vector<Value>());
        }

        if (const CallExpr* call = dynamic_cast<const CallExpr*>(target)) {
            if (const IdentifierExpr* identifier = dynamic_cast<const IdentifierExpr*>(call->callee.get())) {
                return runNamedStage(identifier->name, input, evalArgs(call->args));
            }
        }

        runtimeError("pipeline target without '_' must be a callable or stage name");
        return Value();
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
            std::cout << input.toString() << '\n';
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
                runtimeError("give stage can only be used inside flow, stage, or method bodies");
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

        if (name == "join") {
            expectArity(args, 1, name);
            if (input.type != Value::ARRAY) {
                runtimeError("join expects an array input");
            }
            return Value(joinArray(input.asArray(), requireString(args[0], name)));
        }

        if (name == "map") {
            if (args.empty()) {
                runtimeError("map expects a callable name");
            }
            const std::string callableName = requireSymbol(args[0], name);
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (input.type != Value::ARRAY) {
                runtimeError("map expects an array input");
            }
            std::vector<Value> result;
            const std::vector<Value>& items = input.asArray();
            result.reserve(items.size());
            for (size_t index = 0; index < items.size(); ++index) {
                result.push_back(invokePipeCallable(callableName, items[index], extra));
            }
            return Value(result);
        }

        if (name == "filter") {
            if (args.empty()) {
                runtimeError("filter expects a callable name");
            }
            const std::string callableName = requireSymbol(args[0], name);
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (input.type != Value::ARRAY) {
                runtimeError("filter expects an array input");
            }
            std::vector<Value> result;
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                if (invokePipeCallable(callableName, items[index], extra).isTruthy()) {
                    result.push_back(items[index]);
                }
            }
            return Value(result);
        }

        if (name == "each") {
            if (args.empty()) {
                runtimeError("each expects a callable name");
            }
            const std::string callableName = requireSymbol(args[0], name);
            std::vector<Value> extra(args.begin() + 1, args.end());
            if (input.type != Value::ARRAY) {
                runtimeError("each expects an array input");
            }
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                invokePipeCallable(callableName, items[index], extra);
            }
            return input;
        }

        if (name == "reduce") {
            if (args.size() < 2) {
                runtimeError("reduce expects callable name and initial value");
            }
            const std::string callableName = requireSymbol(args[0], name);
            if (input.type != Value::ARRAY) {
                runtimeError("reduce expects an array input");
            }
            Value acc = args[1];
            const std::vector<Value>& items = input.asArray();
            for (size_t index = 0; index < items.size(); ++index) {
                acc = invokePipeCallable(callableName, acc, std::vector<Value>(1, items[index]));
            }
            return acc;
        }

        if (name == "size" || name == "count") {
            expectArity(args, 0, name);
            return Value(containerSize(input, name));
        }

        if (name == "append" || name == "push") {
            expectArity(args, 1, name);
            if (input.type != Value::ARRAY) {
                runtimeError("append expects an array input");
            }
            std::vector<Value> result = input.asArray();
            result.push_back(args[0]);
            return Value(result);
        }

        if (name == "prepend") {
            expectArity(args, 1, name);
            if (input.type != Value::ARRAY) {
                runtimeError("prepend expects an array input");
            }
            std::vector<Value> result;
            const std::vector<Value>& items = input.asArray();
            result.reserve(items.size() + 1);
            result.push_back(args[0]);
            result.insert(result.end(), items.begin(), items.end());
            return Value(result);
        }

        if (name == "get" || name == "field" || name == "at") {
            expectArity(args, 1, name);
            return readIndexedValue(input, args[0], name);
        }

        if (name == "set") {
            expectArity(args, 2, name);
            return writeIndexedValue(input, args[0], args[1], name);
        }

        if (name == "keys") {
            expectArity(args, 0, name);
            return readKeys(input, name);
        }

        if (name == "values") {
            expectArity(args, 0, name);
            return readValues(input, name);
        }

        if (name == "head") {
            expectArity(args, 0, name);
            return readBoundary(input, true, name);
        }

        if (name == "last") {
            expectArity(args, 0, name);
            return readBoundary(input, false, name);
        }

        if (input.type == Value::OBJECT && hasMethod(input.objectValue->typeName, name)) {
            return invokeMethod(input, name, args);
        }

        std::unordered_map<std::string, const Statement*>::const_iterator stageIt = stages_.find(name);
        if (stageIt != stages_.end()) {
            return invokeStage(*stageIt->second, input, args);
        }

        std::unordered_map<std::string, const Statement*>::const_iterator flowIt = flows_.find(name);
        if (flowIt != flows_.end()) {
            std::vector<Value> flowArgs;
            flowArgs.reserve(args.size() + 1);
            flowArgs.push_back(input);
            flowArgs.insert(flowArgs.end(), args.begin(), args.end());
            return invokeFlow(*flowIt->second, flowArgs, 0);
        }

        runtimeError("unknown stage '" + name + "'");
        return Value();
    }

    Value invokePipeCallable(const std::string& name, const Value& input, const std::vector<Value>& args) {
        return runNamedStage(name, input, args);
    }

    Value invokeFlow(const Statement& flow, const std::vector<Value>& args, const Value* self) {
        if (args.size() != flow.params.size()) {
            runtimeError("flow '" + flow.name + "' expected " + std::to_string(flow.params.size()) +
                         " arguments but received " + std::to_string(args.size()));
        }

        pushScope();
        ++callDepth_;
        if (self != 0) {
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
        if (object.type != Value::OBJECT || object.objectValue.get() == 0) {
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

    Value getVariable(const std::string& name) const {
        for (size_t index = scopes_.size(); index > 0; --index) {
            std::unordered_map<std::string, Value>::const_iterator it = scopes_[index - 1].vars.find(name);
            if (it != scopes_[index - 1].vars.end()) {
                return it->second;
            }
        }
        runtimeError("unknown variable $" + name);
        return Value();
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
        return 0;
    }

    Value makeRange(int start, int end) const {
        std::vector<Value> values;
        if (start <= end) {
            for (int value = start; value < end; ++value) {
                values.push_back(Value(value));
            }
        } else {
            for (int value = start; value > end; --value) {
                values.push_back(Value(value));
            }
        }
        return Value(values);
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
        if (value.type == Value::OBJECT && value.objectValue.get() != 0) {
            return static_cast<int>(value.objectValue->fields.size());
        }
        runtimeError("stage '" + name + "' expects string, array, dict, or object input");
        return 0;
    }

    Value readMember(const Value& object, const std::string& memberName) const {
        if (object.type == Value::DICT) {
            const std::unordered_map<std::string, Value>& entries = object.asDict();
            std::unordered_map<std::string, Value>::const_iterator it = entries.find(memberName);
            return it == entries.end() ? Value() : it->second;
        }

        if (object.type == Value::OBJECT && object.objectValue.get() != 0) {
            std::unordered_map<std::string, Value>::const_iterator it = object.objectValue->fields.find(memberName);
            return it == object.objectValue->fields.end() ? Value() : it->second;
        }

        runtimeError("member access requires dict or object input");
        return Value();
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

        if (container.type == Value::OBJECT && container.objectValue.get() != 0) {
            const std::string key = requireString(index, stageName);
            std::unordered_map<std::string, Value>::const_iterator it = container.objectValue->fields.find(key);
            return it == container.objectValue->fields.end() ? Value() : it->second;
        }

        runtimeError("stage '" + stageName + "' expects array, string, dict, or object input");
        return Value();
    }

    Value writeIndexedValue(const Value& container, const Value& index, const Value& value, const std::string& stageName) const {
        if (container.type == Value::DICT) {
            std::unordered_map<std::string, Value> result = container.asDict();
            result[requireString(index, stageName)] = value;
            return Value(result);
        }

        if (container.type == Value::OBJECT && container.objectValue.get() != 0) {
            container.objectValue->fields[requireString(index, stageName)] = value;
            return container;
        }

        runtimeError("stage '" + stageName + "' expects dict or object input");
        return Value();
    }

    Value readKeys(const Value& input, const std::string& stageName) const {
        std::vector<std::string> keys;

        if (input.type == Value::DICT) {
            const std::unordered_map<std::string, Value>& entries = input.asDict();
            for (std::unordered_map<std::string, Value>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                keys.push_back(it->first);
            }
        } else if (input.type == Value::OBJECT && input.objectValue.get() != 0) {
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

        if (input.type == Value::OBJECT && input.objectValue.get() != 0) {
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
        return Value();
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
        return Value();
    }

    [[noreturn]] void runtimeError(const std::string& message) const {
        throw std::runtime_error("Runtime Error: " + message);
    }

    std::vector<ScopeFrame> scopes_;
    std::unordered_map<std::string, const Statement*> flows_;
    std::unordered_map<std::string, const Statement*> stages_;
    std::unordered_map<std::string, TypeInfo> types_;
    int callDepth_;
    int loopDepth_;
};

}

namespace {

bool isOnlyWhitespace(const std::string& text) {
    for (size_t index = 0; index < text.size(); ++index) {
        if (!std::isspace(static_cast<unsigned char>(text[index]))) {
            return false;
        }
    }
    return true;
}

bool isCompleteChunk(const std::string& source) {
    int braceDepth = 0;
    char lastMeaningful = '\0';
    bool inString = false;
    bool escape = false;

    for (size_t index = 0; index < source.size(); ++index) {
        const char current = source[index];

        if (inString) {
            if (escape) {
                escape = false;
                continue;
            }
            if (current == '\\') {
                escape = true;
                continue;
            }
            if (current == '"') {
                inString = false;
            }
            continue;
        }

        if (current == '/' && index + 1 < source.size() && source[index + 1] == '/') {
            while (index < source.size() && source[index] != '\n') {
                ++index;
            }
            continue;
        }

        if (current == '"') {
            inString = true;
            continue;
        }

        if (current == '{') {
            ++braceDepth;
        } else if (current == '}') {
            --braceDepth;
        }

        if (!std::isspace(static_cast<unsigned char>(current))) {
            lastMeaningful = current;
        }
    }

    if (inString || braceDepth > 0) {
        return false;
    }

    return lastMeaningful == ';' || lastMeaningful == '}';
}

void runRepl() {
    aethe::Interpreter interpreter;
    std::vector<std::vector<std::unique_ptr<aethe::Statement> > > historyPrograms;
    std::string buffer;
    std::string line;

    while (true) {
        std::cout << (buffer.empty() ? ">>> " : "...> ");
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (buffer.empty() && (line == "exit" || line == "quit")) {
            break;
        }

        if (line == "run") {
            if (isOnlyWhitespace(buffer)) {
                continue;
            }

            if (!isCompleteChunk(buffer)) {
                std::cerr << "Input Error: incomplete chunk, keep entering code or close the block before 'run'\n";
                continue;
            }

            try {
                aethe::Lexer lexer(buffer);
                const std::vector<aethe::Token> tokens = lexer.tokenize();
                aethe::Parser parser(tokens);
                std::vector<std::unique_ptr<aethe::Statement> > program = parser.parseProgram();
                historyPrograms.push_back(std::move(program));
                interpreter.executeProgram(historyPrograms.back());
            } catch (const std::exception& error) {
                std::cerr << error.what() << '\n';
            }

            buffer.clear();
            continue;
        }

        if (!buffer.empty()) {
            buffer.push_back('\n');
        }
        buffer += line;

        if (isOnlyWhitespace(buffer)) {
            buffer.clear();
        }
    }
}

}

int main() {
    try {
        runRepl();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
