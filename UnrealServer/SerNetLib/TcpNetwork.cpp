#include <iostream>
#include <stdio.h>
#include <vector>
#include <deque>

#include "ILog.h"
#include "TcpNetwork.h"

namespace NServerNetLib
{
	TcpNetwork::TcpNetwork() {}
	TcpNetwork::~TcpNetwork()
	{
		for (auto& Client : m_ClientSessionPool)
		{
			if (Client.p_RecvBuffer) 
			{
				delete[] Client.p_RecvBuffer;
			}

			if (Client.p_SendBuffer)
			{
				delete[] Client.p_SendBuffer;
			}
		}
	}

	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* p_Config, ILog* p_Logger)
	{
		memcpy(&m_Config, p_Config, sizeof(ServerConfig));

		mp_RefLogger = p_Logger;

		auto InitRet = InitServerSocket();
		if (InitRet != NET_ERROR_CODE::NONE)
		{
			return InitRet;
		}

		auto BindListenRet = BindListen(p_Config->Port, p_Config->BackLogCount);
		if (BindListenRet != NET_ERROR_CODE::NONE)
		{
			return BindListenRet;
		}

		FD_ZERO(&m_ReadFDs);
		FD_SET(m_ServerSockFD, &m_ReadFDs);

		CreateSessionPool(p_Config->MaxClientCount + p_Config->ExtraClientCount);

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::Release()
	{
		WSACleanup();
	}

	//완성된 패킷을 하나 빼온다.
	RecvpacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvpacketInfo PacketInfo;

		if (m_PacketQueue.empty() == false)
		{
			PacketInfo = m_PacketQueue.front();
			m_PacketQueue.pop_front();
		}

		return PacketInfo;
	}

	void TcpNetwork::ForcingClose(const int SessionIndex)
	{
		if (m_ClientSessionPool[SessionIndex].IsConnected() == false)
		{
			return;
		}

		CloseSession(SOCKET_CLOSE_CASE::FORCING_CLOSE, m_ClientSessionPool[SessionIndex].SocketFD, SessionIndex);
	}

	void TcpNetwork::Run()
	{
		auto Read_set	= m_ReadFDs;
		auto Write_set	= m_ReadFDs;

		timeval Timeout{ 0, 1000 };
		auto SelectResult = select(0, &Read_set, &Write_set, 0, &Timeout);

		auto IsFdSetChanged = RunCheckSelectResult(SelectResult);
		if (IsFdSetChanged == false)
		{
			return;
		}

		if (FD_ISSET(m_ServerSockFD, &Read_set))
		{
			NewSession();
		}

		RunCheckSelectClients(Read_set, Write_set);
	}

	bool TcpNetwork::RunCheckSelectResult(const int Result)
	{
		//요너석은 타임아웃 난 경우
		if (Result == 0)
		{
			return false;
		}
		//요너석은 에러가 난 경우
		else if (Result == -1)
		{
			// TODO: 로그 남기기
			return false;
		}

		return true;
	}

	void TcpNetwork::RunCheckSelectClients(fd_set& Read_set, fd_set& Write_set)
	{
		for (int i = 0; i < static_cast<int>(m_ClientSessionPool.size()); ++i)
		{
			auto& Session = m_ClientSessionPool[i];
			if (Session.IsConnected() == false)
			{
				continue;
			}

			SOCKET FD = Session.SocketFD;
			auto SessionIndex = Session.Index;
			
			//Connection맺은 놈이라면 이녀석으로부터 읽을 것이있나 처리.
			auto RetReceive = RunProcessReceive(SessionIndex, FD, Read_set);
			if (RetReceive == false)
			{
				continue;
			}

			RunProcessWrite(SessionIndex, FD, Write_set);
		}
	}

	bool TcpNetwork::RunProcessReceive(const int SessionIndex, const SOCKET FD, fd_set& Read_set)
	{
		if (!FD_ISSET(FD, &Read_set))
		{
			return true;
		}

		auto Ret = RecvSocket(SessionIndex);
		if (Ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, FD, SessionIndex);
			return false;
		}

		Ret = RecvBufferProcess(SessionIndex);
		if (Ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_ERCV_BUFFER_PROCESS_ERROR, FD, SessionIndex);
			return false;
		}

		return true;
	}

	NET_ERROR_CODE TcpNetwork::SendData(const int SessionIndex, const short PacketID, const short Size, const char* p_Msg)
	{
		auto& Session = m_ClientSessionPool[SessionIndex];

		auto Pos = Session.SendSize;

		if ((Pos + Size + PACKET_HEADER_SIZE) > m_Config.MaxClientSendBufferSize)
		{
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;
		}

		//헤더 붙이고 실데이터 붙여서 패킷하나 만든다.
		PacketHeader PktHeader{ PacketID, Size };
		memcpy(&Session.p_SendBuffer[Pos], (char*)&PktHeader, PACKET_HEADER_SIZE);
		memcpy(&Session.p_SendBuffer[Pos + PACKET_HEADER_SIZE], p_Msg, Size);
		Session.SendSize += (Size + PACKET_HEADER_SIZE);

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::CreateSessionPool(const int MaxClientCount)
	{
		for (int i = 0; i < MaxClientCount; ++i)
		{
			ClientSession Session;
			ZeroMemory(&Session, sizeof(Session));
			Session.Index = i;
			Session.p_RecvBuffer = new char[m_Config.MaxClientRecvBufferSize];
			Session.p_SendBuffer = new char[m_Config.MaxClientSendBufferSize];

			m_ClientSessionPool.push_back(Session);
			m_ClientSessionPoolIndex.push_back(Session.Index);
		}
	}

	int TcpNetwork::AllocClientSessionIndex()
	{
		if (m_ClientSessionPoolIndex.empty())
		{
			return -1;
		}

		int Index = m_ClientSessionPoolIndex.front();
		m_ClientSessionPoolIndex.pop_front();

		return Index;
	}

	void TcpNetwork::ReleaseSessionIndex(const int Index)
	{
		m_ClientSessionPoolIndex.push_back(Index);
		m_ClientSessionPool[Index].Clear();
	}

	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			return NET_ERROR_CODE::SERVER_WINSOCK_STARTUP_ERROR;
		}

		m_ServerSockFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_ServerSockFD < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		auto n = 1;
		if (setsockopt(m_ServerSockFD, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUESEADDR_FAIL;
		}

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::BindListen(short Port, int BacklogCount)
	{
		SOCKADDR_IN Server_Addr;
		ZeroMemory(&Server_Addr, sizeof(Server_Addr));
		Server_Addr.sin_family			= AF_INET;
		Server_Addr.sin_addr.s_addr		= htonl(INADDR_ANY);
		Server_Addr.sin_port			= htons(Port);

		if (bind(m_ServerSockFD, (SOCKADDR*)&Server_Addr, sizeof(Server_Addr)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}

		mp_RefLogger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockFD(%d)", __FUNCTION__, m_ServerSockFD);

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::NewSession()
	{
		SOCKADDR_IN Client_Addr;
		auto Client_Len		= static_cast<int>(sizeof(Client_Addr));
		auto Client_SockFD	= accept(m_ServerSockFD, (SOCKADDR*)&Client_Addr, &Client_Len);

		if (Client_SockFD < 0)
		{
			mp_RefLogger->Write(LOG_TYPE::L_ERROR, "%s | Wrong Socket %d cannot accept", __FUNCTION__, Client_SockFD);
			return NET_ERROR_CODE::ACCEPT_API_ERROR;
		}

		auto NewSessionindex = AllocClientSessionIndex();
		if (NewSessionindex < 0)
		{
			mp_RefLogger->Write(LOG_TYPE::L_WARN, "%s | Client_SockFD(%d) >= MAX_SESSION", __FUNCTION__, Client_SockFD);

			//수용 못함 , 세션 크기 다참
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, Client_SockFD, -1);
			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}

		char ClientIP[MAX_IP_LEN] = { 0, };
		//binary IP 주소를 dot 10진수 주소로 변경
		inet_ntop(AF_INET, &(Client_Addr.sin_addr), ClientIP, MAX_IP_LEN - 1);

		SetSockOption(Client_SockFD);

		FD_SET(Client_SockFD, &m_ReadFDs);

		ConnectedSession(NewSessionindex, static_cast<int>(Client_SockFD), ClientIP);

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::ConnectedSession(const int SessionIndex, const int FD, const char* p_IP)
	{
		//Q. 이 시퀀스는 무엇을 의미하는것일까?
		++m_ConnectSeq;

		auto& Session		= m_ClientSessionPool[SessionIndex];
		Session.Seq			= m_ConnectSeq;
		Session.SocketFD	= FD;

		memcpy(Session.IP, p_IP, MAX_IP_LEN - 1);

		++m_ConnectedSessionCount;

		AddPacketQueue(SessionIndex, static_cast<short>(PACKET_ID::NTF_SYS_CONNECT_SESSINO), 0, nullptr);

		mp_RefLogger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%d), m_ConnectSeq(%d), IP(%s)", __FUNCTION__, FD, m_ConnectSeq, p_IP);
	}

	void TcpNetwork::SetSockOption(const SOCKET FD)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;

		setsockopt(FD, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		//세션에서 데이터를 가질 Buffer 사이즈랑 별도로, 소켓의 버퍼사이즈를 설정
		int RecvBuffSize	= m_Config.MaxClientSockOptRecvBufferSize;
		int SendBufSize		= m_Config.MaxClientSockOptSendBufferSize;

		setsockopt(FD, SOL_SOCKET, SO_RCVBUF, (char*)&RecvBuffSize, sizeof(RecvBuffSize));
		setsockopt(FD, SOL_SOCKET, SO_SNDBUF, (char*)&SendBufSize, sizeof(SendBufSize));
	}

	void TcpNetwork::CloseSession(const SOCKET_CLOSE_CASE CloseCase, const SOCKET SockFD, const int SessionIndex)
	{
		if (CloseCase == SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(SockFD);
			FD_CLR(SockFD, &m_ReadFDs);
			return;
		}

		if (m_ClientSessionPool[SessionIndex].IsConnected() == false)
		{
			return;
		}

		closesocket(SockFD);
		FD_CLR(SockFD, &m_ReadFDs);

		m_ClientSessionPool[SessionIndex].Clear();
		--m_ConnectedSessionCount;
		ReleaseSessionIndex(SessionIndex);

		//실제로 네트웤상의 데이터를 주고 받은건 아니지만, 패킷을 해석하여 처리하는 식으로 서버가 동작하도록 하기위해서(일관성 유지의 의미로 패킷으로 처리)
		AddPacketQueue(SessionIndex, static_cast<short>(PACKET_ID::NTF_SYS_CLOSE_SESSION), 0, nullptr);
	}

	NET_ERROR_CODE TcpNetwork::RecvSocket(const int SessionIndex)
	{
		auto& Session	= m_ClientSessionPool[SessionIndex];
		auto FD			= static_cast<SOCKET>(Session.SocketFD);

		if (Session.IsConnected() == false)
		{
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;
		}

		int RecvPos = 0;

		if (Session.RemainingDataSize > 0)
		{
			//이전에 패킷을 완성할수 없어서(아직 덜받은 부분들)을 맨앞으로 다시 땡겨준다!
			memcpy(Session.p_RecvBuffer, &Session.p_RecvBuffer[Session.PrevReadPosInRecvBuffer], Session.RemainingDataSize);
			RecvPos += Session.RemainingDataSize;
		}

		//그리고 잘린 부분이후부터 다시 받아야 된다!!
		auto RecvSize = recv(FD, &Session.p_RecvBuffer[RecvPos], (MAX_PACKET_BODY_SIZE * 2), 0);
		if (RecvSize == 0)
		{
			return NET_ERROR_CODE::RECV_REMOTE_CLOSE;
		}

		if (RecvSize < 0)
		{
			auto Error = WSAGetLastError();
			if (Error != WSAEWOULDBLOCK)
			{
				return NET_ERROR_CODE::RECV_API_ERROR;
			}
			else
			{
				return NET_ERROR_CODE::NONE;
			}
		}

		//Session에서 받아서 처리해야할 데이터가 RecvSize만큼이다.
		Session.RemainingDataSize += RecvSize;

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::RecvBufferProcess(const int SessionIndex)
	{
		auto& Session = m_ClientSessionPool[SessionIndex];

		auto ReadPos = 0;
		const auto DataSize = Session.RemainingDataSize;
		PacketHeader* pPktHeader;

		//최소 헤더 이상의 데이터는 있어야 뭘 읽을수가 있지!
		while ((DataSize - ReadPos) >= PACKET_HEADER_SIZE)
		{
			//일단 헤더만큼읽고!
			pPktHeader = (PacketHeader*)&Session.p_RecvBuffer[ReadPos];
			ReadPos += PACKET_HEADER_SIZE;

			//이제 실제 보디의 내용을 읽자.
			if (pPktHeader->BodySize > 0)
			{
				//아직 완벽하게 Body 만큼의 데이터를 다 못받았다(네트웍상에서)
				//이럴땐 헤더읽은것도 다시 뱉고, 다음에 보디를 다 받으면 그때 패킷을 완성시킨다.
				if (pPktHeader->BodySize > (DataSize - ReadPos))
				{
					ReadPos -= PACKET_HEADER_SIZE;
					break;
				}

				if (pPktHeader->BodySize > MAX_PACKET_BODY_SIZE)
				{
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
				}
			}

			//실제 데이터 만큼을 패킷으로 만들어 저장한다.
			AddPacketQueue(SessionIndex, pPktHeader->ID, pPktHeader->BodySize, &Session.p_RecvBuffer[ReadPos]);

			ReadPos += pPktHeader->BodySize;
		}

		//실제 얼마만큼 읽었고, 읽어야될 데이터 사이즈는 얼마나 되는지 체크.
		Session.RemainingDataSize -= ReadPos;
		Session.PrevReadPosInRecvBuffer = ReadPos;

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::AddPacketQueue(const int SessionIndex, const short PacketID, const short BodySize, char* p_DataPos)
	{
		RecvpacketInfo PacketInfo;
		PacketInfo.SessionIndex = SessionIndex;
		PacketInfo.PacketID = PacketID;
		PacketInfo.PacketBodySize = BodySize;
		PacketInfo.p_RefData = p_DataPos;

		m_PacketQueue.push_back(PacketInfo);
	}

	void TcpNetwork::RunProcessWrite(const int SessionIndex, const SOCKET FD, fd_set& Write_set)
	{
		if(!FD_ISSET(FD, &Write_set))
		{
			return;
		}

		auto RetSend = FlushSendBuff(SessionIndex);
		if (RetSend.Error != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, FD, SessionIndex);
		}
	}

	NetError TcpNetwork::FlushSendBuff(const int SessionIndex)
	{
		auto& Session = m_ClientSessionPool[SessionIndex];
		auto FD = static_cast<SOCKET>(Session.SocketFD);

		if (Session.IsConnected() == false)
		{
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);
		}

		auto Result = SendSocket(FD, Session.p_SendBuffer, Session.SendSize);

		if (Result.Error != NET_ERROR_CODE::NONE)
		{
			return Result;
		}

		//실제 보내려고 하는 데이터와, 실제 보낸 데이터를 계산. 
		//다 못보냈으면, 다 못보낸 데이터사이즈와 데이터를 다시 갱신.
		auto SendSize = Result.Value;
		if (SendSize < Session.SendSize)
		{
			memmove(&Session.p_SendBuffer[0], &Session.p_SendBuffer[SendSize], Session.SendSize - SendSize);
			Session.SendSize -= SendSize;
		}
		else
		{
			Session.SendSize = 0;
		}

		return Result;
	}

	NetError TcpNetwork::SendSocket(const SOCKET FD, const char* p_Msg, const int Size)
	{
		NetError Result(NET_ERROR_CODE::NONE);
		auto RFDs = m_ReadFDs;

		//보낼거 없으면 걍 리턴 ㄱㄱ
		if (Size <= 0)
		{
			return Result;
		}

		Result.Value = send(FD, p_Msg, Size, 0);

		if (Result.Value <= 0)
		{
			Result.Error = NET_ERROR_CODE::SEND_SIZE_ZERO;
		}

		return Result;
	}
}