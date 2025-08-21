#include "ExecutionHandle.h"

namespace FD3DW {

	void ExecutionHandle::Bind(ID3D12Fence* fence, UINT64 val) {
		m_pFence = fence;
		m_uValue = val;

		try { m_xBoundPromise.set_value(); }
		catch (...) {}
	}

	void ExecutionHandle::WaitForExecute()
	{
		FutureWait();

		if (!m_pFence) return;

		if (m_pFence->GetCompletedValue() < m_uValue) {
			HANDLE h = CreateEvent(nullptr, FALSE, FALSE, L"Execution handle sync");
			HRESULT_ASSERT(m_pFence->SetEventOnCompletion(m_uValue, h), "Fence event error");
			WaitForSingleObject(h, INFINITE);
			CloseHandle(h);
		}
	}

	void ExecutionHandle::FutureWait()
	{
		m_xBoundFuture.wait();
	}

	bool ExecutionHandle::IsDone()
	{
		if (m_xBoundFuture.wait_for(std::chrono::seconds(0))!=std::future_status::ready)
			return false;

		if (!m_pFence) return true;
		return m_pFence->GetCompletedValue() >= m_uValue;
	}

	ID3D12Fence* ExecutionHandle::GetFence()
	{
		return m_pFence;
	}

	UINT64 ExecutionHandle::GetFenceValue()
	{
		return m_uValue;
	}


}
