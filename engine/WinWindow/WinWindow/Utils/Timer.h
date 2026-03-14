#pragma once
#include "../pch.h"




namespace FDWWIN
{


	class Timer
	{

	public:

		Timer();
		virtual ~Timer() = default;;

		float GetTime() const; 
		float GetDeltaTime() const;
		void Reset(); 
		void Start(); 
		void Stop(); 
		void Tick();

	private:

		double m_dSecondPerTick;
		double m_dDeltaTime;
		
		__int64 m_iBaseTime;
		__int64 m_iPauseTime;
		__int64 m_iStopTime;
		__int64 m_iPrevTime;
		__int64 m_iCurrTime;

		bool m_bIsStopped;

	};


}