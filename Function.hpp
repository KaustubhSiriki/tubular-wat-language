#pragma once
#include <string>
#include "DataType.hpp"
#include <vector>
#include "lexer.hpp"
//#include "ASTNode.hpp"




class Function{
private:
    int id = 0;
    emplex::Token identifier;
    DataType returnType;
    std::vector<std::pair<DataType, int>> args{}; // pairs of function arguments in function signature: <DataType, unique id in the SymbolTable>
    std::vector<std::pair<DataType, int>> localVars{}; // local variables declared in function body
    std::vector<ASTNode*> nodes{};

    static std::vector<std::string> literalStrings;
public:
    Function(int id, const emplex::Token& identifier, const DataType& returnType, const std::vector<std::pair<DataType, int>>& arguments)
        : id(id), identifier(identifier), returnType(returnType), args(arguments) {}

    void AddNode(ASTNode* node) {
        nodes.push_back(node);
    }

    std::string GetFunctionName() {
        return identifier.lexeme;
    }

    static void addLiteralString(std::string str) {
        Function::literalStrings.push_back(str);
    }

    static void GenerateLiteralStrings() {
        int memoryPos = 0;

        for (auto& str : Function::literalStrings) {
            std::cout << "(data (i32.const " << memoryPos << ") \"" << str << "\\00\")\n";
            memoryPos += str.length() + 1;
        }

        std::cout << "(global $free_mem (mut i32) (i32.const " << memoryPos << "))\n";
    }

    DataType GetReturnType() {
        return returnType;
    }

    int GetNumberOfArguments() {
        return args.size();
    }
    void AddLocalVar(std::pair<DataType, int> localVar) {
        localVars.push_back(localVar);
    }

    std::vector<std::pair<DataType, int>> getArgs() {
        return args;
    }

    void GenerateCode() {
        // function name
        std::cout << "(func $" << identifier.lexeme << " ";

        // function arguments
        for (auto& arg : args)
        {
            std::cout << "(param $var" << arg.second << " " << DataType_ToCode(arg.first) << ") ";
        }

        // return type
        std::cout << "(result " << DataType_ToCode(returnType) << ")\n";

        //local variables 
        for (auto& var : localVars) {
            std::cout << "(local $var" << var.second <<" " << DataType_ToCode(var.first) << ")\n";
        }


        // block
        std::cout << "(block $fun_exit" << id << " (result " << DataType_ToCode(returnType) << ")\n";


        // nodes
        for (auto& node : nodes)
            node->GenerateCode();

        // close block
        std::cout << ")   ;; end of function block.\n";

        std::cout << ")   ;; end of function definition\n";

        // export function

        std::cout << "(export \"" << identifier.lexeme << "\" (func $" << identifier.lexeme << "))\n"; 
    }
};


std::vector<std::string> Function::literalStrings;