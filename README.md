# TIPS Language Interpreter

A lexer, recursive-descent parser, and tree-walking interpreter for a subset of the **TIPS** programming language, built as a capstone project for **MSU CSE 4714/6714** (Fall 2025).

TIPS is a Pascal-like language with uppercase keywords, typed variable declarations, and structured control flow. This project implements the full pipeline from source text to execution: tokenization via Flex, parsing into an AST, and direct interpretation of that AST.

---

## Features

- **Lexer** (Flex) — tokenizes TIPS source files; supports identifiers, integer/real literals, string literals, operators, and comments (`##`)
- **Recursive-descent parser** — builds a typed AST with full error reporting (line numbers, expected vs. got)
- **Symbol table** — typed variable declarations (`INTEGER`, `REAL`) with duplicate-declaration and use-before-declare checks
- **Interpreter** — tree-walking execution of the full statement set
- **CLI flags** — tokenize-only mode, AST printer, symbol table dump, debug traces

### Supported language constructs

| Construct | Syntax |
|---|---|
| Program structure | `PROGRAM name; VAR ... BEGIN ... END` |
| Variable declaration | `name : INTEGER;` / `name : REAL;` |
| Assignment | `X := expr` |
| Input / Output | `READ(X)` / `WRITE(expr)` / `WRITE('string')` |
| Conditionals | `IF cond THEN stmt ELSE stmt` |
| Loops | `WHILE cond stmt` |
| Arithmetic | `+`, `-`, `*`, `/`, `MOD`, `^^` (power), `++`/`--` (unary) |
| Logic | `AND`, `OR`, `NOT` |
| Comparison | `=`, `<>`, `<`, `>` |
| Custom statement | `SENIORITIS` — prints a fun message |
| Comments | `## this is a comment` |

---

## Building

Requires **g++** (C++17) and **flex**.

```bash
make
```

This generates the `parse` executable. To clean build artifacts:

```bash
make clean
```

---

## Usage

```
./parse [options] [file]

Options:
  -p            Print AST after parsing
  -t            Tokenize only (dump tokens) and exit
  -s            Print symbol table after interpretation
  -d            Enable debug traces to stderr
  --skin=NAME   Select keyword skin (default, pirate, cat)
  --help        Show this help
```

**Examples:**

```bash
# Run a TIPS program
./parse program.tips

# Dump tokens only
./parse -t program.tips

# Parse and print the AST, then interpret
./parse -p program.tips

# Interpret and show the symbol table
./parse -s program.tips
```

---

## Sample TIPS Program

```
PROGRAM EXAMPLE;
VAR
  X : INTEGER;
  Y : REAL;
BEGIN
  X := 5;
  Y := X * 2.5;
  WRITE('Result:');
  WRITE(Y);
  IF X > 3 THEN
    WRITE('X is greater than 3')
  ELSE
    WRITE('X is not greater than 3')
END
```

---

## Project Structure

| File | Purpose |
|---|---|
| `rules.l` | Flex lexer rules |
| `lexer.h` | Token definitions shared between lexer and parser |
| `parser.cpp` | Recursive-descent parser; builds AST and populates symbol table |
| `ast.h` | AST node types with `interpret()` and `print_tree()` methods |
| `driver.cpp` | CLI entry point; wires together lexer, parser, and interpreter |
| `debug.h` | Debug trace helpers |
| `makefile` | Build system |
