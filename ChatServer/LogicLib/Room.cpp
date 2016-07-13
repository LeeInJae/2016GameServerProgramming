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
		//���� Index�� �ش� �뿡���� MaxUserCount�� ����
		m_Index = index;
		m_MaxUserCount = maxUserCount;

		m_pGame = std::make_unique<Game>( );
	}

	void Room::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
	{
		//��Ʈ��ũ Ŭ������ �α� Ŭ���� ����
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

	//���� ���� ����.
	ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
	{
		//���� ������� �ϴ� ���� �̹� ��� ���� ���̶�� �̴� ������ �ִ�.
		if (m_IsUsed)
		{
			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
		}

		//���� Ȱ��ȭ��Ű�� Ÿ��Ʋ ����.
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
			//User�� �ƹ��� ������ �� ���� ��������
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

			//List Box�� ����Ͽ� ä�� �޽����� �������� �״°� ����.
			//������ ��� ���忡���� �ڱⰡ ������ �����ϸ� Text�� �׾��ְ�
			//�ٽ� �޴� �����̶�� �޴°� �����ϸ� listbox�� ���� �߰��ϸ� �ȴ�.
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

		//MAX Chatting Size�� üũ�ؾ��Ѵ�.!(������ ������ ���� �� �ִ�)
		//��������ϴ� ��ɵ� ������ ����.(�ʴ� ä�� �ѹ��� �������Ѵ�.)
		//send �� send ���̿� �ð� üũ�ϴ� ���� ����.
		//���͸� ��ɵ� ������ ����.
		//Ŭ���̾�Ʈ�ܿ��� üũ�ϴ� ��찡 ��κ��̴�.(�ֳ��ϸ� ���� ũ��Ƽ���� ���� �ƴϱ⶧���̴�)
		//�׸��� ���������� ä�ó������ �����Ⱓ ������ ���ƾ��Ѵ�.
		wcsncpy_s(pkt.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_ROOM_CHAT_MSG_SIZE);

		//���� �A ���������� ��� �������Ѵ�. �׸��� �����ߴ����� ���θ� Ȯ���ؾ���.
		SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
	}

	void Room::Update( )
	{
		if(m_pGame->GetState( ) == GameState::ING)
		{
			if(m_pGame->CheckSelectTime( ))
			{
				//���� ���ϴ� ����� ������ �Ѵ�.
				//�κ񿡼��� ������Ʈ�� ȣ���Ͽ� ��� ���� Ȯ���غ��ƾ��Ѵ�.(LobbyManager�� update�Լ� ���� ���)
				//�ѹ��� ��� �κ� �ƴ϶�, Ư�� �ð��� �κ� �ϳ��� �ϳ��� ���������� �˻�.
				//�ٽ� �����ϸ� �κ� �Ŵ��������� �ѹ��� ��� �κ� �������� ����.(������ ���ϰ� �� �� ����)
			}
		}
	}
	
}