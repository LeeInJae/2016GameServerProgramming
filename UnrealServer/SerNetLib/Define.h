#pragma once

namespace NServerNetLib
{
	//������ �������� ���� ����ü
	//���� ini���Ϸ� �����Ѵ�.
	const int MAX_IP_LEN		= 32;
	const int MAX_PACKET_SIZE	= 1024;

	struct ServerConfig
	{
		unsigned short	Port;
		int				BackLogCount;

		int				MaxClientCount;
		int				ExtraClientCount;

		short			MaxClientSockOptRecvBufferSize;
		short			MaxClientSockOptSendBufferSize;
		short			MaxClientRecvBufferSize;
		short			MaxClientSendBufferSize;
	};

	struct RecvpacketInfo
	{
		int SessionIndex = 0;
		short PacketID = 0;
		short PacketBodySize = 0;
		char* p_RefData = 0;
	};

	/*
	struct RecvpacketInfo2
	{
	int SessionIndex = 0;
	short PacketID = 0;
	short PacketBodySize = 0;
	char* p_RefData = 0;
	};
	*/

	enum class SOCKET_CLOSE_CASE : short
	{
		SESSION_POOL_EMPTY = 1,
		SELECT_ERROR = 2,
		SOCKET_RECV_ERROR = 3,
		SOCKET_ERCV_BUFFER_PROCESS_ERROR = 4,
		SOCKET_SEND_ERROR = 5,
		FORCING_CLOSE = 6,
	};

	enum class PACKET_ID : short
	{
		NTF_SYS_CLOSE_SESSION = 3,
	};

	//��༮�� ���ؼ� 1����Ʈ ����(push , pop ���� ������ �Ǿ��ִ�)
	//push, pop �����ָ�, ������ ������ ��� ����ü�� ���ĵǰԵȴ�.
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