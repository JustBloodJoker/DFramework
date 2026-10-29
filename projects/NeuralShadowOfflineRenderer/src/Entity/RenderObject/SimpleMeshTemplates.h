#pragma once

#include <Entity/RenderObject/TSimpleMesh.h>

class TSimplePlane : public TSimpleMesh {
public:
	TSimplePlane() = default;
	virtual ~TSimplePlane() = default;

	REFLECT_BODY(TSimplePlane)
	BEGIN_REFLECT(TSimplePlane, TSimpleMesh)
	END_REFLECT(TSimplePlane)

public:
	void AfterCreation() override;

protected:
	std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> CreateSimpleObject(
		ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
};

class TSimpleCube : public TSimpleMesh {
public:
	TSimpleCube() = default;
	virtual ~TSimpleCube() = default;

	REFLECT_BODY(TSimpleCube)
	BEGIN_REFLECT(TSimpleCube, TSimpleMesh)
	END_REFLECT(TSimpleCube)

public:
	void AfterCreation() override;

protected:
	std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> CreateSimpleObject(
		ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
};

class TSimpleCone : public TSimpleMesh {
public:
	TSimpleCone() = default;
	virtual ~TSimpleCone() = default;

	REFLECT_BODY(TSimpleCone)
	BEGIN_REFLECT(TSimpleCone, TSimpleMesh)
	END_REFLECT(TSimpleCone)

public:
	void AfterCreation() override;

protected:
	std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> CreateSimpleObject(
		ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
};

class TSimpleSphere : public TSimpleMesh {
public:
	TSimpleSphere() = default;
	virtual ~TSimpleSphere() = default;

	REFLECT_BODY(TSimpleSphere)
	BEGIN_REFLECT(TSimpleSphere, TSimpleMesh)
	END_REFLECT(TSimpleSphere)

public:
	void AfterCreation() override;

protected:
	std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> CreateSimpleObject(
		ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
};

class TSimpleCylinder : public TSimpleMesh {
public:
	TSimpleCylinder() = default;
	virtual ~TSimpleCylinder() = default;

	REFLECT_BODY(TSimpleCylinder)
	BEGIN_REFLECT(TSimpleCylinder, TSimpleMesh)
	END_REFLECT(TSimpleCylinder)

public:
	void AfterCreation() override;

protected:
	std::unique_ptr<FD3DW::SimpleObject<FD3DW::VertexFrameWork>> CreateSimpleObject(
		ID3D12Device* device, ID3D12GraphicsCommandList* list) override;
};
