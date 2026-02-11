#pragma once

#include "../../pch.h"
#include "ScriptTokenType.h"
#include "ScriptValue.h"

class ScriptManager;

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual ScriptValue Execute(ScriptManager& context) = 0;
};

struct BlockNode : public ASTNode {
    std::vector<std::shared_ptr<ASTNode>> Statements;

    ScriptValue Execute(ScriptManager& sm) override;
};

struct LiteralNode : public ASTNode {
    ScriptValue Value;

    LiteralNode(ScriptValue v);
    ScriptValue Execute(ScriptManager&) override;
};

struct VarNode : public ASTNode {
    std::string Name;

    VarNode(std::string n);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct MemberAccessNode : public ASTNode {
    std::shared_ptr<ASTNode> Object;
    std::string Member;

    MemberAccessNode(std::shared_ptr<ASTNode> o, std::string m);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct BinaryOpNode : public ASTNode {
    std::shared_ptr<ASTNode> Left;
    std::shared_ptr<ASTNode> Right;
    ScriptTokenType Op;

    BinaryOpNode(std::shared_ptr<ASTNode> l, ScriptTokenType o, std::shared_ptr<ASTNode> r);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct AssignNode : public ASTNode {
    std::string VarName;
    std::shared_ptr<ASTNode> Expr;
    std::shared_ptr<MemberAccessNode> MemberAccess;

    AssignNode(std::string name, std::shared_ptr<ASTNode> e);
    AssignNode(std::shared_ptr<MemberAccessNode> acc, std::shared_ptr<ASTNode> e);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct IfNode : public ASTNode {
    std::shared_ptr<ASTNode> Cond;
    std::shared_ptr<ASTNode> ThenBlock;
    std::shared_ptr<ASTNode> ElseBlock;

    IfNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> t, std::shared_ptr<ASTNode> e);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct WhileNode : public ASTNode {
    std::shared_ptr<ASTNode> Cond;
    std::shared_ptr<ASTNode> Body;

    WhileNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> b);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct CallNode : public ASTNode {
    std::string FuncName;
    std::vector<std::shared_ptr<ASTNode>> Args;

    CallNode(std::string name);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct MethodCallNode : public ASTNode {
    std::shared_ptr<ASTNode> Object;
    std::string MethodName;
    std::vector<std::shared_ptr<ASTNode>> Args;

    MethodCallNode(std::shared_ptr<ASTNode> o, std::string m);
    ScriptValue Execute(ScriptManager& sm) override;
};

struct PredicateRegisterNode : public ASTNode {
    std::shared_ptr<ASTNode> Cond;
    std::shared_ptr<ASTNode> Body;
    bool Loop = false;
    
    PredicateRegisterNode(std::shared_ptr<ASTNode> c, std::shared_ptr<ASTNode> b, bool loop = false);
    ScriptValue Execute(ScriptManager& sm) override;
};
