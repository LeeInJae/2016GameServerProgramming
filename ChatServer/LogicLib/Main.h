#pragma once
#include <memory>

#include "../../Common/Packet.h"
#include "../../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NServerNetLib
{
	struct ServerConfig;
	class ILog;
	class ITcpNetwork;
}

namespace NLogicLib
{
	class UserManager;
	class LobbyManager;
	class PacketProcess;

	class Main
	{
	public:
		Main();
		~Main();

		ERROR_CODE Init();

		void Run();
		void Stop();

	private:
		ERROR_CODE LoadConfig();

		void Release();

	private:
		bool m_IsRun = false;

		std::unique_ptr<NServerNetLib::ServerConfig> mp_ServerConfig;
		std::unique_ptr<NServerNetLib::ILog> mp_Logger;

		std::unique_ptr<NServerNetLib::ITcpNetwork> mp_Network;
		std::unique_ptr<PacketProcess> mp_PacketProc;
		std::unique_ptr<UserManager> mp_UserMgr;
		std::unique_ptr<LobbyManager> mp_LobbyMgr;
	};
}