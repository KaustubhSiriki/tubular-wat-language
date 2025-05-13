# Tubular: A Simple Programming Language Compiler to WebAssembly

A lightweight programming language and compiler developed as part of a university Compilers course. Tubular supports core language features—data types, operators, control flow, functions, and robust string operations—and emits WebAssembly Text Format (`.wat`) for execution in browsers or Node.js.

## Features

- **Data Types**: Integer, Character, String, Boolean
- **Operators**:
  - Arithmetic: `+`, `-`, `*`, `/`
  - Relational: `==`, `!=`, `<`, `<=`, `>`, `>=`
  - Logical: `&&`, `||`, `!`
- **String Operations**:
  - Single and multiple-string concatenation
  - String indexing and length function
  - Character-to-string conversion and vice versa
  - Character padding using the `*` operator
- **Control Flow**:
  - `if` statements with optional `else`
  - `while` loops
- **Functions**:
  - User-defined functions with parameters and return values
- **Error Handling**:
  - Graceful syntax and runtime error detection and reporting
- **Code Generation**:
  - Emits WebAssembly Text Format (`.wat`), ready for assembly into `.wasm`

## Getting Started

### Prerequisites

- A C++20–compatible compiler (e.g., `g++` or `clang++`)
- [WebAssembly Binary Toolkit (WABT)](https://github.com/WebAssembly/wabt) for `wat2wasm`
- Node.js or a modern browser to run the generated WebAssembly

### Build

```bash
git clone https://github.com/KaustubhSiriki/tubular-wat-language.git
cd tubular
make
```

### Compile & Run

```bash
# Compile a source file (.tube) to WebAssembly Text (WAT)
./Project4 examples/hello.tube > output.wat

# Convert WAT to Wasm
wat2wasm output.wat -o output.wasm

# Run in Node.js
node run_wasm.js output.wasm

# Or open browser-based tester:
open tests/wasm-tester.html
```

## Tests

A comprehensive test suite is provided under `tests/`:

- `.tube` source files exercising language features
- Expected `.wat` and `.wasm` outputs for each test
- `wasm-tester.html` for interactive browser validation

```bash
make tests
```

## Project Structure

```
├── ASTNode.hpp          # AST node definitions
├── Control.hpp          # Control-flow code generation
├── DataType.hpp         # Type definitions and helpers
├── Function.hpp         # Function codegen utilities
├── Parser.hpp           # Parser implementation
├── Project4.cpp         # Compiler driver and codegen entrypoint
├── lexer.hpp            # Tokenizer definitions
├── TokenQueue.hpp       # Token management helper
├── tools.hpp            # Utility functions
├── Makefile             # Build and test commands
├── LICENSE              # MIT License
└── tests/               # Language conformance tests
    ├── *.tube           # Test source files
    ├── *.wat            # Expected WebAssembly Text
    ├── *.wasm           # Expected WebAssembly binaries
    └── wasm-tester.html # Browser-based test harness
```

## Acknowledgments

Developed in a team of three for a Compilers class. This project showcases end-to-end compiler construction: parsing, AST generation, semantic checks, and backend code generation to WebAssembly.

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for full details.