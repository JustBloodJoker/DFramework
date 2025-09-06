#include <System/SkyboxRenderSystem.h>
#include <World/World.h>
#include <MainRenderer/MainRenderer.h>
#include <MainRenderer/PSOManager.h>

void SkyboxRenderSystem::AfterConstruction() {
	m_pMatricesBuffer = FD3DW::UploadBuffer<FD3DW::MatricesConstantBufferStructureFrameWork>::CreateConstantBuffer(m_pOwner->GetDevice(), 1);
}

std::shared_ptr<FD3DW::ExecutionHandle> SkyboxRenderSystem::OnStartRenderTick(std::shared_ptr<FD3DW::ExecutionHandle> sync) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this](ID3D12GraphicsCommandList* list) {
		if (m_bIsNeedUpdateActiveSkybox.exchange(false, std::memory_order_acq_rel)) {
			m_pActiveComponent = nullptr;

			auto components = GetWorld()->GetAllComponentsOfType<SkyboxComponent>();
			for (const auto& component : components) {
				if (component->IsActive()) {
					m_pActiveComponent = component;
					break;
				}
			}
		}

		if (!m_pActiveComponent) return;

		RenderComponentBeforeRenderInputData inData;
		inData.CameraPosition = m_pOwner->GetCurrentCameraPosition();
		inData.CommandList = list;
		inData.Device = m_pOwner->GetDevice(); 
		auto timer = m_pOwner->GetTimer();
		inData.DT = timer->GetDeltaTime();
		inData.Projection = m_pOwner->GetCurrentProjectionMatrix();
		inData.Time = timer->GetTime();
		inData.View = m_pOwner->GetCurrentViewMatrix();

		m_pActiveComponent->OnStartRenderTick(inData);

		m_pMatricesBuffer->CpyData(0, m_pActiveComponent->MatricesData());
	});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { sync });
}

std::shared_ptr<FD3DW::ExecutionHandle> SkyboxRenderSystem::RenderSkyboxPass(std::shared_ptr<FD3DW::ExecutionHandle> sync, SkyboxRenderPassInput input) {
	auto recipe = std::make_shared<FD3DW::CommandRecipe<ID3D12GraphicsCommandList>>(D3D12_COMMAND_LIST_TYPE_DIRECT, [this, input](ID3D12GraphicsCommandList* list) {
		if (!m_pActiveComponent) return;

		input.RTV->StartDraw(list);
		list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		list->RSSetScissorRects(1, &input.Rect);
		list->RSSetViewports(1, &input.Viewport);
		list->OMSetRenderTargets(1, &FD3DW::keep(input.RTV_CPU), true, &FD3DW::keep(input.DSV_CPU));

		PSOManager::GetInstance()->GetPSOObject(PSOType::SimpleSkyboxDefaultConfig)->Bind(list);

		auto cube = m_pActiveComponent->Cube();
		auto srvPack = m_pActiveComponent->SRVPack();

		list->IASetVertexBuffers(0, 1, cube->GetVertexBufferView());
		list->IASetIndexBuffer(cube->GetIndexBufferView());

		list->SetGraphicsRootConstantBufferView(0, m_pMatricesBuffer->GetGPULocation(0));

		ID3D12DescriptorHeap* heaps[] = { srvPack->GetResult()->GetDescriptorPtr() };
		list->SetDescriptorHeaps(_countof(heaps), heaps);
		list->SetGraphicsRootDescriptorTable(1, srvPack->GetResult()->GetGPUDescriptorHandle(0));

		auto objectInfo = cube->GetObjectParameters(0);
		list->DrawIndexedInstanced(objectInfo.IndicesCount, 1, objectInfo.IndicesOffset, objectInfo.VerticesOffset, 0);

		input.RTV->EndDraw(list);
	});
	return GlobalRenderThreadManager::GetInstance()->Submit(recipe, { sync });
}

void SkyboxRenderSystem::ProcessNotify(NRenderSystemNotifyType type) {
	if (type == NRenderSystemNotifyType::SkyboxActivationDeactivation) {
		m_bIsNeedUpdateActiveSkybox.store(true, std::memory_order_relaxed);
	}
}
