#include "ScriptManager.h"
#include "ScriptTokenType.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"
#include "../Serializer/PolymorphicFactory.h"

bool IsPureExpressionNode(const std::shared_ptr<ASTNode>& node) {
    if (!node) return false;

    if (dynamic_cast<LiteralNode*>(node.get()) != nullptr) return true;

    if (dynamic_cast<VarNode*>(node.get()) != nullptr) return true;

    if (auto* member = dynamic_cast<MemberAccessNode*>(node.get())) {
        return IsPureExpressionNode(member->Object);
    }

    if (auto* arrayLiteral = dynamic_cast<ArrayLiteralNode*>(node.get())) {
        for (const auto& e : arrayLiteral->Elements) {
            if (!IsPureExpressionNode(e)) return false;
        }
        return true;
    }

    if (auto* arrayAccess = dynamic_cast<ArrayAccessNode*>(node.get())) {
        return IsPureExpressionNode(arrayAccess->ArrayExpr) && IsPureExpressionNode(arrayAccess->IndexExpr);
    }

    if (auto* binary = dynamic_cast<BinaryOpNode*>(node.get())) {
        return IsPureExpressionNode(binary->Left) && IsPureExpressionNode(binary->Right);
    }

    return false;
}

bool IsSafeStatementForPredicateRestore(const std::shared_ptr<ASTNode>& node) {
    if (!node) return false;

    if (dynamic_cast<FunctionDefNode*>(node.get()) != nullptr) return true;

    if (dynamic_cast<PredicateRegisterNode*>(node.get()) != nullptr) return true;

    if (auto* assign = dynamic_cast<AssignNode*>(node.get())) {
        return assign->MemberAccess == nullptr && IsPureExpressionNode(assign->Expr);
    }

    return false;
}

std::vector<std::shared_ptr<ASTNode>> ParseStatements(const std::string& script) {
    ScriptLexer lexer(script);
    auto tokens = lexer.Tokenize();
    ScriptParser parser(tokens);
    parser.ParseBlock();
    parser.Flush();

    std::vector<std::shared_ptr<ASTNode>> statements;
    while (parser.Peek().Type != END) {
        auto stmt = parser.ParseStatement();
        if (stmt) {
            statements.push_back(stmt);
        }
        else {
            parser.Advance();
        }
    }
    return statements;
}

size_t ScriptManager::ExecuteScript(const std::string& script, ScriptExecutionMode mode) {
    auto statements = ParseStatements(script);

    auto prevContext = m_pCurrentContext;
    m_pCurrentContext = std::make_shared<ScriptContext>();
    
    auto prevId = m_uCurrentExecutionScriptId;
    auto newId = m_uNextScriptId++;
    m_uCurrentExecutionScriptId = newId;

    for (const auto& stmt : statements) {
        if (!stmt) continue;

        if (mode == ScriptExecutionMode::PredicatesOnly && !IsSafeStatementForPredicateRestore(stmt)) continue;

        stmt->Execute(*this);
    }
    
    m_pCurrentContext = prevContext;
    m_uCurrentExecutionScriptId = prevId;
    
    return newId;
}

size_t ScriptManager::ExecuteFile(const std::string& filepath, ScriptExecutionMode mode) {
    std::ifstream t(filepath);
    if (!t.is_open()) return 0;

    std::stringstream buffer;
    buffer << t.rdbuf();
    return ExecuteScript(buffer.str(), mode);
}

void ScriptManager::Update(float dt) {
    m_fDeltaTime = dt;
    auto prevContext = m_pCurrentContext;

    for (auto it = m_vPredicates.begin(); it != m_vPredicates.end(); ) {
        m_pCurrentContext = it->Context;

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
    m_pCurrentContext = prevContext;
}

float ScriptManager::GetDeltaTime() const {
    return m_fDeltaTime;
}

void ScriptManager::ClearRegisteredObjects() {
    m_mRegisteredObjects.clear();
}

void ScriptManager::SetVariable(const std::string& name, ScriptValue val) {
    if (!m_pCurrentContext) return;
    
    auto ctx = m_pCurrentContext;
    while (ctx) {
        if (ctx->Variables.find(name) != ctx->Variables.end()) {
            ctx->Variables[name] = val;
            return;
        }

        ctx = ctx->Parent;
    }
    m_pCurrentContext->Variables[name] = val;
}

ScriptValue ScriptManager::GetVariable(const std::string& name) {
    auto ctx = m_pCurrentContext;
    while (ctx) {
        if (ctx->Variables.find(name) != ctx->Variables.end()) return ctx->Variables[name];

        ctx = ctx->Parent;
    }

    if (m_mRegisteredObjects.find(name) != m_mRegisteredObjects.end()) {
        return ScriptValue(m_mRegisteredObjects[name].first);
    }
    return 0; 
}

void ScriptManager::RegisterFunction(const std::string& name, const std::vector<std::string>& params, std::shared_ptr<ASTNode> body) {
    if (m_pCurrentContext) {
        m_pCurrentContext->Functions[name] = body;
        m_pCurrentContext->FunctionParams[name] = params;
    }
}

ScriptValue ScriptManager::CallFunction(const std::string& name, const std::vector<ScriptValue>& args) {
    auto ctx = m_pCurrentContext;
    while (ctx) {
        if (ctx->Functions.find(name) != ctx->Functions.end()) {
            auto funBody = ctx->Functions[name];
            auto& params = ctx->FunctionParams[name];
            
            auto fnCtx = std::make_shared<ScriptContext>();
            fnCtx->Parent = ctx;

            for (size_t i = 0; i < params.size() && i < args.size(); ++i) {
                fnCtx->Variables[params[i]] = args[i];
            }
            
            auto oldCtx = m_pCurrentContext;
            m_pCurrentContext = fnCtx;
            
            ScriptValue result = 0;
            try {
                funBody->Execute(*this);
            } catch (ScriptValue& retVal) {
                result = retVal;
            }
            
            m_pCurrentContext = oldCtx;
            return result;
        }
        ctx = ctx->Parent;
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
                 for (auto& a : args) {
                     if (a.IsInt()) funcArgs.push_back(a.AsInt());
                     else if (a.IsFloat()) funcArgs.push_back(a.AsFloat());
                     else if (a.IsString()) funcArgs.push_back(a.AsString());
                     else if (a.IsObject()) funcArgs.push_back(a.AsObject());
                     else if (a.IsArray()) funcArgs.push_back((void*)a.AsArray().get());
                     else funcArgs.push_back((void*)nullptr);
                 }
                 
                 std::variant<int, float, std::string, void*> ret;
                 method->Invoke(currentObj, funcArgs, ret);
                 
                 ScriptValue retVal;
                 if (std::holds_alternative<int>(ret)) retVal.Data = std::get<int>(ret);
                 else if (std::holds_alternative<float>(ret)) retVal.Data = std::get<float>(ret);
                 else if (std::holds_alternative<std::string>(ret)) retVal.Data = std::get<std::string>(ret);
                 else if (std::holds_alternative<void*>(ret)) retVal.Data = std::get<void*>(ret);
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

void ScriptManager::AddPredicate(std::shared_ptr<ASTNode> condition, std::shared_ptr<ASTNode> body, bool loop) {
    m_vPredicates.push_back({ condition, body, false, loop, m_pCurrentContext, m_uCurrentExecutionScriptId });
}

void ScriptManager::StopAllScripts() {
    m_vPredicates.clear();
}

void ScriptManager::StopScript(size_t id) {
    for (auto it = m_vPredicates.begin(); it != m_vPredicates.end(); ) {
        if (it->ScriptId == id) {
            it = m_vPredicates.erase(it);
        } else {
            ++it;
        }
    }
}
