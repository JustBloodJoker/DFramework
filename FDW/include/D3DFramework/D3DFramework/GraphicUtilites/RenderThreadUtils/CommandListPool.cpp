#include "CommandListPool.h"

namespace FD3DW {
	
	CommandListPool::CommandListPool(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE type) : m_pDevice(dev), m_xType(type) {}


}
