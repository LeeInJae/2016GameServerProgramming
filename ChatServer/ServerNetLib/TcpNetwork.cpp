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

	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo PacketInfo;
		if (m_PacketQueue.empty() == false)
		{
			PacketInfo = m_PacketQueue.front();
			m_PacketQueue.pop_front();
		}

		return PacketInfo;
	
	}

	void TcpNetwork::Run()
	{
		auto Read_Set = m_ReadFDs;
		auto Write_Set = m_ReadFDs;
		auto Exc_Set = m_ReadFDs;

		timeval Timeout{ 0, 1000 };
		auto SelectResult = select(0, &Read_Set, &Write_Set, &Exc_Set, &Timeout);

		auto IsFDSetChange = RunCheckSelectResult(SelectResult);
		if (IsFDSetChange == false)
		{
			return;
		}

		if (FD_ISSET(m_ServerSockFD, &Read_Set))
		{
			NewSession();
		}
		else
		{
			RunCheckSelectClients(Exc_Set, Read_Set, Write_Set);
		}
	}

	bool TcpNetwork::RunCheckSelectResult(const int Result)
	{
		if (Result == 0)
		{
			return false;
		}
		else if (Result == -1)
		{
			//error log
			return false;
		}

		return true;
	}

	void TcpNetwork::RunCheckSelectClients(fd_set& Exc_Set, fd_set& Read_Set, fd_set& Write_Set)
	{
		for (unsigned int i = 0; i < m_ClientSessionPool.size(); ++i)
		{
			auto& Session = m_ClientSessionPool[i];

			if (Session.IsConnected() == false)
			{
				continue;
			}

			SOCKET FD = Session.SocketFD;
			auto SessionIndex = Session.Index;

			if (FD_ISSET(FD, &Exc_Set))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, FD, SessionIndex);
				continue;
			}

			auto RetReceive = RunProcessReceive(SessionIndex, FD, Read_Set);
			if (RetReceive == false)
			{
				continue;
			}

			RunProcessWrite(SessionIndex, FD, Write_Set);
		}
	}

	bool TcpNetwork::RunProcessReceive(const int SessionIndex, const SOCKET FD, fd_set& Read_Set)
	{
		if (!FD_ISSET(FD, &Read_Set))
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
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, FD, SessionIndex);
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

		PacketHeader p_PacketHeader{ PacketID, Size };
		memcpy(&Session.p_SendBuffer[Pos], (char*)&p_PacketHeader, PACKET_HEADER_SIZE);
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
		WSADATA WsaData;
		if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
		{
			return NET_ERROR_CODE::SERVER_WINSOCK_STARTUP_ERROR;
		}

		m_ServerSockFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_ServerSockFD == INVALID_SOCKET)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		auto Val = 1;
		if (setsockopt(m_ServerSockFD, SOL_SOCKET, SO_REUSEADDR, (char*)&Val, sizeof(Val)) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::BindListen(short Port, int BacklogCount)
	{
		SOCKADDR_IN ServerAddr;
		ZeroMemory(&ServerAddr, sizeof(ServerAddr));
		ServerAddr.sin_family		= AF_INET;
		ServerAddr.sin_addr.s_addr	= htonl(INADDR_ANY);
		ServerAddr.sin_port			= htons(Port);

		//if (bind(m_ServerSockFD, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr) == SOCKET_ERROR))
		if (bind(m_ServerSockFD, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}

		if (listen(m_ServerSockFD, BacklogCount) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}

		mp_RefLogger->Write(LOG_TYPE::L_INFO, "%s | Liste. ServerSockFD(%d)", __FUNCTION__, m_ServerSockFD);
		
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::NewSession()
	{
		SOCKADDR_IN Client_Addr;
		auto Client_Len = static_cast<int>(sizeof(Client_Addr));
		auto Client_SockFD = accept(m_ServerSockFD, (SOCKADDR*)&Client_Addr, &Client_Len);
		
		if (Client_SockFD == INVALID_SOCKET)
		{
			mp_RefLogger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket %d cannot accept", __FUNCTION__, Client_SockFD);
			return NET_ERROR_CODE::ACCEPT_API_ERROR;
		}

		auto NewSessionIndex = AllocClientSessionIndex();
		if (NewSessionIndex < 0)
		{
			mp_RefLogger->Write(LOG_TYPE::L_WARN, "%s | Client_SockFD(%d) >= MAX_SESSION", __FUNCTION__, Client_SockFD);

			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, Client_SockFD, -1);

			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}

		char ClientIP[MAX_IP_LEN] = { 0, };
		inet_ntop(AF_INET, &(Client_Addr.sin_addr), ClientIP, MAX_IP_LEN - 1);

		SetSockOption(Client_SockFD);

		FD_SET(Client_SockFD, &m_ReadFDs);

		ConnectedSession(NewSessionIndex, (int)Client_SockFD, ClientIP);

		std::cout << "new Session!";
		
		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::ConnectedSession(const int SessionIndex, const int FD, const char* p_IP)
	{
		++m_ConnectSeq;

		auto& Session = m_ClientSessionPool[SessionIndex];
		Session.Seq = m_ConnectSeq;
		Session.SocketFD = FD;
		memcpy(Session.IP, p_IP, MAX_IP_LEN - 1);

		++m_ConnectedSessionCount;

		mp_RefLogger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%d), m_ConnectSeq(%d), IP(%s)", __FUNCTION__, FD, m_ConnectSeq, p_IP);
	}

	void TcpNetwork::SetSockOption(const SOCKET FD)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(FD, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int Size1 = m_Config.MaxClientSockOptRecvBufferSize;
		int Size2 = m_Config.MaxClientSockOptSendBufferSize;
		setsockopt(FD, SOL_SOCKET, SO_RCVBUF, (char*)&Size1, sizeof(Size1));
		setsockopt(FD, SOL_SOCKET, SO_SNDBUF, (char*)&Size2, sizeof(Size2));
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

		AddPacketQueue(SessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	NET_ERROR_CODE TcpNetwork::RecvSocket(const int SessionIndex)
	{
		auto& Session = m_ClientSessionPool[SessionIndex];
		auto FD = static_cast<SOCKET>(Session.SocketFD);

		if (Session.IsConnected() == false)
		{
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;
		}

		int RecvPos = 0;

		if (Session.RemainingDataSize > 0)
		{
			RecvPos += Session.RemainingDataSize;
			Session.RemainingDataSize = 0;
		}

		auto RecvSize = recv(FD, &Session.p_RecvBuffer[RecvPos], (MAX_PACKET_SIZE * 2), 0);

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

		Session.RemainingDataSize += RecvSize;
		
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::RecvBufferProcess(const int SessionIndex)
	{
		auto& Session = m_ClientSessionPool[SessionIndex];

		auto ReadPos = 0;
		const auto DataSize = Session.RemainingDataSize;
		PacketHeader* p_PacketHeader;

		while ((DataSize - ReadPos) >= PACKET_HEADER_SIZE)
		{
			p_PacketHeader = (PacketHeader*)&Session.p_RecvBuffer[ReadPos];
			ReadPos += PACKET_HEADER_SIZE;

			if (p_PacketHeader->BodySize > 0)
			{
				if (p_PacketHeader->BodySize > (DataSize - ReadPos))
				{
					break;
				}

				if (p_PacketHeader->BodySize > MAX_PACKET_SIZE)
				{
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
				}
			}

			AddPacketQueue(SessionIndex, p_PacketHeader->ID, p_PacketHeader->BodySize, &Session.p_RecvBuffer[ReadPos]);

			ReadPos += p_PacketHeader->BodySize;
		}

		Session.RemainingDataSize -= ReadPos;

		if (Session.RemainingDataSize > 0)
		{
			memcpy(Session.p_RecvBuffer, &Session.p_RecvBuffer[ReadPos], Session.RemainingDataSize);
		}

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::AddPacketQueue(const int SessionIndex, const short PacketID, const short BodySize, char* p_DataPos)
	{
		RecvPacketInfo PacketInfo;
		PacketInfo.SessionIndex		= SessionIndex;
		PacketInfo.PacketID			= PacketID;
		PacketInfo.PacketBodySize	= BodySize;
		PacketInfo.p_RefData		= p_DataPos;

		m_PacketQueue.push_back(PacketInfo);
	}

	void TcpNetwork::RunProcessWrite(const int SessionIndex, const SOCKET FD, fd_set& Write_Set)
	{
		if (!FD_ISSET(FD, &Write_Set))
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

		auto SendSize = Result.Value;
		if (SendSize < Session.SendSize)
		{
			memmove(Session.p_SendBuffer, &Session.p_SendBuffer[SendSize], Session.SendSize - SendSize);

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
		auto ReadFDs = m_ReadFDs;

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