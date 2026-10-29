#include "pch.h"

#include <MainRenderer/MainRenderer.h>

int main() {
    auto renderer = std::make_unique<MainRenderer>();
    renderer->__START();
	renderer.reset();
    return 0;
}
