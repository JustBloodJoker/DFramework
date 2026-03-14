#pragma once

#include <Utilities/HelperFunctions.h>

namespace FDWUtils {

	std::string NormalizePath(const std::string& path) {
		if (path.empty()) return path;
		std::error_code ec;
		auto normalized = std::filesystem::weakly_canonical(std::filesystem::path(path), ec);
		if (ec) {
			normalized = std::filesystem::path(path).lexically_normal();
		}
		return normalized.generic_string();
	}

}