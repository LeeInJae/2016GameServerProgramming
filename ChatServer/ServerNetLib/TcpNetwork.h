#pragma once

#define FD_SETSIZE 1024

#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <vector>
#include <deque>
#include <unordered_map>
#include "ITcpNetwork.h"

namespace NServerNetLib
{
	class TcpNetwork : public ITcpNetwork
	{
	public:
		TcpNetwork();
		virtual ~TcpNetwork();

		NET_ERROR_CODE	Init(const ServerConfig* p_Config, ILog* p_Logger) override;
		NET_ERROR_CODE	SendData(const int SessionIndex, const short PacketID, const short Size, const char* p_Msg) override;
		void Run() override;
		RecvPacketInfo GetPacketInfo() override;

	protected:
		NET_ERROR_CODE InitServerSocket();
		NET_ERROR_CODE BindListen(short Port, int BacklogCount);

		int AllocClientSessionIndex();
		void ReleaseSessionIndex(const int Index);

		void CreateSessionPool(const int MaxClientCount);
		NET_ERROR_CODE NewSession();
		void SetSockOption(const SOCKET fd);
		void ConnectedSession(const int SessionIndex, const int FD, const char* p_IP);

		void CloseSession(const SOCKET_CLOSE_CASE CloseCase, const SOCKET SockFD, const int SessionIndex);

		NET_ERROR_CODE RecvSocket(const int SessionIndex);
		NET_ERROR_CODE RecvBufferProcess(const int SessionIndex);
		void AddPacketQueue(const int SessionIndex, const short PacketID, const short BodySize, char* p_DataPos);

		void RunProcessWrite(const int SessionIndex, const SOCKET FD, fd_set& Write_set);
		NetError FlushSendBuff(const int SessionIndex);
		NetError SendSocket(const SOCKET FD, const char* p_Msg, const int Size);

		bool RunCheckSelectResult(const int result);
		void RunCheckSelectClients(fd_set& Exc_set, fd_set& Read_set, fd_set& Write_set);
		bool RunProcessReceive(const int SessionIndex, const SOCKET FD, fd_set& Read_set);

	protected:
		ServerConfig m_Config;

		SOCKET m_ServerSockFD;

		fd_set m_ReadFDs;
		size_t m_ConnectedSessionCount = 0;

		int64_t m_ConnectSeq = 0;

		std::vector<ClientSession> m_ClientSessionPool;
		std::deque<int> m_ClientSessionPoolIndex;

		std::deque<RecvPacketInfo> m_PacketQueue;

		ILog* mp_RefLogger;
	};
};
