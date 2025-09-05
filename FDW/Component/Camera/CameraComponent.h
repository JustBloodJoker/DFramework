#pragma once

#include <pch.h>
#include <Component/Core/IComponent.h>

class CameraComponent : public IComponent {
public:
	CameraComponent();
	virtual ~CameraComponent() = default;


public:
	BEGIN_FIELD_REGISTRATION(CameraComponent, IComponent)
		REGISTER_FIELD(m_xView);
		REGISTER_FIELD(m_xEye);
		REGISTER_FIELD(m_xUp);
		REGISTER_FIELD(m_xAt);
	END_FIELD_REGISTRATION();

public:

	virtual void Init() override;
	virtual void Destroy() override;
	virtual void Activate(bool a) override;

	void SetAllVectors(dx::XMVECTOR newEye, dx::XMVECTOR newUp, dx::XMVECTOR newAt);

	void SetEye(dx::XMVECTOR newEye);
	dx::XMVECTOR GetEye() const;
	void SetUp(dx::XMVECTOR newUp);
	dx::XMVECTOR GetUp() const;
	void SetAt(dx::XMVECTOR newAt);
	dx::XMVECTOR GetAt() const;

public:
	void InitDefault();

	dx::XMMATRIX GetViewMatrix() const;
	dx::XMFLOAT3 GetCameraPosition() const;
	void UpdateViewMatrix();

protected:

	dx::XMMATRIX m_xView;

	dx::XMVECTOR m_xEye;
	dx::XMVECTOR m_xUp;
	dx::XMVECTOR m_xAt;
};
