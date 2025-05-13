#pragma once
#include "TokenQueue.hpp"
#include <vector>
#include "ASTNode.hpp"
#include "Function.hpp"
#include "SymbolTable.hpp"
#include <iostream>
#include <string>

using namespace emplex;

// source - https://github.com/MSU-CSE450/project-3-tubular-icterines/

class Parser {
private:
    TokenQueue tokens;
    std::vector<Function*> functions{};
    SymbolTable* symbols = nullptr;
    Function* currentFunction = nullptr;

    bool pastReturnInMainScope = false; // for error test case 7 (project 4)

    // we need these two to track if we have a guaranteed return in if-else clause
    bool returnInIf = false;
    bool returnInElse = false;
    // used for error detection on returns in if-else
    bool insideIf = false;
    // used for error detection on returns in continue/break statements
    std::vector<bool> insideWhileStack;

    // Index 0 indicates if we are in the stack, Index 1 indicates if we have reached the right-end of the chained assignments and are working our way back left
    // std::vector<bool> insideAssigmentStack = {false, false};
    bool assignmentStackOccurs = false;
    int insideAssignmentStack = 0;

    // counts number of returns help with knowing if there is a guaranteed return in a function
    int numReturns = 0;

    // index to a free memory position
    int memoryPos = 0;

    void addReturnIfElse() {
        if (!returnInIf) {
            returnInIf = true;
            return;
        }
        returnInElse = true;
    }

    void parseFunction() {
        symbols = new SymbolTable(); // create new SymbolTable for every function
        tokens.Use(Lexer::ID_function, "Expected function declaration");

        currentFunction = parseFunctionSignature();

        tokens.Use(Lexer::ID_open_brace);
        parseFunctionBody(currentFunction); // parse everything between { }
        tokens.Use(Lexer::ID_close_brace);
        if (returnInIf && numReturns < 2)
            Error("Function does not have a guaranteed return");

        functions.push_back(currentFunction);
        pastReturnInMainScope = false; // reset for a new function
    }

    Function* parseFunctionSignature() {
        auto function_identifier = tokens.Use(Lexer::ID_identifier, "Expected function identifier");
        tokens.Use(Lexer::ID_open_parenthesis);
        
        // arguments
        std::vector<std::pair<DataType, int>> args;
        while (tokens.Peek().id != Lexer::ID_close_parenthesis)
        {
            DataType type = UseDataType();
            Token identifier = tokens.Use(Lexer::ID_identifier);
            int variable_unique_id = symbols->InitializeVar(identifier.lexeme, type);
            args.push_back({type, variable_unique_id});
            tokens.UseIf(Lexer::ID_comma); 
        }

        tokens.Use(Lexer::ID_close_parenthesis);
        tokens.Use(Lexer::ID_colon);

        DataType returnType = UseDataType();

        Function* currentFunction = new Function(functions.size()+1, function_identifier, returnType, args);
        return currentFunction;
    }

    void parseFunctionBody(Function* currentFunction) {
        while (tokens.Peek().id != Lexer::ID_close_brace)
        {
            ASTNode* node = ParseStatement();

            if (node != nullptr)
                currentFunction->AddNode(node);
        }
    }

    ASTNode* parseWhile() {
        insideWhileStack.push_back(true);
        tokens.Use(Lexer::ID_while); // Consume 'while' keyword
        tokens.Use(Lexer::ID_open_parenthesis); // Consume '('
        
        // Parse the condition expression
        ASTNode* condition = parseLogical();
        tokens.Use(Lexer::ID_close_parenthesis); // Consume ')'

        // Parse the loop body
        ASTNode* body = nullptr;
        if (tokens.Peek().id == Lexer::ID_open_brace) {
            body = parseBlock(); // Block enclosed in braces
        } else {
            symbols->PushScope();
            body = ParseStatement(); // Single statement as body
            symbols->PopScope();
        }

        insideWhileStack.pop_back();

        // Create a WhileNode with the condition and body
        return new WhileNode(condition, body);
    }

    ASTNode* parseIndexing(VariableNode* variable = nullptr) {
        int unique_id;
        DataType varType;

        if (variable == nullptr) {
            Token variableToken = tokens.Use(Lexer::ID_identifier);
            unique_id = symbols->GetUniqueId(variableToken.lexeme);
            varType = symbols->GetDataType(unique_id);
        }
        else {
            unique_id = variable->GetUniqueId();
            varType = variable->GetDataType();
        }

        // Ensure the variable is of type STRING
        if (varType != DataType::STRING) {
            Error("Indexing is only supported on strings");
        }

        tokens.Use(Lexer::ID_open_bracket); // Consume '['

        // Parse the index expression (should result in an integer)
        ASTNode* indexExpression = parseExpression();

        // Ensure the index expression is of type INTEGER
        if (indexExpression->GetDataType() != DataType::INTEGER) {
            Error("Index expression must be of type int");
        }

        tokens.Use(Lexer::ID_close_bracket); // Consume ']'

        // Create and return an IndexNode
        VariableNode* variableNode = new VariableNode(unique_id, varType);
        return new IndexNode(variableNode, indexExpression);
    }



    ASTNode* ParseStatement() {

        if (pastReturnInMainScope)
            Error("Statement encountered after return in the main scope"); // error test 7
        
        switch(tokens.Peek().id) {
            case Lexer::ID_return: {
                if (insideIf) // prevents any return token from triggering the boolean for if-else returns
                    addReturnIfElse(); // signal that we have encountered a return (needed for if-else)

                ASTNode* expression = ParseReturn();
                tokens.Use(Lexer::ID_semicolon);
                return expression;
                break;
            }
            case Lexer::ID_if: {
                ASTNode* expression = parseIf();
                return expression;
            }
            case Lexer::ID_while: {
                return parseWhile();
            }
            case Lexer::ID_identifier: {
                ASTNode* expression = parseAssignment();
                return expression;
            }
            case Lexer::ID_open_brace: {
                ASTNode* expression = parseBlock();
                return expression;
            }
            case Lexer::ID_int:
            case Lexer::ID_double:
            case Lexer::ID_char:
            case Lexer::ID_string_keyword: {
                // might return nullptr if a variable is initialized but not declared
                ASTNode* assignmentNode = parseVariableDeclaration();
                return assignmentNode;
            }

            

            case Lexer::ID_continue: {
                if (size(insideWhileStack) == 0) {
                    Error("Continue statement outside of a loop");
                    return nullptr;
                }
                ASTNode* node = new ContinueBreakNode(true);
                tokens.Use(Lexer::ID_continue);
                tokens.Use(Lexer::ID_semicolon);
                return node;
            }
            case Lexer::ID_break: {
                if (size(insideWhileStack) == 0) {
                    Error("Break statement outside of a loop");
                    return nullptr;
                }
                ASTNode* node = new ContinueBreakNode(false);
                tokens.Use(Lexer::ID_break);
                tokens.Use(Lexer::ID_semicolon);
                return node;
            }
            default:
                Error("Encountered inappropriate lexeme in ParseStatement: " + tokens.Peek().lexeme + "\n");
                return nullptr;

        }
    }

    ASTNode* ParseReturn() {
        numReturns++;
        // for return we just need to put the result of the expression on top of the stack
        tokens.Use();
        ASTNode* expression = parseLogical();
        ReturnNode* node = new ReturnNode(expression);

        if (!symbols->InsideTheScope())
            pastReturnInMainScope = true;
        return node;
    }

    ASTNode* parseFunctionCall() {
        Token identifier = tokens.Use(Lexer::ID_identifier);
        
        Function* functionToCall = nullptr;
        for (auto func : functions) {
            if (func->GetFunctionName() == identifier.lexeme) {
                functionToCall = func;
                break;
            }
        }

        if (functionToCall == nullptr) {
            Error("Function " + identifier.lexeme + " does not exist");
        }
        
        tokens.Use(Lexer::ID_open_parenthesis);
        
        // parse arguments in a function call
         std::vector<ASTNode*> args{};
        while (tokens.Peek().id != Lexer::ID_close_parenthesis) {
            ASTNode* expression = parseExpression();
            args.push_back(expression);

            tokens.UseIf(Lexer::ID_comma);
        }

        auto functionArgs = functionToCall->getArgs();
        if (args.size() != static_cast<std::size_t>(functionToCall->GetNumberOfArguments())) {
            Error("Incorrect number of arguments were provided to the function " + functionToCall->GetFunctionName());
        }

        // type checking (doesn't work properly)
        for (int i = 0; i < args.size(); i++) {
            if (args[i]->GetDataType() != functionArgs[i].first) {
                Error("Argument type mismatch in a function call");
            }
        }

        tokens.Use(Lexer::ID_close_parenthesis);
        
        return new FunctionCallNode(functionToCall->GetFunctionName(), functionToCall->GetReturnType(), args);
    }

    ASTNode* parseIdentifier() {
        // if next lexeme is "(" - parse function call

        if (tokens.PeekByIndex(1).id == Lexer::ID_open_parenthesis) {
            return parseFunctionCall();
        }

        // otherwise parse it as a variable
        Token variable = tokens.Use(Lexer::ID_identifier);
        int unique_id = symbols->GetUniqueId(variable.lexeme);
        VariableNode* variableNode = new VariableNode(unique_id, symbols->GetDataType(unique_id));

        return variableNode;

    }

    ASTNode* parseAnd() {
        ASTNode* node = parseComparison();
        while (tokens.Peek().id == Lexer::ID_and) {
            emplex::Token and_op = tokens.Use();
            ASTNode* right_node = parseComparison();
            BinaryOpNode* and_node = new BinaryOpNode(and_op);
            and_node->SetLeft(node);
            and_node->SetRight(right_node);
            node = and_node;
        }
        return node;
    }


    ASTNode* parseAssignment() {
        Token variable = tokens.Use(Lexer::ID_identifier);
        int unique_id = symbols->GetUniqueId(variable.lexeme);
        VariableNode* variableNode = new VariableNode(unique_id, symbols->GetDataType(unique_id));

        ASTNode* indexNode = nullptr;

        if (tokens.Peek().id == Lexer::ID_open_bracket) {
            indexNode = parseIndexing(variableNode);
        }

        Token equalityOp = tokens.Use(Lexer::ID_assignment);
        ASTNode* expression = parseLogical();
        BinaryOpNode* assignmentNode = new BinaryOpNode(equalityOp);
        
        assignmentNode->SetLeft(variableNode);
        assignmentNode->SetRight(expression);
        assignmentNode->SetIndex(indexNode);

        tokens.Use(Lexer::ID_semicolon);
        return assignmentNode;
    }

    ASTNode* parseBlock() {
        symbols->PushScope();

        tokens.Use(Lexer::ID_open_brace);
        BlockNode* block = new BlockNode();
        while (tokens.Peek().id != Lexer::ID_close_brace) {
            ASTNode* statement = ParseStatement();
            block->addStatement(statement);
        }
        tokens.Use(Lexer::ID_close_brace);

        symbols->PopScope();
        return block;
    }
    ASTNode* parseIf() {
        insideIf = true;
        tokens.Use(Lexer::ID_if);
        tokens.Use(Lexer::ID_open_parenthesis);
        ASTNode* condition = parseLogical();
        tokens.Use(Lexer::ID_close_parenthesis);
        returnInElse = false;
        returnInIf = false;



        ASTNode* ifBlock = nullptr;
        if (tokens.Peek().id == Lexer::ID_open_brace) {
            auto numReturnsBefore = numReturns;
            ifBlock = parseBlock();
            if (numReturns > numReturnsBefore) {
                returnInIf = true;
            }
        } else {
            // single-line if
            symbols->PushScope();
            
            ifBlock = ParseStatement();
            symbols->PopScope();
        }

        ASTNode* elseBlock = nullptr;
        if (tokens.Peek().id == Lexer::ID_else) {
            tokens.Use(Lexer::ID_else);

            if (tokens.Peek().id == Lexer::ID_open_brace) {
                elseBlock = parseBlock();
            } else {
                symbols->PushScope();
                elseBlock = ParseStatement();
                symbols->PopScope();
            }
            
        }

        IfElseNode* node = new IfElseNode(returnInIf, returnInElse);
        node->setCondition(condition);
        node->setIfBlock(ifBlock);
        node->setElseBlock(elseBlock);

        insideIf = false;

        return node;
    }


    ASTNode* parseLogical() {
        ASTNode* node = parseAnd();

        while (tokens.Peek().id == Lexer::ID_or) {
            emplex::Token or_op = tokens.Use();
            ASTNode* right_node = parseAnd();
            BinaryOpNode* or_node = new BinaryOpNode(or_op);
            or_node->SetLeft(node);
            or_node->SetRight(right_node);
            node = or_node;
        }

        return node;
    }

    ASTNode* parseComparison() {
        int binary_node_count = 0;

        ASTNode* node = parseExpression();
        
        while (
            (tokens.Peek().id == Lexer::ID_equality || 
            tokens.Peek().id == Lexer::ID_not_eq || 
            tokens.Peek().id == Lexer::ID_greater_than || 
            tokens.Peek().id == Lexer::ID_greater_or_eq || 
            tokens.Peek().id == Lexer::ID_less_than || 
            tokens.Peek().id == Lexer::ID_less_or_eq)) {

            emplex::Token comparison_op = tokens.Use();
            ASTNode* right_node = parseExpression();
            
            BinaryOpNode* binary_op_node = new BinaryOpNode(comparison_op);
            binary_node_count++;

            if (binary_node_count > 1) {
                Error("Comparisons should be non-associative.");
            }

            binary_op_node->SetLeft(node);
            binary_op_node->SetRight(right_node);
            node = binary_op_node;
        }
        return node;
    }
    ASTNode* parseExpression() {
        ASTNode* node = parseTerm();
        while (tokens.Peek().id == Lexer::ID_add || 
            tokens.Peek().id == Lexer::ID_negation) {
            emplex::Token binary_op = tokens.Use();
            ASTNode* right_node = parseTerm();

            // Type-checking for invalid operations with string
            DataType leftType = node->GetDataType();
            DataType rightType = right_node->GetDataType();
            if ((leftType == DataType::STRING || rightType == DataType::STRING) && (leftType == DataType::INTEGER || rightType == DataType::INTEGER)) {
                Error("Cannot perform operations between strings and integers");
                return nullptr;
            }

            BinaryOpNode* binary_op_node = new BinaryOpNode(binary_op);
            binary_op_node->SetLeft(node);
            binary_op_node->SetRight(right_node);
            node = binary_op_node;
        }

        return node;
    }

    ASTNode* parseTerm() {
        ASTNode* node = parseFactor();
        
        while (tokens.Peek().id == Lexer::ID_multiply || 
            tokens.Peek().id == Lexer::ID_divide || 
            tokens.Peek().id == Lexer::ID_modulus
            ) {
            
            emplex::Token binary_op = tokens.Use();
            ASTNode* right_node = parseFactor();

            // Type-checking for invalid operations with modulus
            if (binary_op.id == Lexer::ID_modulus) {
                DataType leftType = node->GetDataType();
                DataType rightType = right_node->GetDataType();
                
                // Ensure both operands are integers for modulus
                if (leftType != DataType::INTEGER || rightType != DataType::INTEGER) {
                    Error("Error: Modulus operator can only be applied to integers\n");
                    return nullptr;
                }
            }

            if (binary_op.id == Lexer::ID_multiply) {
                DataType leftType = node->GetDataType();
                DataType rightType = right_node->GetDataType();
                
                // Ensure both operands are correct types for multiplication
                if (leftType == DataType::CHAR && rightType == DataType::CHAR) {
                    Error("Error: Multiply operator can only be applied between two integers or between a char and an integer\n");
                    return nullptr;
                }

                if ((leftType == DataType::CHAR && rightType == DataType::DOUBLE) || (leftType == DataType::DOUBLE && rightType == DataType::CHAR)) {
                    Error("Error: Multiply operator can only be applied between two integers or between a char and an integer\n");
                    return nullptr;
                }
                
            }

            if (binary_op.id == Lexer::ID_divide) {
                DataType leftType = node->GetDataType();
                DataType rightType = right_node->GetDataType();
                
                // Ensure both operands are correct types for division
                if (leftType == DataType::CHAR && rightType == DataType::CHAR) {
                    Error("Error: Divide operator can only be applied between two integers or between a char and an integer\n");
                    return nullptr;
                }

                if (leftType == DataType::CHAR || rightType == DataType::CHAR) {
                    Error("Error: Divide operator can only be applied between two integers or between a char and an integer\n");
                    return nullptr;
                }
            }

            BinaryOpNode* binary_op_node = new BinaryOpNode(binary_op);
            binary_op_node->SetLeft(node);
            binary_op_node->SetRight(right_node);
            node = binary_op_node;
        }

        return node;
    }


    ASTNode* parseFactor() {
        // Handle logical NOT '!'
        if (tokens.Peek().id == Lexer::ID_not) {
            emplex::Token not_token = tokens.Use();
            ASTNode* operand = parseFactor();
            UnaryOpNode* not_node = new UnaryOpNode(not_token);
            not_node->SetLeft(operand);
            return not_node;
        }
        
        return parsePrimary();
    }
    
    ASTNode* parsePrimary() {
        // Handle unary negation
        if (tokens.Peek().id == Lexer::ID_negation) {
            emplex::Token negation_token = tokens.Use();
            ASTNode* operand = parsePrimary();
            UnaryOpNode* negation_node = new UnaryOpNode(negation_token);
            negation_node->SetLeft(operand);
            return negation_node;
        }

        // Handle logical NOT
        if (tokens.Peek().id == Lexer::ID_not) {
            emplex::Token not_token = tokens.Use();
            ASTNode* operand = parsePrimary();
            UnaryOpNode* not_node = new UnaryOpNode(not_token);
            not_node->SetLeft(operand);
            return not_node;
        }

        // Handle parentheses (e.g., (expr))
        if (tokens.Peek().id == Lexer::ID_open_parenthesis) {
            tokens.Use(Lexer::ID_open_parenthesis); // Consume '('
            ASTNode* expression = parseExpression();
            tokens.Use(Lexer::ID_close_parenthesis); // Ensure there's a closing ')'
            return expression;
        }

        if (tokens.Peek().lexeme == "size" && tokens.PeekByIndex(1).id == Lexer::ID_open_parenthesis) {
            tokens.Use(); // Consume "size"
            tokens.Use(Lexer::ID_open_parenthesis); // Consume '('

            // Parse the string argument
            ASTNode* arg = parseExpression();

            // Ensure the argument is of type STRING
            if (arg->GetDataType() != DataType::STRING) {
                Error("size() can only be applied to a string");
            }

            tokens.Use(Lexer::ID_close_parenthesis); // Consume ')'

            // Create a FunctionCallNode for $get_length
            return new FunctionCallNode("get_length", DataType::INTEGER, {arg});
        }

        // Handle variables and assignments
        if (tokens.Peek().id == Lexer::ID_identifier) {
        
            if (tokens.PeekByIndex(1).id == Lexer::ID_open_bracket) {
                return parseIndexing();
            }

            // if not a function call
            if (tokens.PeekByIndex(1).id != Lexer::ID_open_parenthesis) {
                int unique_id = symbols->GetUniqueId(tokens.Peek().lexeme);
                tokens.Use();

                // handle conversions between int, char, and double before anything else happens
                return parseConversion(new VariableNode(unique_id, symbols->GetDataType(unique_id)));
            }

            // if a function call
            return parseConversion(parseFunctionCall());
            
        }

        // Handle numeric literals and characters
        if (tokens.Peek().id == Lexer::ID_integer || tokens.Peek().id == Lexer::ID_float || tokens.Peek().id == Lexer::ID_character) {
            Token token = tokens.Use();
            return MakeLiteralValueNode(token);
        }

        // Handle strings
        if (tokens.Peek().id == Lexer::ID_literal_string) {;
            return MakeLiteralStringNode();
        }

        if (tokens.Peek().id == Lexer::ID_sqrt) {
            Token sqrt_op = tokens.Use(Lexer::ID_sqrt);
            tokens.Use(Lexer::ID_open_parenthesis);
            Token token = tokens.Use();
            int unique_id = symbols->GetUniqueId(token.lexeme);
            auto variable = new VariableNode(unique_id, symbols->GetDataType(unique_id));
            tokens.Use(Lexer::ID_close_parenthesis);
            UnaryOpNode* SqrtNode = new UnaryOpNode(sqrt_op);
            SqrtNode->SetLeft(variable);
            return parseConversion(SqrtNode);
        }

        Error("Unexpected token in parsePrimary ", tokens.Peek().lexeme);
        return nullptr;
    }

    ASTNode* parseConversion(ASTNode* node) {
        if (tokens.Peek().id == Lexer::ID_colon) {
            auto colon_token = tokens.Use();
            if (tokens.Peek().id == Lexer::ID_int || 
                tokens.Peek().id == Lexer::ID_double || 
                tokens.Peek().id == Lexer::ID_char ||
                tokens.Peek().id == Lexer::ID_string_keyword) {
                DataType conversion = UseDataType();

                UnaryOpNode* conversionNode = new UnaryOpNode(colon_token);

                
                node->UpdateType(conversion);
                
                conversionNode->SetLeft(node);
                return conversionNode;
            }
        }
        return node; // no conversion
    }
    ASTNode* MakeLiteralValueNode(Token token) {
        switch (token.id) {
            case Lexer::ID_integer: {
                int value = std::stoi(token.lexeme);
                return new LiteralValueNode(DataType::INTEGER, value);
            }
            case Lexer::ID_float: {
                double value = std::stod(token.lexeme);
                return new LiteralValueNode(DataType::DOUBLE, value);
            }
            case Lexer::ID_character: {
                char value = token.lexeme[1];
                return new LiteralValueNode(DataType::CHAR, value);
            }
            default:
                Error("Unknown token in MakeLiteralValueNode", token.lexeme, "\n");
                return nullptr;
        }
    }
    DataType UseDataType() {
        Token token = tokens.Peek();
        DataType type;
        
        switch (token.id) {
            case Lexer::ID_int:
                type = DataType::INTEGER;
                tokens.Use();
                break;
            case Lexer::ID_double:
                type = DataType::DOUBLE;
                tokens.Use();
                break;
            case Lexer::ID_char:
                type = DataType::CHAR;
                tokens.Use();
                break;
            case Lexer::ID_string_keyword:
                type = DataType::STRING;
                tokens.Use();
                break;
            default:
                Error(token.line_id, "Expected int, char, double or string");
                break;
        }
        return type;
    }

    ASTNode* MakeLiteralStringNode() {
        std::string str = tokens.Use().lexeme;
        str = str.substr(1, str.length() - 2); // remove ""
        
        ASTNode* stringNode = new LiteralStringNode(memoryPos, str.length());
        memoryPos += str.length() + 1; // update free memory index

        Function::addLiteralString(str);
        return stringNode;
    }

    ASTNode* parseVariableDeclaration() {
        switch (tokens.Peek().id)
        {
            case Lexer::ID_int: {
                tokens.Use(Lexer::ID_int);
                Token identifier = tokens.Peek();
                int unique_id = symbols->InitializeVar(identifier.lexeme, DataType::INTEGER);

                // add a new variable to the function local variables list
                currentFunction->AddLocalVar({DataType::INTEGER, unique_id});
                
                // just initialization or also declaration?
                // int a;
                if (tokens.PeekByIndex(1).id == Lexer::ID_semicolon) {
                    tokens.Use(Lexer::ID_identifier);
                    tokens.Use(Lexer::ID_semicolon);
                    return nullptr;
                }

                // if declaration - call parseAssignment
                ASTNode* assignment = parseAssignment();
                return assignment;


            }

            case Lexer::ID_double: {
                tokens.Use(Lexer::ID_double);
                Token identifier = tokens.Peek();
                int unique_id = symbols->InitializeVar(identifier.lexeme, DataType::DOUBLE);

                // add a new variable to the function local variables list
                currentFunction->AddLocalVar({DataType::DOUBLE, unique_id});
                
                // just initialization or also declaration?
                // double a;
                if (tokens.PeekByIndex(1).id == Lexer::ID_semicolon) {
                    tokens.Use(Lexer::ID_identifier);
                    tokens.Use(Lexer::ID_semicolon);
                    return nullptr;
                }

                // if declaration - call parseAssignment
                ASTNode* assignment = parseAssignment();
                return assignment;
            }

            case Lexer::ID_string_keyword: {
                tokens.Use(Lexer::ID_string_keyword);
                Token identifier = tokens.Peek();
                int unique_id = symbols->InitializeVar(identifier.lexeme, DataType::STRING);

                currentFunction->AddLocalVar({DataType::STRING, unique_id});

                if (tokens.Peek().id == Lexer::ID_assignment) {
                    tokens.Use(Lexer::ID_assignment);
                    ASTNode* expression = parseExpression();

                    // Ensure the expression is of type STRING
                    if (expression->GetDataType() != DataType::STRING) {
                        Error("Cannot assign non-string value to a string variable");
                    }

                    // Create assignment node
                    VariableNode* variableNode = new VariableNode(unique_id, DataType::STRING);
                    Token assignmentToken = tokens.Peek();
                    BinaryOpNode* assignmentNode = new BinaryOpNode(assignmentToken);
                    assignmentNode->SetLeft(variableNode);
                    assignmentNode->SetRight(expression);
                    tokens.Use(Lexer::ID_semicolon);
                    return assignmentNode;
                }
                
                // just initialization or also declaration?
                // double a;
                if (tokens.PeekByIndex(1).id == Lexer::ID_semicolon) {
                    tokens.Use(Lexer::ID_identifier);
                    tokens.Use(Lexer::ID_semicolon);
                    return nullptr;
                }

                // if declaration - call parseAssignment
                ASTNode* assignment = parseAssignment();
                return assignment;
            }
        
        default:
            Error("parseVariableDeclaration is implemented only for int, double, and string");
            return nullptr; // to prevent warnings
            break;
        }
    }

public:
    Parser(TokenQueue tokens) : tokens(tokens) {}
    std::vector<Function*> Parse() {
        while (tokens.Any()) {
            parseFunction();
        }
        return functions;
    }
};