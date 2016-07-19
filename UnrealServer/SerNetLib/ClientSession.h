#pragma once

#include "Define.h"

namespace NServerNetLib
{
	struct ClientSession
	{
		bool IsConnected()
		{
			return SocketFD > 0 ? true : false;
		}

		void Clear()
		{
			Seq = 0;
			SocketFD = 0;
			IP[0] = '\0';
			RemainingDataSize = 0;
			SendSize = 0;
		}

		int Index		= 0;
		long long Seq	= 0;
		int SocketFD	= 0;
		char IP[MAX_IP_LEN] = { 0, };

		char*	p_RecvBuffer = nullptr;
		int		RemainingDataSize = 0;

		char*	p_SendBuffer = nullptr;
		int		SendSize = 0;
	};
	
}