#pragma once
#include <time.h>
#include <vector>
#include <chrono>
#include "../ServerNetLib/ITcpNetwork.h"
#include "../ServerNetLib/ILog.h"

namespace NLogicLib
{
	struct ConnectedUser
	{
		void Clear( )
		{
			m_IsLoginSuccess = false;
			m_TimeConnected = 0;
		}

		bool	m_IsLoginSuccess = false;
		time_t	m_TimeConnected = 0;
	};

	class ConnectedUserManager
	{
		using TcpNet = NServerNetLib::ITcpNetwork;
		using ILog = NServerNetLib::ILog;

	public:
		ConnectedUserManager( ) {}
		virtual ~ConnectedUserManager( ) {}

		void Init( const int MaxSessionIndex , TcpNet* TcpNetwork , ILog* Log )
		{
			m_pRefLogger = Log;
			m_pRefNetwork = TcpNetwork;

			for(int i = 0; i < MaxSessionIndex; ++i)
			{
				ConnctedUserList.emplace_back( ConnectedUser( ) );
			}
		}

		void SetConnectSession( const int sessionIndex )
		{
			time( &ConnctedUserList[ sessionIndex ].m_TimeConnected );
		}

		void SetLogin( const int sessionIndex )
		{
			ConnctedUserList[ sessionIndex ].m_IsLoginSuccess = true;
		}

		void SetDisconnectSession( const int sessionIndex )
		{
			ConnctedUserList[ sessionIndex ].Clear( );
		}

		void LoginCheck( )
		{
			auto CurTime = std::chrono::system_clock::now( );
			auto DiffTime = std::chrono::duration_cast<std::chrono::milliseconds>( CurTime - m_LatestLoginCheckTime);

			if(DiffTime.count( ) < 60)
				return;
			else
			{
				m_LatestLoginCheckTime = CurTime;
			}

			auto CurSecTime = std::chrono::system_clock::to_time_t( CurTime );
			const auto MaxSessionCount = ConnctedUserList.size( );

			if(m_LatestLogincheckIndex > MaxSessionCount)
			{
				m_LatestLogincheckIndex = -1;
			}

			++m_LatestLogincheckIndex;

			auto LastCheckIndex = m_LatestLogincheckIndex + 100;
			if(LastCheckIndex > MaxSessionCount)
			{
				LastCheckIndex = MaxSessionCount;
			}

			for(; m_LatestLogincheckIndex < LastCheckIndex; ++m_LatestLogincheckIndex)
			{
				auto i = m_LatestLogincheckIndex;
				if(ConnctedUserList[ i ].m_IsLoginSuccess || ConnctedUserList[ i ].m_TimeConnected == 0)
				{
					continue;
				}

				auto Diff = CurSecTime - ConnctedUserList[ i ].m_TimeConnected;

				if(Diff >= 3)
				{
					m_pRefLogger->Write( NServerNetLib::LOG_TYPE::L_WARN , "%s | Login Wait Time Over. sessionIndex(%d)." , __FUNCTION__ , i );
					m_pRefNetwork->ForcingClose( i );
				}

			}
		}
	public:
		TcpNet* m_pRefNetwork;
		ILog* m_pRefLogger;
		std::vector<ConnectedUser> ConnctedUserList;

		std::chrono::system_clock::time_point m_LatestLoginCheckTime = std::chrono::system_clock::now( );
		int m_LatestLogincheckIndex = -1;
	};

}