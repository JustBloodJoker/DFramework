#include <Entity/RenderObject/TRender.h>
#include <Component/RenderObject/RenderComponent.h>
#include <World/World.h>


std::shared_ptr<FD3DW::ExecutionHandle> TRender::RenderInit(ID3D12Device* device, std::shared_ptr<FD3DW::ExecutionHandle> sync) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, device](ID3D12GraphicsCommandList* list) {
		DoRenderInit(device, list);
	});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { sync });
}


std::shared_ptr<FD3DW::ExecutionHandle> TRender::RenderInitDXR(ID3D12Device5* device, std::shared_ptr<FD3DW::ExecutionHandle> sync) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList4>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, device](ID3D12GraphicsCommandList4* list) {
		DoRenderInitDXR(device, list);
	});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { sync });
}

void TRender::DoRenderInit(ID3D12Device* device, ID3D12GraphicsCommandList* list) {
	auto cmps = GetComponents<RenderComponent>();
	for (auto& cmp : cmps) {
		cmp->RenderInit(device, list);
	}
}

void TRender::DoRenderInitDXR(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) {
	auto cmps = GetComponents<RenderComponent>();
	for (auto& cmp : cmps) {
		cmp->RenderInitDXR(device, list);
	}
}


