#include "ScriptNodes.h"
#include "ScriptValue.h"
#include "ScriptManager.h"

//////////////////////////////
//////////   BlockNode

ScriptValue BlockNode::Execute(ScriptManager& sm) {
    for (auto& s : Statements) s->Execute(sm);
    return 0;
}

//////////////////////////////

//////////////////////////////
////////// LiteralNode

LiteralNode::LiteralNode(ScriptValue v) : Value(v) {}
ScriptValue LiteralNode::Execute(ScriptManager&) { return Value; }

//////////////////////////////

//////////////////////////////
////////// VarNode

VarNode::VarNode(std::string n) : Name(n) {}
ScriptValue VarNode::Execute(ScriptManager& sm) { return sm.GetVariable(Name); }

//////////////////////////////


//////////////////////////////
////////// MemberAccessNode

MemberAccessNode::MemberAccessNode(std::shared_ptr<ASTNode> o, std::string m) : Object(o), Member(m) {}
ScriptValue MemberAccessNode::Execute(ScriptManager& sm) {
    auto objVal = Object->Execute(sm);
    
    if (!objVal.IsObject()) return 0;
    
    return sm.GetProperty(objVal.AsObject(), Member);
}

//////////////////////////////

//////////////////////////////
////////// ArrayLiteralNode

ScriptValue ArrayLiteralNode::Execute(ScriptManager& sm) {
    auto arr = std::make_shared<ScriptArray>();
    for (auto& e : Elements) {
        arr->Values.push_back(e ? e->Execute(sm) : ScriptValue(0));
    }
    return ScriptValue(arr);
}

//////////////////////////////

//////////////////////////////
////////// ArrayAccessNode

ArrayAccessNode::ArrayAccessNode(std::shared_ptr<ASTNode> arrayExpr, std::shared_ptr<ASTNode> indexExpr)
    : ArrayExpr(arrayExpr), IndexExpr(indexExpr) {}

ScriptValue ArrayAccessNode::Execute(ScriptManager& sm) {
    auto arrVal = ArrayExpr ? ArrayExpr->Execute(sm) : ScriptValue(0);
    auto idxVal = IndexExpr ? IndexExpr->Execute(sm) : ScriptValue(0);

    auto arr = arrVal.AsArray();
    const int idx = idxVal.AsInt();
    if (!arr || idx < 0 || idx >= (int)arr->Values.size()) return 0;

    return arr->Values[(size_t)idx];
}

//////////////////////////////


//////////////////////////////
////////// BinaryOpNode

BinaryOpNode::BinaryOpNode(std::shared_ptr<ASTNode> l, ScriptTokenType o, std::shared_ptr<ASTNode> r) : Left(l), Op(o), Right(r) {}

ScriptValue BinaryOpNode::Execute(ScriptManager& sm) {
    auto l = Left->Execute(sm);
    auto r = Right->Execute(sm);
    
    switch (Op) {
        case PLUS: return l + r;
        case MINUS: return l - r;
        case MUL: return l * r;
        case DIV: return l / r;
        case EQ: return l == r ? 1 : 0;
        case GT: return l > r ? 1 : 0;
        case LT: return l < r ? 1 : 0;
        case LE: return l < r || l == r ? 1 : 0;
        case GE: return l > r || l == r ? 1 : 0;
    }

    return 0;
}
//////////////////////////////

//////////////////////////////
////////// AssignNode

AssignNode::AssignNode(std::string name, std::shared_ptr<ASTNode> e) : VarName(name), Expr(e) {}
AssignNode::AssignNode(std::shared_ptr<MemberAccessNode> acc, std::shared_ptr<ASTNode> e) : MemberAccess(acc), Expr(e) {}
AssignNode::AssignNode(std::shared_ptr<ArrayAccessNode> acc, std::shared_ptr<ASTNode> e) : ArrayAccess(acc), Expr(e) {}

ScriptValue AssignNode::Execute(ScriptManager& sm) {
    auto val = Expr->Execute(sm);
    if (MemberAccess) {
        auto objVal = MemberAccess->Object->Execute(sm);
        if (objVal.IsObject()) sm.SetProperty(objVal.AsObject(), MemberAccess->Member, val);
    }
    else if (ArrayAccess) {
        auto arrVal = ArrayAccess->ArrayExpr ? ArrayAccess->ArrayExpr->Execute(sm) : ScriptValue(0);
        auto idxVal = ArrayAccess->IndexExpr ? ArrayAccess->IndexExpr->Execute(sm) : ScriptValue(0);

        auto arr = arrVal.AsArray();
        int idx = idxVal.AsInt();
        if (arr && idx >= 0) {
            if ((size_t)idx >= arr->Values.size()) {
                arr->Values.resize((size_t)idx + 1);
            }
            arr->Values[(size_t)idx] = val;
        }
    }
    else {
        sm.SetVariable(VarName, val);
    }
    return val;
}

//////////////////////////////

//////////////////////////////
////////// IfNode

IfNode::IfNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> t, std::shared_ptr<ASTNode> e) : Cond(c), ThenBlock(t), ElseBlock(e) {}

ScriptValue IfNode::Execute(ScriptManager& sm) {
    if (Cond->Execute(sm).AsBool()) {
        ThenBlock->Execute(sm);
    }
    else if (ElseBlock) {
        ElseBlock->Execute(sm);
    }
    return 0;
}

//////////////////////////////

//////////////////////////////
////////// WhileNode

WhileNode::WhileNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> b) : Cond(c), Body(b) {}

ScriptValue WhileNode::Execute(ScriptManager& sm) {
    while (Cond->Execute(sm).AsBool()) Body->Execute(sm);
    return 0;
}

//////////////////////////////

//////////////////////////////
////////// CallNode

CallNode::CallNode(std::string name) : FuncName(name) {}

ScriptValue CallNode::Execute(ScriptManager& sm) {
    std::vector<ScriptValue> evalArgs;
    for (auto& a : Args) evalArgs.push_back( a->Execute(sm) );

	//TODO remove hardcoded functions and use-a reflection or something for extensibility.
    if (FuncName == "GetDeltaTime") {
        return sm.GetDeltaTime();
    }

    if (FuncName == "CreateObject") {
        if (!evalArgs.empty() && evalArgs[0].IsString()) {
            return sm.CreateObject(evalArgs[0].AsString());
        }
    }

    return sm.CallFunction(FuncName, evalArgs);
}

//////////////////////////////

//////////////////////////////
////////// MethodCallNode

MethodCallNode::MethodCallNode(std::shared_ptr<ASTNode> o, std::string m) : Object(o), MethodName(m) {}

ScriptValue MethodCallNode::Execute(ScriptManager& sm) {
    auto objVal = Object->Execute(sm);
    if (!objVal.IsObject()) return 0;

    std::vector<ScriptValue> evalArgs;
    for (auto& a : Args) evalArgs.push_back(a->Execute(sm));

    return sm.CallMethod(objVal.AsObject(), MethodName, evalArgs);
}

//////////////////////////////

//////////////////////////////
////////// PredicateRegisterNode

PredicateRegisterNode::PredicateRegisterNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> b, bool loop) : Cond(c), Body(b), Loop(loop) {}

ScriptValue PredicateRegisterNode::Execute(ScriptManager& sm) {
    sm.AddPredicate(Cond, Body, Loop);
    return 0;
}

//////////////////////////////

//////////////////////////////
////////// FunctionDefNode

FunctionDefNode::FunctionDefNode(std::string name, std::vector<std::string> params, std::shared_ptr<ASTNode> body) 
    : FuncName(name), Params(params), Body(body) {}

ScriptValue FunctionDefNode::Execute(ScriptManager& sm) {
    sm.RegisterFunction(FuncName, Params, Body);
    return 0;
}

//////////////////////////////

//////////////////////////////
////////// ReturnNode

ReturnNode::ReturnNode(std::shared_ptr<ASTNode> e) : Expr(e) {}

ScriptValue ReturnNode::Execute(ScriptManager& sm) {
    auto val = Expr ? Expr->Execute(sm) : ScriptValue(0);
    throw val;
}

//////////////////////////////
