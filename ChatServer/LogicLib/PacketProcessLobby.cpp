#include "../../Common/Packet.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::LobbyEnter(PacketInfo packetInfo)
	{
		CHECK_START

			auto reqPkt = (NCommon::PktLobbyEnterReq*)packetInfo.p_RefData;
		NCommon::PktLobbyEnterRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE)
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLogIn() == false)
		{
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);
		}

		auto pLobby = m_pRefLobbyMgr->GetLobby(reqPkt->LobbyId);
		if (pLobby == nullptr)
		{
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);
		}

		auto enterRet = pLobby->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE)
		{
			CHECK_ERROR(enterRet);
		}

		pLobby->NotifyLobbyEnterUserInfo(pUser);

		resPkt.MaxUserCount = pLobby->MaxUserCount();
		resPkt.MaxRoomCount = pLobby->MaxRoomCount();
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyRoomList(PacketInfo packetInfo)
	{
		CHECK_START

			auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE)
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false)
		{
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);
		}

		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr)
		{
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_LOBBY_INDEX);
		}

		auto reqPkt = (NCommon::PktLobbyRoomListReq*)packetInfo.p_RefData;

		pLobby->SendRoomList(pUser->GetSessionIndex(), reqPkt->StartRoomIndex);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyRoomListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyUserList(PacketInfo packetInfo)
	{
		CHECK_START

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
		}

		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_LOBBY_INDEX);
		}

		auto reqPkt = (NCommon::PktLobbyUserListReq*)packetInfo.p_RefData;

		pLobby->SendUserList(pUser->GetSessionIndex(), reqPkt->StartUserIndex);

		return ERROR_CODE::NONE;
	CHECK_ERR:
		NCommon::PktLobbyUserListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyLeave(PacketInfo packetInfo)
	{
		CHECK_START
		NCommon::PktLobbyLeaveRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLogIn() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LIST_INVALID_DOMAIN);
		}

		m_pRefLobbyMgr->SendLobbyListInfo(packetInfo.SessionIndex);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyChat( PacketInfo packetInfo )
	{
	CHECK_START

		auto reqPkt = ( NCommon::PktLobbyChatReq* )packetInfo.p_RefData;
		NCommon::PktLobbyChatRes resPkt;

		//유저 정보를 받아옴
		auto pUserRet = m_pRefUserMgr->GetUser( packetInfo.SessionIndex );
		auto errorCode = std::get<0>( pUserRet );

		if(errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR( errorCode );
		}

		auto pUser = std::get<1>( pUserRet );

		//해당 유저가 현재의 로비에 있는지 확인.
		if(pUser->IsCurDomainInLobby( ) == false) {
			CHECK_ERROR( ERROR_CODE::LOBBY_CHAT_INVALID_DOMAIN );
		}

		auto lobbyIndex = pUser->GetLobbyIndex( );
		auto pLobby = m_pRefLobbyMgr->GetLobby( lobbyIndex );
		if(pLobby == nullptr) {
			CHECK_ERROR( ERROR_CODE::LOBBY_CHAT_INVALID_LOBBY_INDEX );
		}

		//룸 정보를 얻어와서 룸에 있는 유저들에게 메시지를 전달한다.
		pLobby->NotifyChat( pUser->GetSessionIndex( ) , pUser->GetID( ).c_str( ) , reqPkt->Msg );

		//ROOM_CHAT_RES 패킷을 보낸다.
		m_pRefNetwork->SendData( packetInfo.SessionIndex , ( short )PACKET_ID::LOBBY_CHAT_RES , sizeof( resPkt ) , ( char* )&resPkt );

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError( __result );
		m_pRefNetwork->SendData( packetInfo.SessionIndex , ( short )PACKET_ID::LOBBY_CHAT_RES , sizeof( resPkt ) , ( char* )&resPkt );
		return ( ERROR_CODE )__result;
	}

	ERROR_CODE PacketProcess::LobbyWhisperChat( PacketInfo packetInfo )
	{
		CHECK_START

		auto reqPkt = ( NCommon::PktLobbyWhisperChatReq* )packetInfo.p_RefData;
		NCommon::PktLobbyChatRes resPkt;

		//유저 정보를 받아옴
		auto pUserRet = m_pRefUserMgr->GetUser( packetInfo.SessionIndex );
		auto errorCode = std::get<0>( pUserRet );

		if(errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR( errorCode );
		}

		//상대방을 미리 찾고, 로비가 서로 같은지 확인 후 Lobby의 SendToUser를 찾아서 보내준다.
		auto pWhisperTargetUser = m_pRefUserMgr->FindUserHelpFunction( reqPkt->UserID );		
		if(pWhisperTargetUser == nullptr)
		{
			errorCode = ERROR_CODE::LOBBY_WHISPER_CHAT_INVALID_TARGET;
			CHECK_ERROR( errorCode );
		}
		
		auto pUser = std::get<1>( pUserRet );

		//해당 유저가 현재의 로비에 있는지 확인.
		if(pUser->IsCurDomainInLobby( ) == false) {
			CHECK_ERROR( ERROR_CODE::LOBBY_WHISPER_CHAT_INVALID_DOMAIN );
		}

		auto lobbyIndex = pUser->GetLobbyIndex( );
		auto pLobby = m_pRefLobbyMgr->GetLobby( lobbyIndex );
		if(pLobby == nullptr) {
			CHECK_ERROR( ERROR_CODE::LOBBY_WHISPER_CHAT_INVALID_LOBBY_INDEX );
		}

		//룸 정보를 얻어와서 룸에 있는 유저들에게 메시지를 전달한다.
		pLobby->NotifyChat( pUser->GetSessionIndex( ) , pUser->GetID( ).c_str( ) , reqPkt->Msg );

		//ROOM_CHAT_RES 패킷을 보낸다.
		m_pRefNetwork->SendData( packetInfo.SessionIndex , ( short )PACKET_ID::LOBBY_WHISPER_CHAT_RES , sizeof( resPkt ) , ( char* )&resPkt );

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError( __result );
		m_pRefNetwork->SendData( packetInfo.SessionIndex , ( short )PACKET_ID::LOBBY_CHAT_RES , sizeof( resPkt ) , ( char* )&resPkt );
		return ( ERROR_CODE )__result;
	}
}