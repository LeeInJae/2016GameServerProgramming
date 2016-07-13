#include "../../Common/Packet.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "Room.h"
#include "Game.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
	{
		CHECK_START
			auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.p_RefData;
		NCommon::PktRoomEnterRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		Room* pRoom = nullptr;

		// 룸을 만드는 경우라면 룸을 만든다
		if(reqPkt->IsCreate)
		{
			//Lobby에서 사용할 수 있는 룸을 하나 얻어온다.
			pRoom = pLobby->CreateRoom( );
			
			//로비에서 만들 수 있는 룸이 없다면 빽
			if(pRoom == nullptr) 
			{
				CHECK_ERROR( ERROR_CODE::ROOM_ENTER_EMPTY_ROOM );
			}

			//로비에서 새로이 만들 수 있따면 룸을 만듦.
			auto ret = pRoom->CreateRoom( reqPkt->RoomTitle );
			if(ret != ERROR_CODE::NONE) 
			{
				CHECK_ERROR( ret );
			}
		}
		else
		{
			//룸을 만드는 경우가 아니라면, 해당 룸 정보를 받아오면됨
			pRoom = pLobby->GetRoom( reqPkt->RoomIndex );
			if(pRoom == nullptr) 
			{
				CHECK_ERROR( ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX );
			}
		}

		/*
		//방을 들어가는 사람은 인덱스가 없당!!!!
		//{
			auto pRoom = pLobby->GetRoom( reqPkt->RoomIndex );
			if(pRoom == nullptr) {
				CHECK_ERROR( ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX );
			}
		//}

		// 룸을 만드는 경우라면 룸을 만든다
		if (reqPkt->IsCreate)
		{
			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
			if (ret != ERROR_CODE::NONE) {
				CHECK_ERROR(ret);
			}
		}
		*/
		auto enterRet = pRoom->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(enterRet);
		}

		// 유저 정보를 룸에 들어왔다고 변경한다.
		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

		// 로비에 유저가 나갔음을 알린다
		pLobby->NotifyLobbyLeaveUserInfo(pUser);

		// 로비에 룸 정보를 통보한다.
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		// 룸에 새 유저 들어왔다고 알린다
		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
	{
		// RoomLeave 패킷 정의

		// User 정보를 받아옴

		// 유저정보 valid 체크

		// 현재 유저가 룸에 있는지 확인

		// 유저가 속한 로비정보 얻어옴

		// 로비에서 룸 정보를 얻어옴

		// 룸에서 해당 user가 나가는 것처리

		// 유저 정보를 로비로 변경

		// 룸에 유저 정보가 나갔음을 알림

		// 로비에 새로운 유저 들어왔음을 알림(방금 룸에서 나간 유저는 로비로..)

		// 로비에 바뀐 방 정보알림

		return ERROR_CODE::NONE;

		/*
		CHECK_START
			NCommon::PktRoomLeaveRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();

		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		auto leaveRet = pRoom->LeaveUser(userIndex);
		if (leaveRet != ERROR_CODE::NONE) {
			CHECK_ERROR(leaveRet);
		}

		// 유저 정보를 로비로 변경
		pUser->EnterLobby(lobbyIndex);

		// 룸에 유저가 나갔음을 통보
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		// 로비에 새로운 유저가 들어왔음을 통보
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		// 로비에 바뀐 방 정보를 통보
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	*/	
	}
	
	ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
	{
		CHECK_START
		
		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.p_RefData;
		NCommon::PktRoomChatRes resPkt;

		//유저 정보를 받아옴
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		//해당 유저가 현재의 룸에 있는지 확인.
		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		//룸 정보를 얻어와서 룸에 있는 유저들에게 메시지를 전달한다.
		pRoom->NotifyChat(pUser->GetSessionIndex(), pUser->GetID().c_str(), reqPkt->Msg);

		//ROOM_CHAT_RES 패킷을 보낸다.
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomMasterGameStart( PacketInfo packetInfo )
	{
		CHECK_START

		auto reqPkt = ( NCommon::PktRoomMasterGameStartReq* )packetInfo.p_RefData;
		NCommon::PktRoomMasterGameStartRes resPkt;

		//유저 정보를 받아옴
		auto pUserRet = m_pRefUserMgr->GetUser( packetInfo.SessionIndex );
		auto errorCode = std::get<0>( pUserRet );

		if(errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR( errorCode );
		}

		auto pUser = std::get<1>( pUserRet );

		//해당 유저가 현재의 룸에 있는지 확인.
		if(pUser->IsCurDomainInRoom( ) == false) {
			CHECK_ERROR( ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN );
		}

		auto lobbyIndex = pUser->GetLobbyIndex( );
		auto pLobby = m_pRefLobbyMgr->GetLobby( lobbyIndex );
		if(pLobby == nullptr) {
			CHECK_ERROR( ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX );
		}

		auto pRoom = pLobby->GetRoom( pUser->GetRoomIndex( ) );
		if(pRoom == nullptr) {
			CHECK_ERROR( ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX );
		}

		//방장이 있는지 확인
		if(pRoom->IsMaster( ) == false)
		{
			CHECK_ERROR( ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_MASTER );
		}

		//방의 상태가 게임을 안하는 중인지?
		if(pRoom->GetGameObj( )->GeState( ) != GameState::NONE)
		{
			CHECK_ERROR( ERROR_CODE::ROOM_MASTER_GAME_START_GAME_INVALID_STATE );
		}

		//유저 수를 체크할 것(방의 인원이 2명인가?)
		if(pRoom->GetUserCount( ) != 2)
		{
			CHECK_ERROR( ERROR_CODE::ROOM_MASTER_GAME_START_GAME_INVALID_USER_COUNT );

		}

		//방의 다른 유저에게 방장이 게임 시작 요청을 했음을 알리고
		//방의 상태를 변경
		pRoom->GetGameObj( )->SetState( GameObject::START );
		
		//보기 -> 작업목록 하면 TODO 나옴
		//TODO : 로비의 유저에게 방의 상태 변경 통보

		pRoom->SendToAllUser( ( short )PACKET_ID::ROOM_MASTER_GAME_START_NTF , 0 , nullptr, pUser->GetIndex( ) );

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError( __result );
		m_pRefNetwork->SendData( packetInfo.SessionIndex , ( short )PACKET_ID::ROOM_MASTER_GAME_START_RES , sizeof( resPkt ) , ( char* )&resPkt );
		return ( ERROR_CODE )__result;
	}

	ERROR_CODE PacketProcess::RoomGameStart( PacketInfo packetInfo )
	{
		CHECK_START

		auto reqPkt = ( NCommon::PktRoomGameStartReq* )packetInfo.p_RefData;
		NCommon::PktRoomGameStartRes resPkt;

		//유저 정보를 받아옴
		auto pUserRet = m_pRefUserMgr->GetUser( packetInfo.SessionIndex );
		auto errorCode = std::get<0>( pUserRet );

		if(errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR( errorCode );
		}

		auto pUser = std::get<1>( pUserRet );

		//해당 유저가 현재의 룸에 있는지 확인.
		if(pUser->IsCurDomainInRoom( ) == false) {
			CHECK_ERROR( ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN );
		}

		auto lobbyIndex = pUser->GetLobbyIndex( );
		auto pLobby = m_pRefLobbyMgr->GetLobby( lobbyIndex );
		if(pLobby == nullptr) {
			CHECK_ERROR( ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX );
		}

		auto pRoom = pLobby->GetRoom( pUser->GetRoomIndex( ) );
		if(pRoom == nullptr) {
			CHECK_ERROR( ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX );
		}

		//방의 상태가 게임을 안하는 중인지?
		if(pRoom->GetGameObj( )->GeState( ) != GameState::STARTING)
		{
			CHECK_ERROR( ERROR_CODE::ROOM_GAME_START_GAME_INVALID_STATE );

		}
		//이미 게임 시작 요청을 했는가?

		//방에서 게임 시작 요청한 유저 리스트에 등록

		
		//방의 다른 유저에게 게임 시작 요청을 했음을 알림
		
		//요청자에게 답변을 보낸다.
		pRoom->SendToAllUser( ( short )PACKET_ID::ROOM_GAME_START_NTF , 0 , nullptr , pUser->GetIndex( ) );

		// 게임 시작 가능한가?(방의 모든 요청자가 게임 시작을요청한경우)
		// 시작이면 게임 상태 변화(게임중으로 GamseState::ING)
		//게임 시작 패킷 보내기
		//방의 상태 변경 로비에 알리고
		//게임의 선택 시작 시간 설정(
		{

		}
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError( __result );
		m_pRefNetwork->SendData( packetInfo.SessionIndex , ( short )PACKET_ID::ROOM_GAME_START_RES , sizeof( resPkt ) , ( char* )&resPkt );
		return ( ERROR_CODE )__result;
	}
}