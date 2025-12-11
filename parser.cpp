// ============================================================================
//  parser.cpp — Recursive-descent parser (Part 2)
// ----------------------------------------------------------------------------
// MSU CSE 4714/6714 Capstone Project (Fall 2025)
// Author: Derek Willis
// ============================================================================

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "lexer.h"
#include "ast.h"
#include "debug.h"
using namespace std;

// -----------------------------------------------------------------------------
// Global symbol table definitions
// -----------------------------------------------------------------------------
map<string, variant<int, double>> symbolTable;
map<string, VarType> symbolTypes;

// -----------------------------------------------------------------------------
// One-token lookahead
// -----------------------------------------------------------------------------
bool   havePeek = false;
Token  peekTok  = 0;
string peekLex;

inline const char* tname(Token t) { return tokName(t); }

Token peek() 
{
  if (!havePeek) {
    peekTok = yylex();
    if (peekTok == 0) {
      peekTok = TOK_EOF;
      peekLex.clear();
    } else {
      peekLex = yytext ? string(yytext) : string();
    }
    dbg::line(string("peek: ") + tname(peekTok) +
              (peekLex.empty() ? "" : " [" + peekLex + "]") +
              " @ line " + to_string(yylineno));
    havePeek = true;
  }
  return peekTok;
}

Token nextTok() 
{
  Token t = peek();
  dbg::line(string("consume: ") + tname(t));
  havePeek = false;
  return t;
}

Token expect(Token want, const char* msg) 
{
  Token got = nextTok();
  if (got != want) {
    dbg::line(string("expect FAIL: wanted ") + tname(want) + ", got " + tname(got));
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected "
        << tname(want) << " — " << msg << ", got " << tname(got)
        << " [" << (yytext ? yytext : "") << "]";
    throw runtime_error(oss.str());
  }
  return got;
}

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------
unique_ptr<Block>        parseBlock();
unique_ptr<CompoundStmt> parseCompound();
unique_ptr<Statement>    parseStatement();
unique_ptr<Statement>    parseAssignStatement();
unique_ptr<Statement>    parseReadStatement();
unique_ptr<Statement>    parseWriteStatement();
unique_ptr<Statement>    parseIfStatement();
unique_ptr<Statement>    parseWhileStatement();
unique_ptr<Statement>    parseCustomStatement();
unique_ptr<ExprNode>     parseExpression();
unique_ptr<ExprNode>     parseOr();
unique_ptr<ExprNode>     parseAnd();
unique_ptr<ExprNode>     parseNot();
unique_ptr<ExprNode>     parseRelational();
unique_ptr<ExprNode>     parseAdditive();
unique_ptr<ExprNode>     parseTerm();
unique_ptr<ExprNode>     parsePower();
unique_ptr<ExprNode>     parseUnary();
unique_ptr<ExprNode>     parsePrimary();
VarDecl                  parseDeclaration();
void                     parseDeclarations(Block& block);

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
static void ensureDeclared(const string& name) {
  if (symbolTypes.find(name) == symbolTypes.end()) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): identifier '" << name
        << "' must be declared before use";
    throw runtime_error(oss.str());
  }
}

// -----------------------------------------------------------------------------
// Statements and Components
// -----------------------------------------------------------------------------
VarDecl parseDeclaration() {
  if (peek() != IDENT) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected IDENT in declaration";
    throw runtime_error(oss.str());
  }
  string name = peekLex;
  nextTok(); // consume IDENT

  expect(COLON, "':' after identifier in declaration");

  VarType type;
  Token t = peek();
  if (t == INTEGER) {
    type = VarType::Integer;
    nextTok();
  } else if (t == REAL) {
    type = VarType::Real;
    nextTok();
  } else {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected INTEGER or REAL in declaration";
    throw runtime_error(oss.str());
  }

  expect(SEMICOLON, "semicolon after declaration");

  if (symbolTypes.find(name) != symbolTypes.end()) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): duplicate declaration of '" << name << "'";
    throw runtime_error(oss.str());
  }

  symbolTypes[name] = type;
  if (type == VarType::Integer)
    symbolTable[name] = 0;
  else
    symbolTable[name] = 0.0;

  VarDecl decl;
  decl.name = name;
  decl.type = type;
  return decl;
}

void parseDeclarations(Block& block) {
  expect(VAR, "start of declaration section");

  block.declarations.push_back(parseDeclaration());
  while (peek() == IDENT) {
    block.declarations.push_back(parseDeclaration());
  }
}

unique_ptr<ExprNode> parsePrimary() {
  Token t = peek();

  if (t == INTLIT) {
    auto literal = make_unique<LiteralExpr>();
    literal->isFloat = false;
    literal->intValue = stoi(peekLex);
    nextTok();
    return literal;
  }

  if (t == FLOATLIT) {
    auto literal = make_unique<LiteralExpr>();
    literal->isFloat = true;
    literal->floatValue = stod(peekLex);
    nextTok();
    return literal;
  }

  if (t == IDENT) {
    auto ident = make_unique<IdentifierExpr>();
    ident->name = peekLex;
    ensureDeclared(ident->name);
    nextTok();
    return ident;
  }

  if (t == OPENPAREN) {
    nextTok(); // consume '('
    auto expr = parseExpression();
    expect(CLOSEPAREN, "closing parenthesis");
    return expr;
  }

  ostringstream oss;
  oss << "Parse error (line " << yylineno << "): expected INTLIT, FLOATLIT, IDENT, or '('";
  throw runtime_error(oss.str());
}

unique_ptr<ExprNode> parseUnary() {
  Token t = peek();
  bool hasUnary = false;
  UnaryOp op = UnaryOp::Negate;

  if (t == INCREMENT) {
    hasUnary = true;
    op = UnaryOp::Increment;
    nextTok();
  } else if (t == DECREMENT) {
    hasUnary = true;
    op = UnaryOp::Decrement;
    nextTok();
  } else if (t == TOK_NOT) {
    hasUnary = true;
    op = UnaryOp::LogicalNot;
    nextTok();
  } else if (t == MINUS) {
    hasUnary = true;
    op = UnaryOp::Negate;
    nextTok();
  }

  auto primary = parsePrimary();

  if (hasUnary) {
    auto node = make_unique<UnaryExpr>();
    node->op = op;
    node->operand = std::move(primary);
    return node;
  }

  return primary;
}

unique_ptr<ExprNode> parsePower() {
  auto left = parseUnary();

  if (peek() == CUSTOM_OPER) {
    nextTok(); // consume ^^
    auto right = parsePower(); // recurse for right associativity
    auto node = make_unique<BinaryExpr>();
    node->op = BinaryOp::Custom;
    node->left = std::move(left);
    node->right = std::move(right);
    return node;
  }

  return left;
}

unique_ptr<ExprNode> parseTerm() {
  auto node = parsePower();

  while (true) {
    Token t = peek();
    BinaryOp op;
    bool match = true;

    if (t == MULTIPLY) {
      op = BinaryOp::Multiply;
    } else if (t == DIVIDE) {
      op = BinaryOp::Divide;
    } else if (t == MOD) {
      op = BinaryOp::Modulo;
    } else {
      match = false;
    }

    if (!match)
      break;

    nextTok(); // consume operator
    auto rhs = parsePower();
    auto binary = make_unique<BinaryExpr>();
    binary->op = op;
    binary->left = std::move(node);
    binary->right = std::move(rhs);
    node = std::move(binary);
  }

  return node;
}

unique_ptr<ExprNode> parseAdditive() {
  auto node = parseTerm();

  while (true) {
    Token t = peek();
    BinaryOp op;
    bool match = true;

    if (t == PLUS) {
      op = BinaryOp::Add;
    } else if (t == MINUS) {
      op = BinaryOp::Subtract;
    } else {
      match = false;
    }

    if (!match)
      break;

    nextTok(); // consume operator
    auto rhs = parseTerm();
    auto binary = make_unique<BinaryExpr>();
    binary->op = op;
    binary->left = std::move(node);
    binary->right = std::move(rhs);
    node = std::move(binary);
  }

  return node;
}

unique_ptr<ExprNode> parseRelational() {
  auto left = parseAdditive();

  Token t = peek();
  if (t == LESSTHAN || t == GREATERTHAN || t == EQUALTO || t == NOTEQUALTO) {
    nextTok(); // consume relop
    auto right = parseAdditive();
    auto node = make_unique<BinaryExpr>();
    switch (t) {
      case LESSTHAN:     node->op = BinaryOp::LessThan; break;
      case GREATERTHAN:  node->op = BinaryOp::GreaterThan; break;
      case EQUALTO:      node->op = BinaryOp::EqualTo; break;
      case NOTEQUALTO:   node->op = BinaryOp::NotEqualTo; break;
      default: break;
    }
    node->left = std::move(left);
    node->right = std::move(right);
    return node;
  }

  return left;
}

unique_ptr<ExprNode> parseNot() {
  // Logical NOT handled as unary operator for precedence
  return parseRelational();
}

unique_ptr<ExprNode> parseAnd() {
  auto node = parseNot();

  while (peek() == TOK_AND) {
    nextTok(); // consume AND
    auto rhs = parseNot();
    auto binary = make_unique<BinaryExpr>();
    binary->op = BinaryOp::LogicalAnd;
    binary->left = std::move(node);
    binary->right = std::move(rhs);
    node = std::move(binary);
  }

  return node;
}

unique_ptr<ExprNode> parseOr() {
  auto node = parseAnd();

  while (peek() == TOK_OR) {
    nextTok(); // consume OR
    auto rhs = parseAnd();
    auto binary = make_unique<BinaryExpr>();
    binary->op = BinaryOp::LogicalOr;
    binary->left = std::move(node);
    binary->right = std::move(rhs);
    node = std::move(binary);
  }

  return node;
}

unique_ptr<ExprNode> parseExpression() {
  return parseOr();
}

unique_ptr<Statement> parseAssignStatement() {
  string name = peekLex;
  expect(IDENT, "identifier before ASSIGN");
  ensureDeclared(name);

  expect(ASSIGN, "assignment operator ':='");

  auto value = parseExpression();

  auto node = make_unique<AssignStmt>();
  node->name = name;
  node->expr = std::move(value);
  return node;
}

unique_ptr<Statement> parseReadStatement() {
  expect(READ, "READ keyword");
  expect(OPENPAREN, "OPENPAREN after READ");

  if (peek() != IDENT) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected IDENT inside READ(...)";
    throw runtime_error(oss.str());
  }

  string name = peekLex;
  ensureDeclared(name);
  nextTok(); // consume IDENT

  expect(CLOSEPAREN, "CLOSEPAREN after READ identifier");

  auto node = make_unique<ReadStmt>();
  node->name = name;
  return node;
}

unique_ptr<Statement> parseWriteStatement() {
  expect(WRITE, "WRITE keyword");
  expect(OPENPAREN, "OPENPAREN after WRITE");

  Token t = peek();
  auto node = make_unique<WriteStmt>();

  if (t == STRINGLIT) {
    string literal = peekLex;
    nextTok(); // consume STRINGLIT
    if (literal.size() >= 2)
      node->stringLiteral = literal.substr(1, literal.size() - 2);
    else
      node->stringLiteral.clear();
    node->hasString = true;
  } else {
    node->hasString = false;
    node->expr = parseExpression();
  }

  expect(CLOSEPAREN, "CLOSEPAREN after WRITE argument");

  return node;
}

unique_ptr<Statement> parseStatement() {
  Token t = peek();
  if (t == IDENT) return parseAssignStatement();
  if (t == READ)  return parseReadStatement();
  if (t == WRITE) return parseWriteStatement();
  if (t == IF)    return parseIfStatement();
  if (t == WHILE) return parseWhileStatement();
  if (t == CUSTOM)return parseCustomStatement();
  if (t == TOK_BEGIN) return parseCompound();

  ostringstream oss;
  oss << "Parse error (line " << yylineno << "): unexpected token " << tname(t) << " in statement";
  throw runtime_error(oss.str());
}

unique_ptr<CompoundStmt> parseCompound() {
  expect(TOK_BEGIN, "BEGIN to start compound statement");

  if (peek() == END) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): compound statement requires at least one statement";
    throw runtime_error(oss.str());
  }

  auto compound = make_unique<CompoundStmt>();
  compound->statements.push_back(parseStatement());

  while (peek() == SEMICOLON) {
    nextTok(); // consume ';'
    if (peek() == END) {
      ostringstream oss;
      oss << "Parse error (line " << yylineno << "): trailing semicolon before END is not allowed";
      throw runtime_error(oss.str());
    }
    compound->statements.push_back(parseStatement());
  }

  expect(END, "END to close compound statement");
  return compound;
}

unique_ptr<Statement> parseIfStatement() {
  expect(IF, "IF keyword");
  auto condition = parseExpression();
  expect(THEN, "THEN after IF condition");

  auto thenBranch = parseStatement();
  unique_ptr<Statement> elseBranch;
  if (peek() == ELSE) {
    nextTok(); // consume ELSE
    elseBranch = parseStatement();
  }

  auto node = make_unique<IfStmt>();
  node->condition = std::move(condition);
  node->thenBranch = std::move(thenBranch);
  node->elseBranch = std::move(elseBranch);
  return node;
}

unique_ptr<Statement> parseWhileStatement() {
  expect(WHILE, "WHILE keyword");
  auto condition = parseExpression();
  auto body = parseStatement();

  auto node = make_unique<WhileStmt>();
  node->condition = std::move(condition);
  node->body = std::move(body);
  return node;
}

unique_ptr<Statement> parseCustomStatement() {
  expect(CUSTOM, "SENIORITIS keyword");
  auto node = make_unique<CustomStmt>();
  return node;
}

unique_ptr<Block> parseBlock() {
  auto block = make_unique<Block>();

  if (peek() == VAR) {
    parseDeclarations(*block);
  }

  if (peek() != TOK_BEGIN) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected BEGIN to start block";
    throw runtime_error(oss.str());
  }

  block->compound = parseCompound();
  return block;
}

// -----------------------------------------------------------------------------
// Program → PROGRAM IDENT ';' Block EOF
// -----------------------------------------------------------------------------
unique_ptr<Program> parseProgram() {
  symbolTable.clear();
  symbolTypes.clear();

  expect(PROGRAM, "start of program");

  if (peek() != IDENT) {
    ostringstream oss;
    oss << "Parse error (line " << yylineno << "): expected IDENT after PROGRAM";
    throw runtime_error(oss.str());
  }

  string nameLex = peekLex;
  expect(IDENT, "program name");
  expect(SEMICOLON, "after program name");

  auto p = make_unique<Program>();
  p->name = nameLex;
  p->block = parseBlock();

  expect(TOK_EOF, "at end of file (no trailing tokens)");
  return p;
}
