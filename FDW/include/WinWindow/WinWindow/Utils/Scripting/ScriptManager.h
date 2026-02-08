#pragma once
#include "../../pch.h"
#include "ScriptValue.h"
#include "ScriptNodes.h"
#include "../Reflection/ReflectionRegistry.h"

class ScriptManager : public FDWWIN::CreativeSingleton<ScriptManager> {
public:
    void ExecuteScript(const std::string& script);
    void Update(); 

public:
    void SetVariable(const std::string& name, ScriptValue val);
    ScriptValue GetVariable(const std::string& name);

    void RegisterObject(const std::string& name, void* obj, const std::string& className);
    ScriptValue GetProperty(void* obj, const std::string& propName);
    void SetProperty(void* obj, const std::string& propName, ScriptValue val);
    ScriptValue CallMethod(void* obj, const std::string& methodName, const std::vector<ScriptValue>& args);
    
    ScriptValue CreateObject(const std::string& className);

    void AddPredicate(std::shared_ptr<ASTNode> condition, std::shared_ptr<ASTNode> body);

private:
    std::unordered_map<std::string, ScriptValue> m_mVariables;
    std::unordered_map<std::string, std::pair<void*, const ClassInfo*>> m_mRegisteredObjects;
    
    struct ScriptPredicate {
        std::shared_ptr<ASTNode> Condition;
        std::shared_ptr<ASTNode> Body;
        bool Checked = false;
        bool Loop = false;
    };
    std::vector<ScriptPredicate> m_vPredicates;
};
