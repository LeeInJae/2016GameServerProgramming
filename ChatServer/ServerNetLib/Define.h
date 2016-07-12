#pragma once

namespace NServerNetLib
{
	struct ServerConfig
	{
		unsigned short Port;
		int BackLogCount;

		int MaxClientCount;
		int ExtraClientCount;

		short MaxClientSockOptRecvBufferSize;
		short MaxClientSockOptSendBufferSize;
		short MaxClientRecvBufferSize;
		short MaxClientSendBufferSize;

		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	const int MAX_IP_LEN		= 32;
	const int MAX_PACKET_SIZE	= 1024;

	struct ClientSession
	{
		bool IsConnected()
		{
			return SocketFD > 0 ? true : false;
		}

		void Clear()
		{
			Seq						= 0;
			SocketFD				= 0;
			IP[0]					= '\0';
			RemainingDataSize		= 0;
			SendSize				= 0;
		}

		int Index					= 0;
		long long Seq				= 0;
		int	SocketFD				= 0;
		char IP[MAX_IP_LEN]			= { 0, };

		char*	p_RecvBuffer		= nullptr;
		int		RemainingDataSize	= 0;

		char*	p_SendBuffer		= nullptr;
		int		SendSize			= 0;	
	};

	struct RecvPacketInfo
	{
		int SessionIndex		= 0;
		short PacketID			= 0;
		short PacketBodySize	= 0;
		char* p_RefData			= 0;
	};

	struct RecvPacketInfo2
	{
		int SessionIndex		= 0;
		short PacketID			= 0;
		short PacketBodySize	= 0;
		short* p_RefData		= 0;
	};

	enum class SOCKET_CLOSE_CASE : short
	{
		SESSION_POOL_EMPTY					= 1,
		SELECT_ERROR						= 2,
		SOCKET_RECV_ERROR					= 3,
		SOCKET_RECV_BUFFER_PROCESS_ERROR	= 4,
		SOCKET_SEND_ERROR					= 5,
		FORCING_CLOSE = 6,
	};

	enum class PACKET_ID : short
	{
		NTF_SYS_CLOSE_SESSION = 3,
	};

#pragma pack(push, 1)
	struct PacketHeader
	{
		short ID;
		short BodySize;
	};

	const int PACKET_HEADER_SIZE = sizeof(PacketHeader);

	struct PacketSysCloseSession : PacketHeader
	{
		int SockFD;
	};
#pragma pack(pop)
}
