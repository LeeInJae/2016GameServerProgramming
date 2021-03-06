#pragma once

#include <vector>
#include <unordered_map>

#include "Room.h"

namespace NServerNetLib
{
	class ITcpNetwork;
}

namespace NServerNetLib
{
	class ILog;
}

namespace NCommon
{
	enum class ERROR_CODE : short;
}
using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	using TcpNet = NServerNetLib::ITcpNetwork;
	using ILog = NServerNetLib::ILog;

	class User;

	struct LobbyUser
	{
		short Index = 0;
		User* pUser = nullptr;
	};

	class Lobby
	{
	public:
		Lobby();
		virtual ~Lobby();
		void Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount);

		void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

		short GetIndex() { return m_LobbyIndex; }


		ERROR_CODE EnterUser(User* pUser);
		ERROR_CODE LeaveUser(const int userIndex);

		short GetUserCount();


		void NotifyLobbyEnterUserInfo(User* pUser);

		ERROR_CODE SendRoomList(const int sessionId, const short startRoomId);

		ERROR_CODE SendUserList(const int sessionId, const short startUserIndex);

		void NotifyLobbyLeaveUserInfo(User* pUser);

		//현재 로비에서 사용하지않고있는 룸을 찾아서 반환
		Room* CreateRoom( );
		Room* GetRoom(const short roomIndex);

		void NotifyChangedRoomInfo(const short roomIndex);

		auto MaxUserCount() { return (short)m_MaxUserCount; }

		auto MaxRoomCount() { return (short)m_RoomList.size(); }

		void NotifyChat( const int sessionIndex , const char* pszUserID , const wchar_t* pszMsg );

		void WhisperChat( const int sessionIndex , const char* pszUserID , const wchar_t* pszMsg, User* TargetUser);
	protected:
		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);
		void SendToUser( const short packetId , const short dataSize , char* pData , const int passUserindex = -1 );



	protected:
		User* FindUser(const int userIndex);

		ERROR_CODE AddUser(User* pUser);

		void RemoveUser(const int userIndex);


	protected:
		ILog* m_pRefLogger;
		TcpNet* m_pRefNetwork;


		short m_LobbyIndex = 0;

		short m_MaxUserCount = 0;
		std::vector<LobbyUser> m_UserList;
		std::unordered_map<int, User*> m_UserIndexDic;
		std::unordered_map<const char*, User*> m_UserIDDic;

		std::vector<Room> m_RoomList;
	};
}