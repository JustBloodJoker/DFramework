#pragma once

#include "../../pch.h"
#include "ScriptTokenType.h"

struct ScriptToken {
    ScriptTokenType Type;
    std::string Text;
    int Line;
};
