#pragma once

#include "../../pch.h"

namespace FD3DW {

	class ExecutionHandle {
	public:
		ExecutionHandle() = default;
		virtual ~ExecutionHandle() = default;

		void MustFullExecuteWait(bool b);
		bool IsMustFullExecuteWait();
		void Bind(ID3D12Fence* fence, UINT64 val);
		void WaitForExecute();
		void FutureWait();
		bool IsDone();
		bool IsFutureDone();

		ID3D12Fence* GetFence();
		UINT64 GetFenceValue();

	protected:
		bool DoIsFutureDone();

	protected:
		ID3D12Fence* m_pFence = nullptr;
		UINT64 m_uValue;

		std::atomic<bool> m_bMustFullExecuteWait;
		std::promise<void>        m_xBoundPromise;
		std::shared_future<void>  m_xBoundFuture = m_xBoundPromise.get_future().share();
	};


}

