#pragma once

// Some potentially useful member functions.
#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include "DataType.hpp"
#include "lexer.hpp"
//#include "Function.hpp"

using namespace emplex;
class ASTNode {
protected:
    /*
    We must keep track of a resulting data type. For example,
    if we add int to double, we will get double. If we add int
    to char, we will throw an error.
    */
    DataType type; 
public:
    virtual void GenerateCode() = 0;

    /*
    Not all nodes will need this function, for example if-else or while.
    In this case just provide empty implementation.
    */
    virtual DataType GetDataType() = 0;

    void UpdateType(DataType type) {
        this->type = type;
    }
};


class LiteralValueNode : public ASTNode {
private:
    int value_int; // for int and char (chars can be represented with ASCII indexes)
    double value_double; // for double
    char character; // for char
public:
    LiteralValueNode(const DataType& valueType, int value_int) : value_int(value_int) {
        this->type = valueType;
    }
    LiteralValueNode(const DataType& valueType, double value_double) : value_double(value_double) {
        this->type = valueType;
    }
    LiteralValueNode(const DataType& valueType, char value) : character(value) {
        this->type = valueType;
        this->value_int = int(character);
    }

    void GenerateCode() {
        if (type == DataType::INTEGER || type == DataType::CHAR)
            std::cout << "(i32.const " << value_int << ")\n";
        else if (type == DataType::DOUBLE)
            std::cout << "(f64.const " << value_double << ")\n";
    }

    DataType GetDataType() {
        return type;
    }
};

class VariableNode : public ASTNode {
private:
    int unique_id = 0;
public:
    VariableNode(int unique_id, DataType type) : unique_id(unique_id) {
        this->type = type;
    }

    void GenerateCode() {
        std::cout << "(local.get $var" << unique_id << ")\n";
    }

    DataType GetDataType() {
        return type;
    }

    int GetUniqueId() {
        return unique_id;
    }

};

class ExpressionNode : public ASTNode {
private:
    ASTNode* expression;
public:
    ExpressionNode(ASTNode* expression) : expression(expression) {}

    void GenerateCode(){
        // just put the result of the expression on top of the stack
        expression->GenerateCode();
    }

    DataType GetDataType() {
        return expression->GetDataType();
    }
};

class ReturnNode : public ASTNode {
private:
    ASTNode* expression = nullptr;
    bool insideIf = false;
public:
    ReturnNode(ASTNode* expression) : expression(expression) {}
    void GenerateCode() {
        expression->GenerateCode();
        if (insideIf) {
            std::cout << "(return)\n";
        }
    }
    DataType GetDataType() {
        return expression->GetDataType();
    }
};
class LiteralStringNode : public ASTNode {
private:
    int memoryPos; // index of a literal string in global memory
    int length;
public:
    void GenerateCode() {
        // put the index of the start of the string onto a stack
        std::cout << "(i32.const " << memoryPos << ")\n";
    }

    DataType GetDataType() {
        return DataType::STRING;
    }

    int getMemoryPos() {
        return memoryPos;
    }

    int getLength() {
        return length;
    }

    LiteralStringNode(int memoryPos, int length) : memoryPos(memoryPos), length(length) {} 
};

class IndexNode : public ASTNode {
private:
    ASTNode* variableNode; // Should be a VariableNode representing the string
    ASTNode* indexExpression; // Expression resulting in the index

public:
    IndexNode(ASTNode* variableNode, ASTNode* indexExpression)
        : variableNode(variableNode), indexExpression(indexExpression) {
        this->type = DataType::CHAR; // Indexing a string results in a CHAR
    }

    void GenerateCode() override {
        // Generate code to load the base address of the string variable
        variableNode->GenerateCode(); // Pushes the base address onto the stack

        // Generate code for the index expression
        indexExpression->GenerateCode(); // Pushes the index onto the stack

        // Add base address and index
        std::cout << "(i32.add)\n";

        // Load the byte at the calculated address
        std::cout << "(i32.load8_u)\n";
    }

    void GenerateCodeAssignment() {
        // Generate code to load the base address of the string variable
        variableNode->GenerateCode(); // Pushes the base address onto the stack

        // Generate code for the index expression
        indexExpression->GenerateCode(); // Pushes the index onto the stack

        // Add base address and index
        std::cout << "(i32.add)\n";
    }

    ASTNode* GetIndexExpression() {
        return indexExpression;
    }

    DataType GetDataType() override {
        return DataType::CHAR;
    }
};

class BinaryOpNode : public ASTNode {
private:
    emplex::Token op;
    ASTNode* lhs = nullptr;
    ASTNode* rhs = nullptr;
    ASTNode* index = nullptr;
    bool stack = false;

public:
    BinaryOpNode(emplex::Token operation) : op(operation) {}

    void SetLeft(ASTNode* left) {
        lhs = left;
    }

    void SetRight(ASTNode* right) {
        rhs = right;
    }

    void SetIndex(ASTNode* index) {
        this->index = index;
    }

    void SetStack(bool stack) {
        this->stack = stack;
    }

    bool GetStack() {
        return stack;
    }


    void GenerateCode() {
        // Declare the variable outside the switch to avoid jump errors
        VariableNode* varNode = nullptr;
        IndexNode* indexNode = nullptr;

        if (op.id != Lexer::ID_add && op.id != Lexer::ID_assignment && op.id != Lexer::ID_and && op.id != Lexer::ID_or && op.id != Lexer::ID_divide && op.id != Lexer::ID_multiply) {
            lhs->GenerateCode();
            rhs->GenerateCode();
        }

        DataType type = GetDataType();
        std::string type_str = DataType_ToCode(type); // "i32" or "f64"

        switch (op.id) {
            case Lexer::ID_add:
                if ((lhs->GetDataType() == DataType::STRING || lhs->GetDataType() == DataType::CHAR) &&
                    (rhs->GetDataType() == DataType::STRING || rhs->GetDataType() == DataType::CHAR)) {
                    
                    lhs->GenerateCode();
                    if (lhs->GetDataType() == DataType::CHAR && rhs->GetDataType() == DataType::STRING) {
                        std::cout << "(call $char_to_string)\n"; // Convert to string
                    }

                    rhs->GenerateCode();
                    if (rhs->GetDataType() == DataType::CHAR && lhs->GetDataType() == DataType::STRING) {
                        std::cout << "(call $char_to_string)\n"; // Convert to string
                    }

                    // Concatenate lhs and rhs
                    if (lhs->GetDataType() == DataType::CHAR && rhs->GetDataType() == DataType::CHAR) {
                        std::cout << "(i32.add)\n";
                    }
                    else {
                        std::cout << "(call $add_strings)\n";
                    }

                    return;
                }

                // Handle numerical addition
                if (lhs->GetDataType() != DataType::STRING && rhs->GetDataType() != DataType::STRING) {
                    lhs->GenerateCode();
                    rhs->GenerateCode();
                    std::cout << "(" << type_str << ".add)\n";
                }
                break;
            case Lexer::ID_multiply:
                
                lhs->GenerateCode();

                // conversion int to double if one of the operands is a double
                if (lhs->GetDataType() != DataType::DOUBLE && rhs->GetDataType() == DataType::DOUBLE) {
                    std::cout << "(f64.convert_i32_s)\n"; // convert to double
                }
                else if (lhs->GetDataType() == DataType::CHAR && rhs->GetDataType() == DataType::INTEGER) {
                    std::cout << "(call $char_to_string)\n";
                }

                rhs->GenerateCode();
                if (rhs->GetDataType() != DataType::DOUBLE && lhs->GetDataType() == DataType::DOUBLE) {
                    std::cout << "(f64.convert_i32_s)\n"; // convert to double
                }
                else if (rhs->GetDataType() == DataType::INTEGER && lhs->GetDataType() == DataType::CHAR) {
                    lhs->GenerateCode();
                    std::cout << "(call $pad_char)\n";
                    break;
                }
                std::cout << "(" << type_str << ".mul)\n";
                break;
            case Lexer::ID_negation:
                std::cout << "(" << type_str << ".sub)\n";
                break;
            case Lexer::ID_modulus:
                if (type == DataType::DOUBLE) {
                    Error("Modulus operation is not supported for doubles\n");
                }
                std::cout << "(" << type_str << ".rem_s)\n";
                break;
            case Lexer::ID_less_than:
                std::cout << "(" << type_str << ".lt";
                if (type != DataType::DOUBLE) {
                    std::cout << "_s";
                }
                std::cout << ")\n";
                break;
            case Lexer::ID_greater_than:
                std::cout << "(" << type_str << ".gt";
                if (type != DataType::DOUBLE) {
                    std::cout << "_s";
                }
                std::cout << ")\n";
                break;
            case Lexer::ID_less_or_eq:
                std::cout << "(" << type_str << ".le";
                if (type != DataType::DOUBLE) {
                    std::cout << "_s";
                }
                std::cout << ")\n";
                break;
            case Lexer::ID_greater_or_eq:
                std::cout << "(" << type_str << ".ge";
                if (type != DataType::DOUBLE) {
                    std::cout << "_s";
                }
                std::cout << ")\n";
                break;
            case Lexer::ID_not_eq:
                std::cout << "(" << type_str << ".ne)\n";
                break;

            case Lexer::ID_and:
                lhs->GenerateCode(); // test left side and then perform if

                std::cout << "(if (result " << type_str << ")\n";
                std::cout << "(then\n";
                rhs->GenerateCode();

                std::cout << "(i32.const 0)\n";
                std::cout << "(i32.ne)\n";
                std::cout << ")\n"; // close then

                std::cout << "(else\n";
                std::cout << "(i32.const 0)\n";
                std::cout << ")\n"; // close else

                std::cout << ")\n"; // close if
                break;

            case Lexer::ID_or:
                lhs->GenerateCode(); // test left side and then perform if

                std::cout << "(if (result " << type_str << ")\n";
                std::cout << "(then\n";
                std::cout << "(i32.const 1)\n";
                std::cout << ")\n"; // close then

                std::cout << "(else\n";
                rhs->GenerateCode();

                std::cout << "(i32.const 0)\n";
                std::cout << "(i32.ne)\n";
                std::cout << ")\n"; // close else

                std::cout << ")\n"; // close if
                break;

            case Lexer::ID_assignment:
                if (index == nullptr)
                    rhs->GenerateCode();

                // Assign the variable
                varNode = dynamic_cast<VariableNode*>(lhs);
                if (!varNode) {
                    Error("Could not obtain a variable on the left side of assignment");
                }
                if (varNode->GetDataType() == DataType::DOUBLE && rhs->GetDataType() != DataType::DOUBLE) {
                    std::cout << "(f64.convert_i32_s)\n"; // convert right hand side if needed
                }

                // check for indexing, and if so, generate code for indexing
                if (index == nullptr) {

                    std::cout << "(local.set $var" << varNode->GetUniqueId() << ")\n";
                    std::cout << "(local.get $var" << varNode->GetUniqueId() << ")\n";
                    if (!stack)
                        std::cout << "(drop)\n";
                    break;
                }
                else {
                    // Assign the variable
                    indexNode = dynamic_cast<IndexNode*>(index);
                    if (!indexNode) {
                        Error("Could not obtain an index");
                    }
                    else {
                        auto expression = indexNode->GetIndexExpression();
                        expression->GenerateCode();
                    }

                    std::cout << "(local.get $var" << varNode->GetUniqueId() << ")\n";
                    // Add base address and index
                    std::cout << "(i32.add)\n";
                    rhs->GenerateCode();
                    std::cout << "(i32.store8)\n";
                    
                    break;
                }

            case Lexer::ID_equality:
                std::cout << "(i32.eq)\n";
                break;
            case Lexer::ID_divide:
                lhs->GenerateCode();

                // conversion int to double if one of the operands is a double
                if (lhs->GetDataType() != DataType::DOUBLE && rhs->GetDataType() == DataType::DOUBLE) {
                    std::cout << "(f64.convert_i32_s)\n"; // convert to double
                }
                rhs->GenerateCode();
                if (rhs->GetDataType() != DataType::DOUBLE && lhs->GetDataType() == DataType::DOUBLE) {
                    std::cout << "(f64.convert_i32_s)\n"; // convert to double
                }
                if (type == DataType::DOUBLE) {
                    std::cout << "(f64.div)\n";
                } else 
                    std::cout << "(i32.div_s)\n";
                break;
            default:
                Error("Encountered unknown operation in BinaryOpNode: ", op.lexeme, "\n");
        }
    }


    DataType GetDataType() {
        DataType left_type = lhs->GetDataType();
        DataType right_type = rhs->GetDataType();
        
        switch (op.id) {
            case Lexer::ID_add:
            case Lexer::ID_negation: {
                if (left_type == DataType::CHAR && right_type == DataType::CHAR)
                    return DataType::CHAR;
                if ((left_type == DataType::CHAR && right_type == DataType::DOUBLE) ||
                    (right_type == DataType::CHAR && left_type == DataType::DOUBLE))
                    Error("CHAR can only be added or subtracted with CHAR or INT");
                return std::max(left_type, right_type);
            }
            
            case Lexer::ID_multiply:
            case Lexer::ID_divide: {
                if (left_type == DataType::CHAR && right_type == DataType::CHAR)
                    Error("Cannot perform multiplication/division with CHAR type");
                return std::max(left_type, right_type);
            }
            
            case Lexer::ID_modulus: {
                if (left_type == DataType::CHAR || right_type == DataType::CHAR)
                    Error("Cannot perform modulus with CHAR type");
                if (left_type == DataType::DOUBLE || right_type == DataType::DOUBLE)
                    Error("Cannot perform modulus with DOUBLE type");
                return DataType::INTEGER;
            }
            
            case Lexer::ID_less_than:
            case Lexer::ID_greater_than:
            case Lexer::ID_less_or_eq:
            case Lexer::ID_greater_or_eq:
            case Lexer::ID_equality:
            case Lexer::ID_not_eq:
                return std::max(lhs->GetDataType(), rhs->GetDataType());  // Comparison operators return int, double or char
                
            case Lexer::ID_and:
            case Lexer::ID_or: {
                if (left_type == DataType::DOUBLE || right_type == DataType::DOUBLE)
                    Error("Logical operators can not be applied to double");
                return DataType::INTEGER;
            }
            
            case Lexer::ID_assignment: {
                //std::cout << "Left type: " << left_type << ", right type: " << right_type << "\n";
                if (left_type < right_type)
                    Error("Cannot assign higher precision type to lower precision variable");
                return left_type;
            }
            
            default:
                Error("Unknown operator in GetDataType");
                return DataType::INTEGER;  // To avoid compiler warning
        }
    }
};

class UnaryOpNode : public ASTNode {
private:
    emplex::Token op;
    ASTNode* operand;

public:
    UnaryOpNode(emplex::Token operation) : op(operation) {}

    void SetLeft(ASTNode* node) {
        operand = node;
    }

void GenerateCode() {
    if (op.id == Lexer::ID_negation) {
        VariableNode* varNode = dynamic_cast<VariableNode*>(operand);
        if (varNode) {
            // If the operand is a variable, generate code to negate it
            std::cout << "(i32.const 0)\n";   // Push 0 onto the stack
            std::cout << "(local.get $var" << varNode->GetUniqueId() << ")\n"; // Push variable onto the stack
            std::cout << "(i32.sub)\n";       // Subtract to negate the value
        } else {
            std::cout << "(i32.const 0)\n";   // Push 0 onto the stack
            operand->GenerateCode(); // Generate code for the operand if it's not a variable
            
            std::cout << "(i32.sub)\n";       // Subtract to negate the value
        }
    } else if (op.id == Lexer::ID_not) {
        // Perform logical NOT using (i32.eqz)
        operand->GenerateCode();
        std::cout << "(i32.eqz)\n";
    }

    else if (op.id == Lexer::ID_colon) {
        // Perform type conversion
        // (local.get $var0)    ;; Place var 'val' onto stack
        // (i32.trunc_f64_s) ;; Convert to int.

        // (local.get $var1)    ;; Place var 'floored' onto stack
        // (f64.convert_i32_s) ;; Convert to double.

        operand->GenerateCode();
        if (operand->GetDataType() == DataType::INTEGER) {
            std::cout << "(i32.trunc_f64_s) ;; colon - convert to int\n";
        } 
        else if (operand->GetDataType() == DataType::STRING) {
            std::cout << "(call $char_to_string)\n";
        }
        else {
            std::cout << "(f64.convert_i32_s) ;; colon - convert to double\n";
        }
    }

    else if (op.id == Lexer::ID_sqrt) {
        operand->GenerateCode();
        if (operand->GetDataType() == DataType::INTEGER) {
            std::cout << "(f64.convert_i32_s)\n";
        }
        
        std::cout << "(f64.sqrt)\n";
        //operand->UpdateType(DataType::DOUBLE);
    }
}

    DataType GetDataType() {
        return operand->GetDataType();
    }
};

class IfElseNode : public ASTNode {
private:
    ASTNode* condition = nullptr;
    ASTNode* ifBlock = nullptr;
    ASTNode* elseBlock = nullptr;
    bool returnInIf = false;
    bool returnInElse = false;

public:
    IfElseNode(bool returnInIf, bool returnInElse) : returnInIf(returnInIf), returnInElse(returnInElse) {}

    void setCondition(ASTNode* condition) {
        this->condition = condition;
    }
    void setIfBlock(ASTNode* ifBlock) {
        this->ifBlock = ifBlock;
    }
    void setElseBlock(ASTNode* elseBlock) {
        this->elseBlock = elseBlock;
    }

    DataType GetDataType() {
        if (returnInElse && returnInIf) {
            // Ensure that both branches return the same type
            DataType thenType = ifBlock ? ifBlock->GetDataType() : DataType::INTEGER;
            DataType elseType = elseBlock ? elseBlock->GetDataType() : DataType::INTEGER;
            if (thenType != elseType) {
                Error("Mismatched types in if-else branches");
            }
            return thenType;
        }
        Error("IfElseNode does not have a consistent data type");
        return DataType::INTEGER;
    }

void GenerateCode() {
    // Generate the condition for the `if` statement
    condition->GenerateCode();

    // Check if the if statement returns a value
    std::cout << "(if ";
    if ( returnInIf && returnInElse ) {
        // guaranteed return in both if and else
        std::cout << "(result i32)\n";
    }
    else
        std::cout << "\n";

    std::cout << "(then\n";
    if (ifBlock) {
        ifBlock->GenerateCode();
        if (returnInIf) {
            std::cout << "(return)\n";
        }
        if (returnInIf && !returnInElse) {
            // we don't have a guaranteed return because there is no return in else
            std::cout << "(br $fun_exit1)\n";
        }
    }
    std::cout << ")\n"; // Close 'then' block

    // Generate code for the `else` block if it exists
    if (elseBlock) {
        std::cout << "(else\n";
        elseBlock->GenerateCode();
        if (returnInElse && !returnInIf) {
            // no guaranteed return because there is no return in if
            std::cout << "(br $fun_exit1)\n";
        }
        std::cout << ")\n"; // Close 'else' block
    }

    std::cout << ")\n"; // Close 'if' block
}

};

class BlockNode : public ASTNode {
private:
    std::vector<ASTNode*> statements{};
public:
    BlockNode() {}

    void addStatement(ASTNode* statement) {
        statements.push_back(statement);
    }

    void GenerateCode() {
        for (const auto &s : statements)
            s->GenerateCode();
    }

    DataType GetDataType() {
        Error("BlockNode does not have a data type");
        return DataType::CHAR; // to prevent compiler warnings
    }
};

class WhileNode : public ASTNode {
private:
    ASTNode* condition;
    ASTNode* body;

public:
    WhileNode(ASTNode* condition, ASTNode* body) : condition(condition), body(body) {}

    void GenerateCode() override {
        std::cout << "(block $exit1\n"; // Outer block for breaking the loop
        std::cout << "(loop $loop1\n";  // Inner loop for continuing

        // Generate code for the condition
        condition->GenerateCode();
        std::cout << "(i32.eqz)\n";      // Invert the condition (while condition is true, keep looping)
        std::cout << "(br_if $exit1)\n"; // Exit loop if condition is false

        // Generate code for the body
        if (body) {
            body->GenerateCode();
        }

        // Jump back to the start of the loop
        std::cout << "(br $loop1)\n";
        std::cout << ")\n"; // End of loop
        std::cout << ")\n"; // End of block
    }

    DataType GetDataType() override {
        return DataType::INTEGER; // While loops do not return a specific type
    }
};

class ContinueBreakNode : public ASTNode {
private:
    bool isContinue;
public:
    ContinueBreakNode(bool isContinue) : isContinue(isContinue) {}

    void GenerateCode() {
        if (isContinue) {
            std::cout << "(br $loop1)\n";
        } else {
            std::cout << "(br $exit1)\n";
        }
    }

    DataType GetDataType() {
        return DataType::INTEGER; // Continue/Break do not return a specific type
    }
};

class FunctionCallNode : public ASTNode {
private:
    std::string functionName;
    std::vector<ASTNode*> args;
    
public:
    FunctionCallNode(std::string functionName, DataType returnType, std::vector<ASTNode*> args) :
        functionName(functionName), args(args) {
            this->type = returnType;
        }

    void GenerateCode() {
        for (auto& arg : args) {
            arg->GenerateCode();
        }
        std::cout << "(call $" << functionName << ")\n";
    }

    DataType GetDataType() {
        return type;
    }
};


