#pragma once

#include <pch.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <D3DFramework/Utilites/Audio.h>

class RenderableAudioObject : public BaseRenderableObject {
public:
	RenderableAudioObject(std::unique_ptr<FD3DW::Audio> audio, const std::string& path);
	virtual ~RenderableAudioObject() = default;

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;

	void Play();
	void Stop();
	void Restart();
	
	void SetVolume(float volume);
	float GetVolume();
	
	bool IsLoop();
	void Loop(bool loop);

protected:
	void TickAudioUpdate();

protected:
	std::unique_ptr<FD3DW::Audio> m_pAudio;
	bool m_bIsLoop = false;

};