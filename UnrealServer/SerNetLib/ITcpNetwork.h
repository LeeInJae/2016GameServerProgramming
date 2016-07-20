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

		virtual NET_ERROR_CODE SendData(const int SessionIndex, const short PacketID, const short Size, const char* p_Msg)
		{
			return NET_ERROR_CODE::NONE;
		}

		virtual void Run()
		{
		}

		virtual RecvpacketInfo GetPacketInfo()
		{
			return RecvpacketInfo();
		}

		virtual void Release() {}

		virtual int ClientSessionPoolSize() { return 0; }

		virtual void ForcingClose(const int SessionIndex) {}
	};
}