#include <algorithm>

#include "../ServerNetLib/ILog.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/Packet.h"
#include "../../Common/ErrorCode.h"

#include "User.h"
#include "Room.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	Room::Room() {}
	Room::~Room() {}

	void Room::Init(const short index, short int maxUserCount)
	{
		//룸의 Index와 해당 룸에서의 MaxUserCount를 정의
		m_Index = index;
		m_MaxUserCount = maxUserCount;

		m_pGame = std::make_unique<Game>( );
	}

	void Room::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
	{
		//네트워크 클래스와 로그 클래스 설정
		mp_RefNetwork = pNetwork;
		mp_RefLogger = pLogger;
	}

	void Room::Clear()
	{
		m_IsUsed = false;
		m_Title = L"";
		m_UserList.clear();
	}

	Game* Room::GetGameObj( )
	{
		return m_pGame.get( );
	}

	//방을 새로 만듦.
	ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
	{
		//새로 만들려고 하는 방이 이미 사용 중인 방이라면 이는 문제가 있다.
		if (m_IsUsed)
		{
			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
		}

		//방을 활성화시키고 타이틀 지정.
		m_IsUsed = true;
		m_Title = pRoomTitle;

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::EnterUser(User* pUser)
	{
		if (m_IsUsed == false)
		{
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		if (m_UserList.size() == m_MaxUserCount)
		{
			return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
		}

		m_UserList.push_back(pUser);
		
		return ERROR_CODE::NONE;
	}

	ERROR_CODE Room::LeaveUser(const short userIndex)
	{
		if (m_IsUsed == false)
		{
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		auto iter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
		if (iter == std::end(m_UserList))
		{
			return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
		}

		m_UserList.erase(iter);

		if (m_UserList.empty())
		{
			//User가 아무도 없으면 이 방은 날려버림
			Clear();
		}

		return ERROR_CODE::NONE;
	}


	void Room::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
	{
		for (auto pUser : m_UserList)
		{
			if (pUser->GetIndex() == passUserindex)
			{
				continue;
			}

			//List Box를 사용하여 채팅 메시지를 차곡차곡 쌓는게 좋다.
			//보내는 사람 입장에서는 자기가 보낸게 성공하면 Text를 쌓아주고
			//다시 받는 입장이라면 받는게 성공하면 listbox에 역시 추가하면 된다.
			mp_RefNetwork->SendData(pUser->GetSessionIndex(), packetId, dataSize, pData);
		}
	}

	void Room::NotifyEnterUserInfo(const int userIndex, const char* pszUserID)
	{
		NCommon::PktRoomEnterUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
		
		SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, userIndex);
	}

	void Room::NotifyLeaveUserInfo(const char* pszUserID)
	{
		if (m_IsUsed == false)
		{
			return;
		}

		NCommon::PktRoomLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
	}

	void Room::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
	{
		NCommon::PktRoomChatNtf pkt;

		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		//MAX Chatting Size를 체크해야한다.!(악의적 목적이 있을 수 있다)
		//도배금지하는 기능도 넣으면 좋다.(초당 채팅 한번만 보내야한다.)
		//send 와 send 사이에 시간 체크하는 것이 좋다.
		//필터링 기능도 있으면 좋다.
		//클라이언트단에서 체크하는 경우가 대부분이다.(왜냐하면 아주 크리티컬한 것은 아니기때문이다)
		//그리고 서버에서는 채팅내용들을 일정기간 저장해 놓아야한다.
		wcsncpy_s(pkt.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_ROOM_CHAT_MSG_SIZE);

		//나를 뺸 나머지에게 모두 보내야한다. 그리고 성공했는지의 여부를 확인해야함.
		SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
	}

	void Room::Update( )
	{
		if(m_pGame->GetState( ) == GameState::ING)
		{
			if(m_pGame->CheckSelectTime( ))
			{
				//선택 안하는 사람이 지도록 한다.
				//로비에서도 업데이트를 호출하여 모든 방을 확인해보아야한다.(LobbyManager에 update함수 만들어서 사용)
				//한번에 모든 로비가 아니라, 특정 시간당 로비 하나씩 하나씩 순차적으로 검사.
				//다시 정리하면 로비 매니저에서는 한번에 모든 로비를 조사하지 말것.(서버에 부하가 갈 수 있음)
			}
		}
	}
	
}