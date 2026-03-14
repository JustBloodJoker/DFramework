#pragma once
#include "../pch.h"
#include "CommandListTemplate.h"

namespace FD3DW
{

	using CommandList = CommandListTemplate<ID3D12GraphicsCommandList>;
	using DXRCommandList = CommandListTemplate<ID3D12GraphicsCommandList4>;
}