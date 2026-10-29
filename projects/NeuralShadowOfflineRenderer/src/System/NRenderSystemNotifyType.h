#pragma once

enum class NRenderSystemNotifyType {
	Light,
	LightUpdateData,
	Shadow,
	CameraActivationDeactivation,
	CameraInfoChanged,
	UpdateTLAS,
	MeshActivationDeactivation
};
