#include <assert.h>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
// UNCOMMENT THESE IF YOU WANT TO USE THEM
// #include "ASTNode.hpp"
#include "Parser.hpp"
#include "lexer.hpp"
// #include "SymbolTable.hpp"
#include "TokenQueue.hpp"  // A fully-implemented token manager
// #include "tools.hpp"       // A few helpful functions

void CharToStringFunction() {
    // Hard coded function to convert a CHAR to a STRING in memory
    std::cout << "(func $char_to_string (param $char i32) (result i32)\n";
    std::cout << "  (local $address i32)\n"; // Local variable to hold the memory address
    std::cout << "  ;; Allocate memory for the string\n";
    std::cout << "  (global.get $free_mem)\n";
    std::cout << "  (local.set $address)\n";
    std::cout << "  ;; Store the char as a single-character string\n";
    std::cout << "  (local.get $address)\n";
    std::cout << "  (local.get $char)\n";
    std::cout << "  (i32.store8)\n";
    std::cout << "  ;; Null-terminate the string\n";
    std::cout << "  (local.get $address)\n";
    std::cout << "  (i32.const 1)\n";
    std::cout << "  (i32.add)\n";
    std::cout << "  (i32.const 0)\n";
    std::cout << "  (i32.store8)\n";
    std::cout << "  ;; Update free_mem\n";
    std::cout << "  (local.get $address)\n";
    std::cout << "  (i32.const 2)\n";
    std::cout << "  (i32.add)\n";
    std::cout << "  (global.set $free_mem)\n";
    std::cout << "  ;; Return the address of the string\n";
    std::cout << "  (local.get $address)\n";
    std::cout << ")\n";
    std::cout << "(export \"char_to_string\" (func $char_to_string))\n";
}

void GetSizeFunction() {
  // hard coded function to get the length of a string
  // HAS NOT BEEN TESTED
  std::cout << "(func $get_length (param $str i32) (result i32) \n";
  std::cout << "  (local $length i32)\n";
  std::cout << "  (local $current i32)\n";
  std::cout << "  (block $fun_exit1 (result i32)\n";
  std::cout << "    (local.set $length (i32.const 0))\n";
  std::cout << "    (block $exit1\n";
  std::cout << "    (loop $loop1\n";
  std::cout << "      (local.get $str)\n";
  std::cout << "      (local.get $length)\n";
  std::cout << "      (i32.add)\n";
  std::cout << "      (i32.load8_u) ;; access the character\n ";
  std::cout << "      (local.set $current)\n\n";
  std::cout << "      (local.get $current)\n";
  std::cout << "      (i32.eqz) ;; check if the character is null terminator\n";
  std::cout << "      (br_if $exit1)\n";
  std::cout << "      (local.get $length)\n";
  std::cout << "      (i32.const 1)\n";
  std::cout << "      (i32.add) ;; add 1 to the length of a string\n";
  std::cout << "      (local.set $length)\n";
  std::cout << "      (br $loop1)\n";
  std::cout << "    )\n";
  std::cout << "    )\n";
  std::cout << "  (local.get $length)\n";
  std::cout << "  )\n";
  std::cout << ")\n";
  std::cout << "(export \"get_length\" (func $get_length))\n";
}

void AddStringsFunction() {
  // hard coded function to add two strings
  std::cout << "(func $add_strings (param $str1 i32) (param $str2 i32) (result i32)\n";
  std::cout << "  (local $old_free_mem i32)\n";
  std::cout << "  (local $count i32)\n"; // total offset from the beginning
  std::cout << "  (local $current i32)\n";
  std::cout << "  (local $i i32)\n"; // offset in the second string

  std::cout << "  (global.get $free_mem)\n";
  std::cout << "  (local.set $old_free_mem)\n";
  std::cout << "  (i32.const 0)\n";
  std::cout << "  (local.set $count)\n";
  std::cout << "    (i32.const 0)\n";
  std::cout << "    (local.set $i)\n";

  std::cout << "  (block $fun_exit1 (result i32)\n";

// loop through the first string
  std::cout << "    (block $exit1\n";
  std::cout << "      (loop $loop1\n";

  std::cout << "        (local.get $str1)\n";
  std::cout << "        (local.get $count)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (i32.load8_u)\n";
  std::cout << "        (local.set $current)\n"; // load current character

  std::cout << "        (local.get $current)\n"; // check if it's not zero
  std::cout << "        (i32.eqz)\n";
  std::cout << "        (br_if $exit1)\n";

  std::cout << "        (local.get $old_free_mem)\n";
  std::cout << "        (local.get $count)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (local.get $current)\n";
  std::cout << "        (i32.store8)\n"; // copy to a new string

  std::cout << "        (local.get $count)\n"; 
  std::cout << "        (i32.const 1)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (local.set $count)\n"; // update count
  std::cout << "        (br $loop1)\n";
  std::cout << "      )\n";
  std::cout << "    )\n";


  std::cout << "    (block $exit2\n";
  std::cout << "      (loop $loop2\n";


// do the same for the second string
  std::cout << "        (local.get $str2)\n";
  std::cout << "        (local.get $i)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (i32.load8_u)\n";
  std::cout << "        (local.set $current)\n"; // load current character

  std::cout << "        (local.get $current)\n"; // check if not zero
  std::cout << "        (i32.eqz)\n";
  std::cout << "        (br_if $exit2)\n";

  std::cout << "        (local.get $old_free_mem)\n";
  std::cout << "        (local.get $count)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (local.get $current)\n";
  std::cout << "        (i32.store8)\n"; // copy to a new string

  std::cout << "        (local.get $count)\n";
  std::cout << "        (i32.const 1)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (local.set $count)\n";
  std::cout << "        (local.get $i)\n";
  std::cout << "        (i32.const 1)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (local.set $i)\n";
  std::cout << "        (br $loop2)\n"; // update count and i
  std::cout << "      )\n";
  std::cout << "    )\n";

  // null character
  std::cout << "    (local.get $old_free_mem)\n";
  std::cout << "    (local.get $count)\n";
  std::cout << "    (i32.add)\n";
  std::cout << "    (i32.const 0)\n";
  std::cout << "    (i32.store8)\n";

  //update free mem
  std::cout << "    (local.get $old_free_mem)\n";
  std::cout << "    (local.get $count)\n";
  std::cout << "    (i32.add)\n";
  std::cout << "    (i32.const 1)\n";
  std::cout << "    (i32.add)\n";
  std::cout << "    (global.set $free_mem)\n";

  // return
  std::cout << "    (local.get $old_free_mem)\n";
  std::cout << "  )\n";
  std::cout << ")\n";
  std::cout << "(export \"add_strings\" (func $add_strings))\n";
}

void PadCharFunction() {
  
  // std::cout << "(func $pad_char\n";
  // std::cout << "  (param $repeat i32)    ;; Input character as ASCII\n";
  // std::cout << "  (param $char i32)  ;; Number of repetitions\n";
  // std::cout << "  (result i32)         ;; Address of the resulting string\n\n";
  // std::cout << "  (local $offset i32)  ;; Offset to track writing position\n";
  // std::cout << "  (local $free_mem_copy i32) ;; Copy of $free_mem for returning the start address\n" << "\n";

  // std::cout << "  ;; Initialize $free_mem_copy with the current $free_mem\n";
  // std::cout << "  (global.get $free_mem)\n";
  // std::cout << "  (local.set $free_mem_copy)\n" << "\n";

  // std::cout << "  ;; Initialize $offset to 0\n";
  // std::cout << "  (i32.const 0)\n";
  // std::cout << "  (local.set $offset)\n" << "\n";

  // std::cout << "  ;; While loop: offset < repeat\n";
  // std::cout << "  (block $loop_exit\n";
  // std::cout << "    (loop $loop\n";
  // std::cout << "      ;; Break the loop if offset >= repeat\n";
  // std::cout << "      (local.get $offset)\n";
  // std::cout << "      (local.get $repeat)\n";
  // std::cout << "      (i32.ge_u)\n";
  // std::cout << "      (br_if $loop_exit)\n" << "\n";

  // std::cout << "      ;; Store $char at address $free_mem + $offset\n";
  // std::cout << "      (global.get $free_mem)\n";
  // std::cout << "      (local.get $offset)\n";
  // std::cout << "      (i32.add)\n";
  // std::cout << "      (local.get $char)\n";
  // std::cout << "      (i32.store8)\n" << "\n";

  // std::cout << "      ;; Increment $offset\n";
  // std::cout << "      (local.get $offset)\n";
  // std::cout << "      (i32.const 1)\n";
  // std::cout << "      (i32.add)\n";
  // std::cout << "      (local.set $offset)\n" << "\n";

  // std::cout << "      ;; Continue the loop\n";
  // std::cout << "      (br $loop)\n";
  // std::cout << "    )\n";
  // std::cout << "  )\n" << "\n";

  // std::cout << "  ;; Add null terminator at the end ($free_mem + $offset)\n";
  // std::cout << "  (global.get $free_mem)\n";
  // std::cout << "  (local.get $offset)\n";
  // std::cout << "  (i32.add)\n";
  // std::cout << "  (i32.const 0)\n";
  // std::cout << "  (i32.store8)\n" << "\n";

  // std::cout << "  ;; Update $free_mem: $free_mem + $offset + 2\n";
  // std::cout << "  (global.get $free_mem)\n";
  // std::cout << "  (local.get $offset)\n";
  // std::cout << "  (i32.add)\n";
  // std::cout << "  (i32.const 2)\n";
  // std::cout << "  (i32.add)\n";
  // std::cout << "  (global.set $free_mem)\n" << "\n";

  // std::cout << "  ;; Return the start address of the string\n";
  // std::cout << "  (local.get $free_mem_copy)\n";
  // std::cout << ")\n" << "\n";

  // std::cout << ";; Export the pad_char function\n";
  // std::cout << "  (export \"pad_char\" (func $pad_char))\n\n";

  std::cout << "(func $pad_char (param $repeat i32) (param $char i32) (result i32)\n";
  std::cout << "  (local $char_str i32)\n";
  std::cout << "  (local $result i32)\n";
  std::cout << "  (local $i i32)\n";

  std::cout << "    (block $empty_string_block\n";
  std::cout << "      (local.get $repeat)\n";
  std::cout << "      (i32.eqz)\n";
  std::cout << "      (if\n";
  std::cout << "        (then\n";
  std::cout << "          (global.get $free_mem)\n";
  std::cout << "          (i32.const 0)\n";
  std::cout << "          (i32.store8)\n";
  std::cout << "          (global.get $free_mem)\n";
  std::cout << "          (i32.const 1)\n";
  std::cout << "          (i32.add)\n";
  std::cout << "          (global.set $free_mem)\n";
  std::cout << "          (global.get $free_mem)\n";
  std::cout << "          (return)\n";
  std::cout << "        )\n";
  std::cout << "      )\n";
  std::cout << "    )\n";
  std::cout << "\n";
  std::cout << "    ;; Convert char to string\n";
  std::cout << "    (local.get $char)\n";
  std::cout << "    (call $char_to_string)\n";
  std::cout << "    (local.set $char_str)\n";
  std::cout << "\n";
  std::cout << "    (local.get $char_str)\n";
  std::cout << "    (local.set $result)\n";
  std::cout << "\n";
  std::cout << "    (local.set $i (i32.const 1))\n";
  std::cout << "    (block $exit_block\n";
  std::cout << "      (loop $loop\n";
  std::cout << "        ;; Break if $i >= $repeat\n";
  std::cout << "        (local.get $i)\n";
  std::cout << "        (local.get $repeat)\n";
  std::cout << "        (i32.ge_u)\n";
  std::cout << "        (br_if $exit_block)\n";
  std::cout << "\n";
  std::cout << "        ;; Concatenate $result with $char_str\n";
  std::cout << "        (local.get $result)\n";
  std::cout << "        (local.get $char_str)\n";
  std::cout << "        (call $add_strings)\n";
  std::cout << "        (local.set $result) ;; Update result after concatenation\n";
  std::cout << "\n";
  std::cout << "        ;; Increment $i\n";
  std::cout << "        (local.get $i)\n";
  std::cout << "        (i32.const 1)\n";
  std::cout << "        (i32.add)\n";
  std::cout << "        (local.set $i)\n";
  std::cout << "\n";
  std::cout << "        ;; Repeat the loop\n";
  std::cout << "        (br $loop)\n";
  std::cout << "      )\n";
  std::cout << "    )\n";
  std::cout << "\n";
  std::cout << "    ;; Return the final string address\n";
  std::cout << "    (local.get $result)\n";
  std::cout << "  )\n";
  std::cout << "\n";
  std::cout << "  (export \"pad_char\" (func $pad_char))\n";
}

void SetAtFunction() {
  std::cout << "  ;; Function to reassign a specific index of a string\n";
  std::cout << "  (func $set_at\n";
  std::cout << "    (param $str i32)    ;; Address of the string\n";
  std::cout << "    (param $index i32)  ;; Index to modify\n";
  std::cout << "    (param $char i32)   ;; ASCII value of the character\n";
  std::cout << "    (result i32)        ;; Return the updated string address\n";
  std::cout << "    (local $target_addr i32) ;; Address of the target index\n";
  std::cout << "    (local $current_char i32) ;; Char at the current index (optional null-check)\n\n";

  std::cout << "    ;; Calculate the target address: $str + $index\n";
  std::cout << "    (local.get $str)\n";
  std::cout << "    (local.get $index)\n";
  std::cout << "    (i32.add)\n";
  std::cout << "    (local.set $target_addr)\n\n";

  std::cout << "    ;; Store the new character at the target address\n";
  std::cout << "    (local.get $target_addr)\n";
  std::cout << "    (local.get $char)\n";
  std::cout << "    (i32.store8)\n\n";

  std::cout << "    ;; Return the start of the string address\n";
  std::cout << "    (local.get $str)\n";
  std::cout << "  )\n\n";

  std::cout << "  (export \"set_at\" (func $set_at))\n";
  std::cout << ")\n";
}

class Tubular {
private:
  // Private member variables and helper functions
  TokenQueue tokens;
  std::vector<Function*> functions{};
public:
  Tubular(std::string filename) {    
    std::ifstream in_file(filename);              // Load the input file
    if (in_file.fail()) {
      std::cerr << "ERROR: Unable to open file '" << filename << "'." << std::endl;
      exit(1);
    }
    tokens.Load(in_file);  // Load all tokens from the file.
  }

  void Parse() {
    // Outer layer can only be function definitions.
    Parser parser(tokens);
    functions = parser.Parse();
  }

  void ToWASM() {
    // CODE GOES HERE to convert the AST made in Parse() into WebAssembly Text

    std::cout << "(module\n"; // begin module

    std::cout << "(memory (export \"memory\") 10)\n"; // allocate memory

    Function::GenerateLiteralStrings();

    GetSizeFunction(); // hard coded function for the size of a string
    AddStringsFunction(); // hard coded function for adding two strings
    CharToStringFunction(); // hard coded function for converting a char to a string
    PadCharFunction(); // hard coded function for repeating a char


    
    for (auto& func : functions)
      func->GenerateCode();

    std::cout << ")\n"; // end module
  }

  // ... OTHER USEFUL PUBLIC FUNCTIONS ...

};


int main(int argc, char * argv[])
{
  if (argc != 2) {
    std::cout << "Format: " << argv[0] << " [filename]" << std::endl;
    exit(1);
  }

  Tubular prog(argv[1]);
  prog.Parse();
  prog.ToWASM();
}
