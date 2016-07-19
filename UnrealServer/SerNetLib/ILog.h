#pragma once

#include <stdio.h>

//가변인자를 위한 헤더파일
#include <stdarg.h>

namespace NServerNetLib
{
	const int MAX_LOG_STRING_LENGTH = 256;

	enum class LOG_TYPE : short
	{
		L_TRACE = 1,
		L_DEBUG = 2,
		L_WARN = 3,
		L_ERROR = 4,
		L_INFO = 5,
	};

	class ILog
	{
	public:
		ILog() {}
		virtual ~ILog() {}

		virtual void Write(const LOG_TYPE LogType, const char* p_Format, ...)
		{
			char szText[MAX_LOG_STRING_LENGTH];

			va_list args;
			va_start(args, p_Format);
			//vsprintf_s자체가 가변인자를 받음.
			vsprintf_s(szText, MAX_LOG_STRING_LENGTH, p_Format, args);
			va_end(args);

			//Abstrac Class이기에 인스턴스화 불가, 반드시 override 되어있을 것이므로 호출 가능.
			switch (LogType)
			{
			case LOG_TYPE::L_INFO:
				Info(szText);
				break;
			case LOG_TYPE::L_ERROR:
				Error(szText);
				break;
			case LOG_TYPE::L_WARN:
				Warn(szText);
				break;
			case LOG_TYPE::L_DEBUG:
				Debug(szText);
				break;
			case LOG_TYPE::L_TRACE:
				Info(szText);
				break;
			default:
				break;
			}
		}

	protected:
		virtual void Error(const char* pText) = 0;
		virtual void Warn(const char* pText) = 0;
		virtual void Debug(const char* pText) = 0;
		virtual void Trace(const char* pText) = 0;
		virtual void Info(const char* pText) = 0;
	};
}