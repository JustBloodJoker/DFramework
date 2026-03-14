#pragma once

#include "..\..\pch.h"

namespace FD3DW {

    struct ICommandRecipe {
        ICommandRecipe() = default;
        virtual ~ICommandRecipe() = default;
        virtual D3D12_COMMAND_LIST_TYPE GetType() const = 0;
        virtual void Record(ID3D12CommandList* list) const = 0;
    };


    template<typename ListType>
    struct CommandRecipe : public ICommandRecipe {
        using CommandRecorder = std::function<void(ListType*)>;

        D3D12_COMMAND_LIST_TYPE type;
        CommandRecorder record;

        CommandRecipe(D3D12_COMMAND_LIST_TYPE t, CommandRecorder r)
            : type(t), record(std::move(r)) {
        }

        D3D12_COMMAND_LIST_TYPE GetType() const override { return type; }

        void Record(ID3D12CommandList* list) const override {
            wrl::ComPtr<ListType> typed;
            HRESULT_ASSERT( list->QueryInterface(IID_PPV_ARGS(&typed)), "This device does not support requested CommandList interface");
            record(typed.Get());
        }
    };

}