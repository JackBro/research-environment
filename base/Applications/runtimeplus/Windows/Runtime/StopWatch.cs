/* Copyright (c) 2015 Jonathan Moore
///
/// This software is provided 'as-is', without any express or implied warranty. 
/// In no event will the authors be held liable for any damages arising from 
/// the use of this software.
/// 
/// Permission is granted to anyone to use this software for any purpose, 
/// including commercial applications, and to alter it and redistribute it 
/// freely, subject to the following restrictions:
///
/// 1. The origin of this software must not be misrepresented; 
/// you must not claim that you wrote the original software. 
/// If you use this software in a product, an acknowledgment in the product 
/// documentation would be appreciated but is not required.
/// 
/// 2. Altered source versions must be plainly marked as such, 
/// and must not be misrepresented as being the original software.
///
///3. This notice may not be removed or altered from any source distribution.
*/

using System;
using System.Runtime.InteropServices;

namespace System.Extensions
{
	/// <summary>
	/// 
	/// </summary>
	public class Stopwatch
	{
		[DllImport("kernel32.dll")]
		extern static short QueryPerformanceCounter(ref long x);
		[DllImport("kernel32.dll")]
		extern static short QueryPerformanceFrequency(ref long x);

		private long StartTime;
		private long StopTime;
		private long ClockFrequency;
		private long CalibrationTime;

		public Stopwatch()
		{
			StartTime       = 0;
			StopTime        = 0;
			ClockFrequency  = 0;
			CalibrationTime = 0;
			Calibrate();
		}

		public void Calibrate()
		{
			QueryPerformanceFrequency(ref ClockFrequency);
					
			for(int i=0; i<1000; i++)
			{
				Start();
				Stop();
				CalibrationTime += StopTime - StartTime;
			}

			CalibrationTime /= 1000;
		}

		public void Reset()
		{
			StartTime = 0;
			StopTime  = 0;
		}

		public void Start()
		{
			QueryPerformanceCounter(ref StartTime);
		}

		public void Stop()
		{
			QueryPerformanceCounter(ref StopTime);
		}

		public TimeSpan GetElapsedTimeSpan()
		{
			return TimeSpan.FromMilliseconds(_GetElapsedTime_ms());
		}

		public TimeSpan GetSplitTimeSpan()
		{
			return TimeSpan.FromMilliseconds(_GetSplitTime_ms());
		}

		public double GetElapsedTimeInMicroseconds()
		{
			return (((StopTime - StartTime - CalibrationTime) * 1000000.0 / ClockFrequency));
		}

		public double GetSplitTimeInMicroseconds()
		{
			long current_count = 0;
			QueryPerformanceCounter(ref current_count);
			return (((current_count - StartTime - CalibrationTime) * 1000000.0 / ClockFrequency));
		}

		private double _GetSplitTime_ms()
		{
			long current_count = 0;
			QueryPerformanceCounter(ref current_count);
			return (((current_count - StartTime - CalibrationTime) * 1000000.0 / ClockFrequency) / 1000.0);
		}

		private double _GetElapsedTime_ms()
		{
			return (((StopTime - StartTime - CalibrationTime) * 1000000.0 / ClockFrequency) / 1000.0);
		}

	}
}
