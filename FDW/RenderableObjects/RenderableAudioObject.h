#pragma once

#include <pch.h>
#include <RenderableObjects/BaseRenderableObject.h>
#include <D3DFramework/Utilites/Audio.h>
#include <D3DFramework/Utilites/AudioManager.h>


class RenderableAudioObject : public BaseRenderableObject {
public:
	RenderableAudioObject()=default;
	RenderableAudioObject(const std::string& path);
	virtual ~RenderableAudioObject() = default;

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
	virtual void BeforeRender(const BeforeRenderInputData& data) override;
	virtual void DeferredRender(ID3D12GraphicsCommandList* list) override;
	virtual void ForwardRender(ID3D12GraphicsCommandList* list) override;
	virtual RenderPass GetRenderPass() const override;

	void CreateAfterLoadAudio(FD3DW::AudioManager* manager);

	void Play();
	void Stop();
	void Restart();
	
	void SetVolume(float volume);
	float GetVolume();
	
	bool IsLoop();
	void Loop(bool loop);

	virtual void InitBLASBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* list) override;

	BEGIN_FIELD_REGISTRATION(RenderableAudioObject, BaseRenderableObject)
		REGISTER_FIELD(m_sPath)
	END_FIELD_REGISTRATION()


protected:
	void TickAudioUpdate();

protected:
	std::string m_sPath;
	std::unique_ptr<FD3DW::Audio> m_pAudio;
	bool m_bIsLoop = false;

};