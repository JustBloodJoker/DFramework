#include "../pch.h"




namespace FDW
{


	class Timer
	{

	public:

		Timer();
		~Timer() = default;;

		float GetTime() const; 
		float GetDeltaTime() const;
		void Reset(); 
		void Start(); 
		void Stop(); 
		void Tick();

	private:

		double secondPerTick;
		double deltaTime;

		__int64 baseTime;
		__int64 pauseTime;
		__int64 stopTime;
		__int64 prevTime;
		__int64 currTime;

		bool isStopped;

	};


}