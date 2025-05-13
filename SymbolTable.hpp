#pragma once

#include <assert.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "DataType.hpp"


struct VarData {
  int unique_id;
  DataType type;

  VarData(int unique_id, DataType type) : unique_id(unique_id), type(type) {}
};

class SymbolTable {
private:
  std::vector<std::unordered_map<std::string, int>> scopes;
  std::vector<VarData> variables;
  int unique_id_increment = 0;

  std::unordered_map<std::string, int>& GetCurrentScope() {
    return scopes.back();
  }

public:
  SymbolTable() {
    PushScope(); // Initialize with the global scope
  }

  bool HasVarInCurrentScope(const std::string& name) {
    return GetCurrentScope().find(name) != GetCurrentScope().end();
  }

  bool HasVar(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      if (it->find(name) != it->end())
        return true;
    }
    return false;
  }

  DataType GetDataType(int unique_id) const {
    for (const auto& varData : variables) {
      if (varData.unique_id == unique_id)
        return varData.type;
    }
    return DataType::INTEGER; // Error case, should't be reachable (to prevent warnings)
  }

  int GetUniqueId(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end())
        return found->second;
    }
    Error("Variable not defined: " + name);
    return -1;
  }

  int InitializeVar(const std::string& name, const DataType& type) {
    if (HasVarInCurrentScope(name)) {
      Error("Variable already defined in this scope: " + name);
    }

    GetCurrentScope()[name] = unique_id_increment;
    variables.emplace_back(unique_id_increment, type); // New variable with default value 0
    return unique_id_increment++;
  }

  void PushScope() {
    scopes.emplace_back();
  }

  void PopScope() {
    if (scopes.size() > 1) {
      scopes.pop_back();
    } else {
      Error("No scope to pop");
    }
  }


  /*
  This function tells us if we are inside any of the scopes.
  This is needed because if we return from inside if or while, 
  we need to add a "break" statement in our code.
  This can be achieved by checking if we are inside any of the scopes (excluding main function scope).
  */
  bool InsideTheScope() {
    return scopes.size() > 1;
  }
};