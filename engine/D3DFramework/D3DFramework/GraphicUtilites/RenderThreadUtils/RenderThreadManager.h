#pragma once

#include "../../pch.h"
#include "AsyncCommandQueue.h"
#include "CommandListPool.h"
#include "CommandRecipe.h"
#include "RenderThread.h"
#include "WinWindow/Utils/WorkerPool.h"

namespace FD3DW {

class RenderThreadManager {
  public:
    RenderThreadManager() = default;
    virtual ~RenderThreadManager() = default;

    std::shared_ptr<ExecutionHandle> CreateWaitHandle(D3D12_COMMAND_LIST_TYPE type);

    void WaitForLastCommandGPU();
    void WaitForLastCommandGPU(D3D12_COMMAND_LIST_TYPE type);

    void WaitIdle();

    void Init(ID3D12Device *device);
    void Shutdown();

    AsyncCommandQueue *GetQueue(D3D12_COMMAND_LIST_TYPE type);

    std::shared_ptr<ExecutionHandle> Submit(const std::shared_ptr<ICommandRecipe> &recipe,
                                            std::vector<std::shared_ptr<ExecutionHandle>> dependencies = {},
                                            bool IsNeedFullExecute = false);
    std::shared_ptr<ExecutionHandle> SubmitLambda(std::function<void()> func,
                                                  std::vector<std::shared_ptr<ExecutionHandle>> dependencies = {},
                                                  bool IsNeedFullExecute = false, bool ExecuteOnBuilderThread = false);
    std::shared_ptr<ExecutionHandle> SubmitGpuSynchronizedLambda(std::function<void()> func,
                                                                 std::vector<std::shared_ptr<ExecutionHandle>> dependencies = {},
                                                                 bool IsNeedFullExecute = false);

    void GarbageCollectAll();

  protected:
    int m_iMaxThreadsCount = 1;

  private:
    AsyncCommandQueue *PickCommandQueue(D3D12_COMMAND_LIST_TYPE type);
    std::shared_ptr<ExecutionHandle> SubmitManagedLambda(std::function<void()> func,
                                                         std::vector<std::shared_ptr<ExecutionHandle>> dependencies, bool IsNeedFullExecute,
                                                         bool ExecuteOnBuilderThread, bool WaitForGpuDependencies);

  private:
    ID3D12Device *m_pDevice = nullptr;
    FD3DW::RenderThread m_xRenderThread;
    FDWWIN::WorkerPool m_xCommandsBuilder;

    std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<AsyncCommandQueue>> m_mQueues;
    std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandListPool>> m_mPools;
};

} // namespace FD3DW
