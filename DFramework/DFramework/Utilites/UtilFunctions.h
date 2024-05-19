#pragma once
#include "../pch.h"



namespace FDW
{



	template <class T>
	constexpr auto& keep(T&& x) noexcept {
		return x;
	}

}


