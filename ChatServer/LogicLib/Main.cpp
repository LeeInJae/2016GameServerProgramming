#include <thread>
#include <chrono>

#include "../ServerNetLib/ServerNetErrorCode.h"
#include "../ServerNetLib/Define.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "ConsoleLogger.h"
#include "LobbyManager.h"
#include "PacketProcess.h"
#include "UserManager.h"
#include "Main.h"

using LOG_TYPE = NServerNetLib::LOG_TYPE;
using NET_ERROR_CODE = NServerNetLib::NET_ERROR_CODE;

namespace NLogicLib
{
	Main::Main()
	{
	}

	Main::~Main()
	{
		Release();
	}

	ERROR_CODE Main::Init()
	{
		mp_Logger = std::make_unique<ConsoleLog>();

		LoadConfig();

		mp_Network = std::make_unique<NServerNetLib::TcpNetwork>();
		auto result = mp_Network->Init(mp_ServerConfig.get(), mp_Logger.get());

		if (result != NET_ERROR_CODE::NONE)
		{
			mp_Logger->Write(LOG_TYPE::L_ERROR, "%s | Init Fail. NetErrorCode(%s)", __FUNCTION__, (short)result);
			return ERROR_CODE::MAIN_INIT_NETWORK_INIT_FAIL;
		}


		mp_UserMgr = std::make_unique<UserManager>();
		mp_UserMgr->Init(mp_ServerConfig->MaxClientCount);

		mp_LobbyMgr = std::make_unique<LobbyManager>();
		mp_LobbyMgr->Init({ mp_ServerConfig->MaxLobbyCount,
			mp_ServerConfig->MaxLobbyUserCount,
			mp_ServerConfig->MaxRoomCountByLobby,
			mp_ServerConfig->MaxRoomUserCount },
			mp_Network.get(), mp_Logger.get());

		mp_PacketProc = std::make_unique<PacketProcess>();
		mp_PacketProc->Init(mp_Network.get(), mp_UserMgr.get(), mp_LobbyMgr.get(), mp_Logger.get());

		m_IsRun = true;

		mp_Logger->Write(LOG_TYPE::L_INFO, "%s | Init Success. Server Run", __FUNCTION__);
		return ERROR_CODE::NONE;
	}

	void Main::Release() { }

	void Main::Stop()
	{
		m_IsRun = false;
	}

	//Running 스레드(worker)
	void Main::Run()
	{
		while (m_IsRun)
		{
			mp_Network->Run();

			while (true)
			{
				//패킷단위로 작업 처리
				auto packetInfo = mp_Network->GetPacketInfo();

				//패킷이 없으면 할일없음
				if (packetInfo.PacketID == 0)
				{
					break;
				}

				//패킷있으면 처리
				mp_PacketProc->Process(packetInfo);
			}

			mp_PacketProc->StateCheck( );
			//이게 없으면 CPU가 정말 정말 쉴새없이 돌아서 점유율이 하늘을 찌를 것이다.
			//내가 진짜 별로 할게 없으면 다른 스레드에서 코어 쓰라는 형식
			//즉 이함수롤 호출하면 이 스레드는 블락
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
		}
	}

	ERROR_CODE Main::LoadConfig()
	{
		mp_ServerConfig = std::make_unique<NServerNetLib::ServerConfig>();

		wchar_t sPath[MAX_PATH] = { 0, };
		::GetCurrentDirectory(MAX_PATH, sPath);

		wchar_t inipath[MAX_PATH] = { 0, };
		_snwprintf_s(inipath, _countof(inipath), _TRUNCATE, L"%s\\ServerConfig.ini", sPath);

		mp_ServerConfig->Port = (unsigned short)GetPrivateProfileInt(L"Config", L"Port", 0, inipath);
		mp_ServerConfig->BackLogCount = GetPrivateProfileInt(L"Config", L"BackLogCount", 0, inipath);
		mp_ServerConfig->MaxClientCount = GetPrivateProfileInt(L"Config", L"MaxClientCount", 0, inipath);

		mp_ServerConfig->MaxClientSockOptRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptRecvBufferSize", 0, inipath);
		mp_ServerConfig->MaxClientSockOptSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSockOptSendBufferSize", 0, inipath);
		mp_ServerConfig->MaxClientRecvBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientRecvBufferSize", 0, inipath);
		mp_ServerConfig->MaxClientSendBufferSize = (short)GetPrivateProfileInt(L"Config", L"MaxClientSendBufferSize", 0, inipath);

		mp_ServerConfig->ExtraClientCount = GetPrivateProfileInt(L"Config", L"ExtraClientCount", 0, inipath);
		mp_ServerConfig->MaxLobbyCount = GetPrivateProfileInt(L"Config", L"MaxLobbyCount", 0, inipath);
		mp_ServerConfig->MaxLobbyUserCount = GetPrivateProfileInt(L"Config", L"MaxLobbyUserCount", 0, inipath);
		mp_ServerConfig->MaxRoomCountByLobby = GetPrivateProfileInt(L"Config", L"MaxRoomCountByLobby", 0, inipath);
		mp_ServerConfig->MaxRoomUserCount = GetPrivateProfileInt(L"Config", L"MaxRoomUserCount", 0, inipath);

		mp_Logger->Write(NServerNetLib::LOG_TYPE::L_INFO, "%s | Port(%d), Backlog(%d)", __FUNCTION__, mp_ServerConfig->Port, mp_ServerConfig->BackLogCount);
		return ERROR_CODE::NONE;
	}

}