#pragma once

#include <vector>
#include <string>
#include <memory>
#include "User.h"

namespace NServerNetLib { class ITcpNetwork; }
namespace NServerNetLib { class ILog; }
namespace NCommon { enum class ERROR_CODE : short; }

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	using TcpNet = NServerNetLib::ITcpNetwork;
	using ILog = NServerNetLib::ILog;

	class Game;

	class Room
	{
	public:
		Room();
		virtual ~Room();

		void Init(const short Index, short int MaxUserCount);

		void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

		void Clear();

		short GetIndex() { return m_Index; }

		bool IsUsed() { return m_IsUsed; }

		const wchar_t* GetTitle() { return m_Title.c_str(); }

		short MaxUserCount() { return m_MaxUserCount; }

		short GetUserCount() { return (short)m_UserList.size(); }

		ERROR_CODE CreateRoom(const wchar_t* pRoomTitle);

		ERROR_CODE EnterUser(User* pUser);

		ERROR_CODE LeaveUser(const short UserIndex);

		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);

		void NotifyEnterUserInfo(const int userIndex, const char* pszUserID);

		void NotifyLeaveUserInfo(const char* pszUserID);

		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);


		bool IsMaster( )
		{
			return true;
		}

		Game* GetGameObj( );
		void Update( );
	private:
		ILog* mp_RefLogger;
		TcpNet* mp_RefNetwork;

		short m_Index = -1;
		short m_MaxUserCount;

		bool m_IsUsed = false;
		std::wstring m_Title;


		//Vector를 쓴 이유는 갯수가 적기때문에 그냥 써도 된다.(룸에 들어가는 Max User 수는 8명 이기에)
		std::vector<User*> m_UserList;

		std::unique_ptr<Game> m_pGame;
	};
}