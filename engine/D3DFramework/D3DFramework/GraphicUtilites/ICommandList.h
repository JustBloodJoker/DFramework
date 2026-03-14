#pragma once
#include "../pch.h"

namespace FD3DW
{

    class ICommandList
    {
    public:
        ICommandList() = default;
        virtual ~ICommandList() = default;

        virtual bool TryCloseList() = 0;
        virtual void CloseList() = 0;
        virtual ID3D12CommandList* GetPtrDefaultCommandList() const = 0;
        virtual D3D12_COMMAND_LIST_TYPE GetType() const = 0;
    };

}