#pragma once

#include <windows.h>
#include "concepts.h"

#ifndef D3D12BOOK_GAMETIMER
#define D3D12BOOK_GAMETIMER

template <class T> requires Floating<T>
class GameTimer
{
private:
    double mTimescale;

    double mCountPerSec;
    double mUnscaledDeltaTime;
    double mDeltaTime;

    __int64 mBaseTime;
    __int64 mPausedTime;
    __int64 mStopTime;
    __int64 mPrevTime;
    __int64 mCurrTime;
    double mScaledAccTime;

    bool mStopped;

public:
    GameTimer() : mTimescale(1.0), mCountPerSec(0L), mUnscaledDeltaTime(-1.0), mDeltaTime(-1.0),
        mBaseTime(0L), mPausedTime(0L), mStopTime(0L), mPrevTime(0L), mCurrTime(0L),
        mScaledAccTime(0.0), mStopped(false)
    {
        __int64 countPerSec;
        QueryPerformanceFrequency((LARGE_INTEGER*)&countPerSec);

        mCountPerSec = (double)countPerSec;
    }

    T UnscaledTotalTime() const
    {
        if(mStopped)
        {
            return (T)((mStopTime - mBaseTime - mPausedTime) / mCountPerSec);
        }

        return (T)((mCurrTime - mBaseTime - mPausedTime) / mCountPerSec);
    }

    T TotalTime() const
    {
        return (T)mScaledAccTime;
    }

    T UnscaledDeltaTime() const
    {
        return (T)mUnscaledDeltaTime;
    }

    T DeltaTime() const
    {
        return (T)mDeltaTime;
    }

    void SetTimescale(double timescale)
    {
        mTimescale = timescale;
    }

    double GetTimescale() const
    {
        return mTimescale;
    }

    void Reset()
    {
        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        mTimescale = 1.0;
        mBaseTime = currTime;
        mPrevTime = currTime;
        mPausedTime = 0;
        mStopTime = 0;
        mScaledAccTime = 0.0;
        mStopped = false;
    }

    void Start()
    {
        if(mStopped)
        {
            __int64 startTime;
            QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

            mPausedTime += (startTime - mStopTime);
            mPrevTime = startTime;

            mStopTime = 0;
            mStopped = false;
        }
    }

    void Stop()
    {
        if(!mStopped)
        {
            __int64 currTime;
            QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

            mStopTime = currTime;
            mStopped = true;
        }
    }

    void Tick()
    {
        if(mStopped)
        {
            mUnscaledDeltaTime = 0.0;
            mDeltaTime = 0.0;
            return;
        }

        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
        mCurrTime = currTime;

        mUnscaledDeltaTime = (mCurrTime - mPrevTime) / mCountPerSec;
        if(mUnscaledDeltaTime < 0.0)
        {
            mUnscaledDeltaTime = 0.0;
        }
        mDeltaTime = mUnscaledDeltaTime * mTimescale;

        mScaledAccTime += mDeltaTime;

        mPrevTime = mCurrTime;
    }
};

#endif