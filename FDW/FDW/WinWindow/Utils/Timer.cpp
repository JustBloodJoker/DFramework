#include "../pch.h"
#include "Timer.h"


namespace FDWWIN
{


	Timer::Timer()
		: m_dSecondPerTick(0.0), m_dDeltaTime(-1.0), m_iBaseTime(0), m_iPauseTime(0), m_iPrevTime(0), m_iCurrTime(0), m_iStopTime(0), m_bIsStopped(false)
	{
		__int64 countsPerSec;
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

		m_dSecondPerTick = 1.0 / static_cast<double>(countsPerSec);
	}

	float Timer::GetTime() const
	{
		return m_bIsStopped ? static_cast<float>((m_iStopTime - m_iPauseTime - m_iBaseTime) * m_dSecondPerTick) : static_cast<float>((m_iCurrTime - m_iPauseTime - m_iBaseTime) * m_dSecondPerTick);
	}

	float Timer::GetDeltaTime() const
	{
		return static_cast<float>(m_dDeltaTime);
	}

	void Timer::Reset()
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&m_iPrevTime);

		m_iBaseTime = m_iPrevTime;
		m_iStopTime = 0;
		m_bIsStopped = false;
	}

	void Timer::Start()
	{
		__int64 startTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

		if (m_bIsStopped)
		{
			m_iPauseTime += (startTime - m_iStopTime);
			m_iPrevTime = startTime;
			m_iStopTime = 0;
			m_bIsStopped = false;
		}

		m_bIsStopped = false;
	}

	void Timer::Stop()
	{
		if (!m_bIsStopped)
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&m_iStopTime);
			m_bIsStopped = true;
		}
	}

	void Timer::Tick()
	{
		if (m_bIsStopped)
		{
			m_dDeltaTime = 0.0;
			return;
		}

		QueryPerformanceCounter((LARGE_INTEGER*)&m_iCurrTime);

		m_dDeltaTime = (m_iCurrTime - m_iPrevTime) * m_dSecondPerTick;

		m_iPrevTime = m_iCurrTime;

		if (m_dDeltaTime < 0.0)
		{
			m_dDeltaTime = 0.0;
		}
	}

}
