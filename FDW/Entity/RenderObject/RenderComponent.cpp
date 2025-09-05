#include <Entity/RenderObject/RenderComponent.h>
#include <World/World.h>


dx::XMMATRIX RenderComponent::GetWorldMatrix() const {
	return m_xWorldMatrix;
}