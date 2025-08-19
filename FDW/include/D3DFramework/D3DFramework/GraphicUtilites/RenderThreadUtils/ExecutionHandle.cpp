#include "ExecutionHandle.h"

namespace FD3DW {

	void ExecutionHandle::Bind(ID3D12Fence* fence, UINT64 val) {
		m_pFence = fence;
		m_uValue = val;
	}

	void ExecutionHandle::WaitForExecute()
	{
		if (!m_pFence) return;

		if (m_pFence->GetCompletedValue() < m_uValue) {
			HANDLE h = CreateEvent(nullptr, FALSE, FALSE, L"Execution handle sync");
			HRESULT_ASSERT(m_pFence->SetEventOnCompletion(m_uValue, h), "Fence event error");
			WaitForSingleObject(h, INFINITE);
			CloseHandle(h);
		}
	}

	bool ExecutionHandle::IsDone()
	{
		return m_pFence ? m_pFence->GetCompletedValue()>=m_uValue : true;
	}


}
