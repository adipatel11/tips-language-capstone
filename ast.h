// =============================================================================
//   ast.h — AST for TIPS Subset (Part 2)
// =============================================================================
// MSU CSE 4714/6714 Capstone Project (Fall 2025)
// Author: Derek Willis
//
// Part 2 adds:
//   - Statement hierarchy (assign, read, write, compound)
//   - Optional declarations section with a symbol table
//   - INTEGER/REAL value handling with coercions
// =============================================================================
#pragma once
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
using namespace std;

// ----------------------------------------------------------------------------- 
// Pretty printer helpers
// -----------------------------------------------------------------------------
inline void ast_line(ostream& os, const string& prefix, bool last, const string& label) {
  os << prefix << (last ? "└── " : "├── ") << label << "\n";
}

inline string ast_child_prefix(const string& prefix, bool last) {
  return prefix + (last ? "    " : "│   ");
}

// ----------------------------------------------------------------------------- 
// Symbol table
// -----------------------------------------------------------------------------
enum class VarType { Integer, Real };

extern map<string, variant<int, double>> symbolTable;
extern map<string, VarType> symbolTypes;

inline const char* to_string(VarType type) {
  return type == VarType::Integer ? "INTEGER" : "REAL";
}

// ----------------------------------------------------------------------------- 
// Base Statement node
// -----------------------------------------------------------------------------
struct Statement {
  virtual ~Statement() = default;
  virtual void interpret(ostream& out) const = 0;
  virtual void print_tree(ostream& os, const string& prefix, bool last) const = 0;
};

// ----------------------------------------------------------------------------- 
// Expression tree
// -----------------------------------------------------------------------------
inline bool holdsInt(const variant<int, double>& value) {
  return std::holds_alternative<int>(value);
}

inline double asDouble(const variant<int, double>& value) {
  return holdsInt(value) ? static_cast<double>(std::get<int>(value)) : std::get<double>(value);
}

// Truth helper using small tolerance for floating-point comparisons
constexpr double EPSILON = 1e-5;

inline bool isTrue(double x) { return std::fabs(x) >= EPSILON; }

enum class UnaryOp { Negate, Increment, Decrement, LogicalNot };
enum class BinaryOp {
  Add,
  Subtract,
  Multiply,
  Divide,
  Modulo,
  Custom,
  LessThan,
  GreaterThan,
  EqualTo,
  NotEqualTo,
  LogicalAnd,
  LogicalOr
};

inline string unaryOpName(UnaryOp op) {
  switch (op) {
    case UnaryOp::Negate:    return "Negate";
    case UnaryOp::Increment: return "Increment";
    case UnaryOp::Decrement: return "Decrement";
    case UnaryOp::LogicalNot:return "Not";
  }
  return "?";
}

inline string binaryOpName(BinaryOp op) {
  switch (op) {
    case BinaryOp::Add:      return "Add";
    case BinaryOp::Subtract: return "Subtract";
    case BinaryOp::Multiply: return "Multiply";
    case BinaryOp::Divide:   return "Divide";
    case BinaryOp::Modulo:   return "Modulo";
    case BinaryOp::Custom:   return "CustomOper";
    case BinaryOp::LessThan: return "LessThan";
    case BinaryOp::GreaterThan:return "GreaterThan";
    case BinaryOp::EqualTo:  return "EqualTo";
    case BinaryOp::NotEqualTo:return "NotEqualTo";
    case BinaryOp::LogicalAnd:return "And";
    case BinaryOp::LogicalOr:return "Or";
  }
  return "?";
}

struct ExprNode {
  virtual ~ExprNode() = default;
  virtual variant<int, double> evaluate() const = 0;
  virtual void print_tree(ostream& os, const string& prefix, bool last) const = 0;
};

struct LiteralExpr : ExprNode {
  bool isFloat = false;
  int intValue = 0;
  double floatValue = 0.0;

  variant<int, double> evaluate() const override {
    if (isFloat)
      return floatValue;
    return intValue;
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    if (isFloat) {
      ostringstream oss;
      oss << "Float(" << floatValue << ")";
      ast_line(os, prefix, last, oss.str());
    } else {
      ast_line(os, prefix, last, "Int(" + to_string(intValue) + ")");
    }
  }
};

struct IdentifierExpr : ExprNode {
  string name;

  variant<int, double> evaluate() const override {
    auto it = symbolTable.find(name);
    if (it == symbolTable.end())
      throw runtime_error("Runtime error: use of undeclared identifier '" + name + "'");
    return it->second;
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "Ident(" + name + ")");
  }
};

struct UnaryExpr : ExprNode {
  UnaryOp op = UnaryOp::Negate;
  unique_ptr<ExprNode> operand;

  variant<int, double> evaluate() const override {
    if (!operand)
      throw runtime_error("Runtime error: missing operand for unary expression");
    auto value = operand->evaluate();
    switch (op) {
      case UnaryOp::Negate:
        if (holdsInt(value))
          return -std::get<int>(value);
        return -std::get<double>(value);
      case UnaryOp::Increment:
        if (holdsInt(value))
          return std::get<int>(value) + 1;
        return std::get<double>(value) + 1.0;
      case UnaryOp::Decrement:
        if (holdsInt(value))
          return std::get<int>(value) - 1;
        return std::get<double>(value) - 1.0;
      case UnaryOp::LogicalNot: {
        bool truth = isTrue(asDouble(value));
        return truth ? 0 : 1;
      }
    }
    throw runtime_error("Runtime error: unknown unary operator");
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "Unary(" + unaryOpName(op) + ")");
    if (operand)
      operand->print_tree(os, ast_child_prefix(prefix, last), true);
  }
};

struct BinaryExpr : ExprNode {
  BinaryOp op = BinaryOp::Add;
  unique_ptr<ExprNode> left;
  unique_ptr<ExprNode> right;

  variant<int, double> evaluate() const override {
    if (!left || !right)
      throw runtime_error("Runtime error: missing operand for binary expression");
    auto lhs = left->evaluate();
    auto rhs = right->evaluate();

    switch (op) {
      case BinaryOp::Add:
        if (holdsInt(lhs) && holdsInt(rhs))
          return std::get<int>(lhs) + std::get<int>(rhs);
        return asDouble(lhs) + asDouble(rhs);
      case BinaryOp::Subtract:
        if (holdsInt(lhs) && holdsInt(rhs))
          return std::get<int>(lhs) - std::get<int>(rhs);
        return asDouble(lhs) - asDouble(rhs);
      case BinaryOp::Multiply:
        if (holdsInt(lhs) && holdsInt(rhs))
          return std::get<int>(lhs) * std::get<int>(rhs);
        return asDouble(lhs) * asDouble(rhs);
      case BinaryOp::Divide: {
        double denom = asDouble(rhs);
        if (denom == 0.0)
          throw runtime_error("Runtime error: division by zero");
        return asDouble(lhs) / denom;
      }
      case BinaryOp::Modulo: {
        if (!holdsInt(lhs) || !holdsInt(rhs))
          throw runtime_error("Runtime error: MOD requires integer operands");
        int divisor = std::get<int>(rhs);
        if (divisor == 0)
          throw runtime_error("Runtime error: MOD by zero");
        return std::get<int>(lhs) % divisor;
      }
      case BinaryOp::Custom:
        return pow(asDouble(lhs), asDouble(rhs));
      case BinaryOp::LessThan: {
        bool result = asDouble(lhs) < asDouble(rhs);
        return result ? 1 : 0;
      }
      case BinaryOp::GreaterThan: {
        bool result = asDouble(lhs) > asDouble(rhs);
        return result ? 1 : 0;
      }
      case BinaryOp::EqualTo: {
        bool result = std::fabs(asDouble(lhs) - asDouble(rhs)) < EPSILON;
        return result ? 1 : 0;
      }
      case BinaryOp::NotEqualTo: {
        bool result = std::fabs(asDouble(lhs) - asDouble(rhs)) >= EPSILON;
        return result ? 1 : 0;
      }
      case BinaryOp::LogicalAnd: {
        bool result = isTrue(asDouble(lhs)) && isTrue(asDouble(rhs));
        return result ? 1 : 0;
      }
      case BinaryOp::LogicalOr: {
        bool result = isTrue(asDouble(lhs)) || isTrue(asDouble(rhs));
        return result ? 1 : 0;
      }
    }
    throw runtime_error("Runtime error: unknown binary operator");
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "Binary(" + binaryOpName(op) + ")");
    string child = ast_child_prefix(prefix, last);
    if (left)
      left->print_tree(os, child, false);
    if (right)
      right->print_tree(os, child, true);
  }
};

// ----------------------------------------------------------------------------- 
// Declarations
// -----------------------------------------------------------------------------
struct VarDecl {
  string name;
  VarType type = VarType::Integer;

  void print_tree(ostream& os, const string& prefix, bool last) const {
    ast_line(os, prefix, last, "VarDecl(" + name + " : " + to_string(type) + ")");
  }
};

// ----------------------------------------------------------------------------- 
// Statement implementations
// -----------------------------------------------------------------------------
struct AssignStmt : Statement {
  string name;
  unique_ptr<ExprNode> expr;

  void interpret(ostream& out) const override {
    (void)out; // unused
    auto typeIt = symbolTypes.find(name);
    if (typeIt == symbolTypes.end())
      throw runtime_error("Runtime error: assignment to undeclared identifier '" + name + "'");

    if (!expr)
      throw runtime_error("Runtime error: assignment missing expression");

    auto rhs = expr->evaluate();
    if (typeIt->second == VarType::Integer) {
      int result = std::holds_alternative<int>(rhs)
                       ? std::get<int>(rhs)
                       : static_cast<int>(std::get<double>(rhs));
      symbolTable[name] = result;
    } else {
      double result = std::holds_alternative<int>(rhs)
                          ? static_cast<double>(std::get<int>(rhs))
                          : std::get<double>(rhs);
      symbolTable[name] = result;
    }
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "Assign(" + name + ")");
    if (expr)
      expr->print_tree(os, ast_child_prefix(prefix, last), true);
  }
};

struct ReadStmt : Statement {
  string name;

  void interpret(ostream& out) const override {
    (void)out; // unused
    auto typeIt = symbolTypes.find(name);
    if (typeIt == symbolTypes.end())
      throw runtime_error("Runtime error: READ into undeclared identifier '" + name + "'");

    if (typeIt->second == VarType::Integer) {
      long long temp = 0;
      if (!(cin >> temp))
        throw runtime_error("Runtime error: expected integer input for '" + name + "'");
      symbolTable[name] = static_cast<int>(temp);
    } else {
      double temp = 0.0;
      if (!(cin >> temp))
        throw runtime_error("Runtime error: expected real input for '" + name + "'");
      symbolTable[name] = temp;
    }
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "Read(" + name + ")");
  }
};

struct WriteStmt : Statement {
  bool hasString = false;
  string stringLiteral;
  unique_ptr<ExprNode> expr;

  void interpret(ostream& out) const override {
    if (hasString) {
      out << stringLiteral << '\n';
      return;
    }

    if (!expr)
      throw runtime_error("Runtime error: WRITE missing expression");

    auto value = expr->evaluate();
    visit([&](auto&& v) { out << v; }, value);
    out << '\n';
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    if (hasString)
      ast_line(os, prefix, last, "Write(\"" + stringLiteral + "\")");
    else {
      ast_line(os, prefix, last, "WriteExpr");
      if (expr)
        expr->print_tree(os, ast_child_prefix(prefix, last), true);
    }
  }
};

struct CompoundStmt : Statement {
  vector<unique_ptr<Statement>> statements;

  void interpret(ostream& out) const override {
    for (const auto& stmt : statements)
      stmt->interpret(out);
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "Compound");
    for (size_t i = 0; i < statements.size(); ++i) {
      statements[i]->print_tree(os, ast_child_prefix(prefix, last), i + 1 == statements.size());
    }
  }
};

struct IfStmt : Statement {
  unique_ptr<ExprNode> condition;
  unique_ptr<Statement> thenBranch;
  unique_ptr<Statement> elseBranch;

  void interpret(ostream& out) const override {
    if (!condition || !thenBranch)
      throw runtime_error("Runtime error: malformed IF statement");
    bool cond = isTrue(asDouble(condition->evaluate()));
    if (cond) {
      thenBranch->interpret(out);
    } else if (elseBranch) {
      elseBranch->interpret(out);
    }
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "If");
    string child = ast_child_prefix(prefix, last);
    if (condition)
      condition->print_tree(os, child, false);
    if (thenBranch)
      thenBranch->print_tree(os, child, elseBranch ? false : true);
    if (elseBranch)
      elseBranch->print_tree(os, child, true);
  }
};

struct WhileStmt : Statement {
  unique_ptr<ExprNode> condition;
  unique_ptr<Statement> body;

  void interpret(ostream& out) const override {
    if (!condition || !body)
      throw runtime_error("Runtime error: malformed WHILE statement");
    while (isTrue(asDouble(condition->evaluate()))) {
      body->interpret(out);
    }
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "While");
    string child = ast_child_prefix(prefix, last);
    if (condition)
      condition->print_tree(os, child, false);
    if (body)
      body->print_tree(os, child, true);
  }
};

struct CustomStmt : Statement {
  void interpret(ostream& out) const override {
    out << "SENIORITIS strikes! Time for a nap and a snack.\n";
  }

  void print_tree(ostream& os, const string& prefix, bool last) const override {
    ast_line(os, prefix, last, "Senioritis");
  }
};

// ----------------------------------------------------------------------------- 
// Block & Program
// -----------------------------------------------------------------------------
struct Block {
  vector<VarDecl> declarations;
  unique_ptr<CompoundStmt> compound;

  void interpret(ostream& out) const {
    if (compound)
      compound->interpret(out);
  }

  void print_tree(ostream& os, const string& prefix, bool last) const {
    ast_line(os, prefix, last, "Block");
    string childPrefix = ast_child_prefix(prefix, last);

    if (!declarations.empty()) {
      ast_line(os, childPrefix, compound ? false : true, "Declarations");
      string declPrefix = ast_child_prefix(childPrefix, compound ? false : true);
      for (size_t i = 0; i < declarations.size(); ++i) {
        declarations[i].print_tree(os, declPrefix, i + 1 == declarations.size());
      }
    }

    if (compound) {
      bool lastChild = true;
      compound->print_tree(os, childPrefix, lastChild);
    }
  }
};

struct Program {
  string name;
  unique_ptr<Block> block;

  void interpret(ostream& out) const {
    if (block)
      block->interpret(out);
  }

  void print_tree(ostream& os) const {
    os << "Program " << name << "\n";
    if (block)
      block->print_tree(os, "", true);
  }

  void print_symbols(ostream& os) const {
    for (const auto& [name, value] : symbolTable) {
      auto typeIt = symbolTypes.find(name);
      const char* typeName = (typeIt != symbolTypes.end()) ? to_string(typeIt->second) : "?";
      os << name << " : " << typeName << " = ";
      visit([&](auto&& v) { os << v; }, value);
      os << "\n";
    }
  }
};

inline ostream& operator<<(ostream& os, const Program& program) {
  program.print_tree(os);
  return os;
}

inline ostream& operator<<(ostream& os, const unique_ptr<Program>& program) {
  if (program)
    program->print_tree(os);
  return os;
}
