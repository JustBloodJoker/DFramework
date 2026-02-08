#include "ScriptManager.h"
#include "ScriptTokenType.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"
#include "../Serializer/PolymorphicFactory.h"


void ScriptManager::ExecuteScript(const std::string& script) {
    ScriptLexer lexer(script);
    auto tokens = lexer.Tokenize();
    ScriptParser parser(tokens);
    auto block = parser.ParseBlock();

	parser.Flush();
    while (parser.Peek().Type != END) {
        auto stmt = parser.ParseStatement();
        if(stmt) stmt->Execute(*this);
    }
}

void ScriptManager::Update() {
    for (auto it = m_vPredicates.begin(); it != m_vPredicates.end(); ) {
        if ( it->Condition->Execute(*this).AsBool() ) {
            it->Body->Execute(*this);
            it->Checked = true;
            
            if (it->Loop) {
                ++it;
            } else {
                it = m_vPredicates.erase(it);
            }

        } else {
            ++it;
        }
    }
}

void ScriptManager::SetVariable(const std::string& name, ScriptValue val) {
    m_mVariables[name] = val;
}
ScriptValue ScriptManager::GetVariable(const std::string& name) {
    if ( m_mVariables.find(name) != m_mVariables.end() ) return m_mVariables[name];

    if (m_mRegisteredObjects.find(name) != m_mRegisteredObjects.end()) {
        return ScriptValue(m_mRegisteredObjects[name].first);
    }
    return 0; 
}

void ScriptManager::RegisterObject(const std::string& name, void* obj, const std::string& className) {
    auto* info = ReflectionRegistry::GetInstance()->GetReflectedClassInfo(className);
    if (info) {
        m_mRegisteredObjects[name] = std::make_pair(obj, info);
    }
}

ScriptValue ScriptManager::GetProperty(void* obj, const std::string& propName) {
    for (auto& [name, pair] : m_mRegisteredObjects) {
        if (pair.first == obj) {
            void* currentObj = obj;
            auto* prop = pair.second->FindProperty(propName, currentObj);
            if (prop && prop->Getter) {
                return prop->Getter(currentObj);
            }
        }
    }
    return 0;
}

void ScriptManager::SetProperty(void* obj, const std::string& propName, ScriptValue val) {
    for (auto& [name, pair] : m_mRegisteredObjects) {
        if (pair.first == obj) {
            void* currentObj = obj;
            auto* prop = pair.second->FindProperty(propName, currentObj);
            if (prop && prop->Setter) {
                prop->Setter(currentObj, val);
            }
        }
    }
}

ScriptValue ScriptManager::CallMethod(void* obj, const std::string& methodName, const std::vector<ScriptValue>& args) {
    for (auto& [name, pair] : m_mRegisteredObjects) {
        if (pair.first == obj) {
             void* currentObj = obj;
             auto* method = pair.second->FindMethod(methodName, currentObj);
             if (method) {
                 std::vector<std::variant<int, float, std::string, void*>> funcArgs;
                 for(auto& a : args) funcArgs.push_back(a.Data);
                 
                 std::variant<int, float, std::string, void*> ret;
                 method->Invoke(currentObj, funcArgs, ret);
                 
                 ScriptValue retVal;
                 retVal.Data = ret;
                 return retVal;
             }
        }
    }
    return 0;
}

ScriptValue ScriptManager::CreateObject(const std::string& className) {
    void* obj = PolymorphicFactory::GetInstance()->Create(className);
    if (!obj) return 0;
    
    static int anonymousId = 0;
    std::string name = "__anon_" + std::to_string(anonymousId++);
    
    RegisterObject(name, obj, className);
    return ScriptValue(obj);
}

void ScriptManager::AddPredicate(std::shared_ptr<ASTNode> condition, std::shared_ptr<ASTNode> body) {
    m_vPredicates.push_back({ condition, body });
}
