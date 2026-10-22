#include "GameTimer.h"


GameTimer::GameTimer()
{

}

float GameTimer::TotalTime() const
{
	if (m_stopped)
	{
		return (float)((m_stopTime - m_pausedTime - m_baseTime) * m_secsPerCount);
	}
	else
	{
		return (float)((m_currTime - m_pausedTime - m_baseTime) * m_secsPerCount);
	}
}

float GameTimer::DeltaTime() const
{
	return float(m_dTime);
}


void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_baseTime = currTime;
	m_prevTime = currTime;
	m_stopTime = 0;
	m_stopped = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (m_stopped)
	{
		m_pausedTime += (startTime - m_stopTime);

		m_prevTime = startTime;

		m_stopTime = 0;
		m_stopped = false;
	}
}

void GameTimer::Stop()
{
	if (!m_stopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		m_stopTime = currTime;
		m_stopped = true;
	}
}

void GameTimer::Tick()
{
	if (m_stopped)
	{
		m_dTime = 0;
		return;
	}

	// Get current time
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_currTime = currTime;

	// Time difference t_i - t_{i-1}
	m_dTime = (m_currTime - m_prevTime) * m_secsPerCount;

	// For next frame
	m_prevTime = m_currTime;

	if (m_dTime < 0) { m_dTime = 0; }
}

