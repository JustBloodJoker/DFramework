#include "ExecutionHandle.h"

namespace FD3DW {
	void ExecutionHandle::MustFullExecuteWait(bool b)
	{
		m_bMustFullExecuteWait = b;
	}

	bool ExecutionHandle::IsMustFullExecuteWait()
	{
		return m_bMustFullExecuteWait;
	}

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
		if (!DoIsFutureDone()) return false;

		if (!m_pFence) return true;
		return m_pFence->GetCompletedValue() >= m_uValue;
	}

	bool ExecutionHandle::IsFutureDone()
	{
		if (m_bMustFullExecuteWait) {
			return IsDone();
		}
		return DoIsFutureDone();
	}

	ID3D12Fence* ExecutionHandle::GetFence()
	{
		return m_pFence;
	}

	UINT64 ExecutionHandle::GetFenceValue()
	{
		return m_uValue;
	}

	bool ExecutionHandle::DoIsFutureDone()
	{
		return m_xBoundFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}


}
