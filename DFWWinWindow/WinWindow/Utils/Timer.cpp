#include "../pch.h"
#include "Timer.h"


namespace FDWWIN
{


	Timer::Timer()
		: secondPerTick(0.0), deltaTime(-1.0), baseTime(0), pauseTime(0), prevTime(0), currTime(0), stopTime(0), isStopped(false)
	{
		__int64 countsPerSec;
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

		secondPerTick = 1.0 / static_cast<double>(countsPerSec);
	}

	float Timer::GetTime() const
	{
		return isStopped ? static_cast<float>((stopTime - pauseTime - baseTime) * secondPerTick) : static_cast<float>((currTime - pauseTime - baseTime) * secondPerTick);
	}

	float Timer::GetDeltaTime() const
	{
		return static_cast<float>(deltaTime);
	}

	void Timer::Reset()
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&prevTime);

		baseTime = prevTime;
		stopTime = 0;
		isStopped = false;
	}

	void Timer::Start()
	{
		__int64 startTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

		if (isStopped)
		{
			pauseTime += (startTime - stopTime);
			prevTime = startTime;
			stopTime = 0;
			isStopped = false;
		}

		isStopped = false;
	}

	void Timer::Stop()
	{
		if (!isStopped)
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&stopTime);
			isStopped = true;
		}
	}

	void Timer::Tick()
	{
		if (isStopped)
		{
			deltaTime = 0.0;
			return;
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		deltaTime = (currTime - prevTime) * secondPerTick;

		prevTime = currTime;

		if (deltaTime < 0.0)
		{
			deltaTime = 0.0;
		}
	}

}
