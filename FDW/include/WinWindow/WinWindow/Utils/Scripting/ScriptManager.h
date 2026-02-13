#pragma once
#include "../../pch.h"
#include "ScriptValue.h"
#include "ScriptNodes.h"
#include "../Reflection/ReflectionRegistry.h"

class ScriptManager : public FDWWIN::CreativeSingleton<ScriptManager> {

public:
    size_t ExecuteScript(const std::string& script);
    size_t ExecuteFile(const std::string& filepath);
    void Update(float dt);
    float GetDeltaTime() const;

public:
    void SetVariable(const std::string& name, ScriptValue val);
    ScriptValue GetVariable(const std::string& name);

    void RegisterObject(const std::string& name, void* obj, const std::string& className);
    ScriptValue GetProperty(void* obj, const std::string& propName);
    void SetProperty(void* obj, const std::string& propName, ScriptValue val);
    ScriptValue CallMethod(void* obj, const std::string& methodName, const std::vector<ScriptValue>& args);
    
    ScriptValue CreateObject(const std::string& className);
    
    void RegisterFunction(const std::string& name, const std::vector<std::string>& params, std::shared_ptr<ASTNode> body);
    ScriptValue CallFunction(const std::string& name, const std::vector<ScriptValue>& args);

    void AddPredicate(std::shared_ptr<ASTNode> condition, std::shared_ptr<ASTNode> body, bool loop = false);

    void StopAllScripts();
    void StopScript(size_t id);

protected:
    struct ScriptContext {
        std::unordered_map<std::string, ScriptValue> Variables;
        std::unordered_map<std::string, std::shared_ptr<ASTNode>> Functions;
        std::unordered_map<std::string, std::vector<std::string>> FunctionParams;
        std::shared_ptr<ScriptContext> Parent = nullptr;
    };

    std::shared_ptr<ScriptContext> m_pCurrentContext;
    size_t m_uNextScriptId = 1;
    size_t m_uCurrentExecutionScriptId = 0;

    std::unordered_map<std::string, std::pair<void*, const ClassInfo*>> m_mRegisteredObjects;
    
    struct ScriptPredicate {
        std::shared_ptr<ASTNode> Condition;
        std::shared_ptr<ASTNode> Body;
        bool Checked = false;
        bool Loop = false;
        std::shared_ptr<ScriptContext> Context; 
        size_t ScriptId = 0;
    };
    std::vector<ScriptPredicate> m_vPredicates;
    float m_fDeltaTime = 0.0f;
};
