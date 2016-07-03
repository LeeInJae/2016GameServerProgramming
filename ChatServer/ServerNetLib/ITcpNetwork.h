#pragma once

#include "Define.h"
#include "ServernetErrorCode.h"
#include "ILog.h"

namespace NServerNetLib
{
	class ITcpNetwork
	{
	public:
		ITcpNetwork() {}
		virtual ~ITcpNetwork() {}

		virtual NET_ERROR_CODE Init(const ServerConfig* pConfig, ILog* pLogger)
		{
			return NET_ERROR_CODE::NONE;
		}

		virtual NET_ERROR_CODE SendData(const int SessionIndex, const short packetID, const short Size, const char* p_Msg)
		{
			return NET_ERROR_CODE::NONE;
		}

		virtual void Run() 
		{
		}

		virtual RecvPacketInfo GetPacketInfo()
		{
			return RecvPacketInfo();
		}
	};
}