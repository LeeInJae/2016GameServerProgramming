#pragma once

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>

#include "../../../Common/ErrorCode.h"
#include "../../../Common/PacketID.h"
#include "../../../Common/Packet.h"

//���� ��Ŷ�� ������� Socket Recv Buffer�� ����� ����
//���� Recv Buffer�� ����� �۰��س�����, �������� ���� �����͵��� ����ؼ� �������ϴ� ��찡 �ִ�
//�� �̸��̸� �޾Ƴ��´ٴ� �ǹ̷� �˳��ϰ� ����??

const int MAX_PACKET_SIZE		= 1024;
const int MAX_SOCK_RECV_BUFFER	= 8016;

//����ü�� 1����Ʈ ���� ��Ű�ڴٴ� �ǹ�(����ü�� ������� ����ȭ�� ���� ������)
#pragma pack(push, 1)
struct PacketHeader
{
	short ID; //�� ��Ŷ�� � ��Ŷ���� ������ �� �ִ� ID
	short BodySize; //���� ������ ������
};
#pragma pack(pop)

//��Ŷ ��� ������ ����
const int PACKET_HEADER_SIZE = sizeof(PacketHeader);

struct RecvPacketInfo
{
	RecvPacketInfo() {}

	short PacketID			= 0;
	short PacketBodySIze	= 0;
	char* p_Data			= nullptr;
};

class TcpNetwork
{
public:
	TcpNetwork()	{}
	~TcpNetwork()	{}

	bool ConnectTo(const char* hostIP, int port)
	{
		WSADATA WsaData;
		if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
		{
			return false;
		}

		m_Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_Socket == INVALID_SOCKET)
		{
			return false;
		}

		SOCKADDR_IN ServerAddr;
		ZeroMemory(&ServerAddr, sizeof(ServerAddr));
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_port = htons(port);
		inet_pton(AF_INET, hostIP, (void*)&ServerAddr.sin_addr.s_addr);

		//connect ������ 0 return
		if (connect(m_Socket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) != 0)
		{
			return false;
		}

		m_IsConnected = true;

		NonBlock(m_Socket);

		m_Thread = std::thread([&]() {Update(); });

		return true;
	}

	bool IsConnected()
	{
		return m_IsConnected;
	}

	void DisConnect()
	{
		if (m_IsConnected)
		{
			closesocket(m_Socket);

			Clear();
		}

		if (m_Thread.joinable())
		{
			m_Thread.join();
		}
	}

	void SendPacket(const short PacketID, const short DataSize, char* p_Data)
	{
		//������ ���� Packet�� ��������� byte �迭
		char Data[MAX_PACKET_SIZE] = { 0, };

		//Header ����
		PacketHeader Header = { PacketID, DataSize };

		//������ ���� Hedaer ������ ��Ŷ�� �� ������ �ؼ� Header ���� ����
		memcpy(Data, (char*)&Header, PACKET_HEADER_SIZE);

		//�׸��� ������ ���������ϴ� �����Ͱ� �ִ� ��쿡�� ��� ���� �������ʹ� ���� �����͸� ä���� Packet�� ������
		if (DataSize > 0)
		{
			memcpy(&Data[PACKET_HEADER_SIZE], p_Data, DataSize);
		}

		//���� ��� + ���� ������ �� ���� ��Ŷ�� ������
		send(m_Socket, Data, DataSize + PACKET_HEADER_SIZE, 0);
	}

	//Thread���� �Ź� ��Ʈ��ũ update�� ���ִ� �κ��̴�
	void Update()
	{
		while (m_IsConnected)
		{
			RecvData();
			RecvBufferProcess();
		}
	}

	RecvPacketInfo GetPacket()
	{
		std::lock_guard<std::mutex> guard(m_Mutex);

		if (m_PacketQueue.empty())
		{
			return RecvPacketInfo();
		}

		auto Packet = m_PacketQueue.front();
		m_PacketQueue.pop_front();

		return Packet;
	}

private:
	//Socket�� ������ �Ѱܾ����������� ������ �� �ִ�(���� �׷���..)
	//������ Socket�� Ŀ�ο��� �����ϴ� ��ü��, �츮�� �� ��ü�� ���� �ڵ鰪�� ��� �ִ°��̴�
	//�� 100�� Ŀ�� ��ü�� ������ ������ �ٲ��ּ���~ ��� ���̱⶧����
	//������ �ѱ� �ʿ䰡����.
	void NonBlock(SOCKET Socket)
	{
		u_long val = 1L;
		ioctlsocket(Socket, FIONBIO, (u_long*)&val);
	}

	void RecvData()
	{
		fd_set	read_set;
		timeval tv;
		tv.tv_sec	= 0;
		tv.tv_usec	= 100;

		FD_ZERO(&read_set);
		FD_SET(m_Socket, &read_set);
		
		//select�� �Ǿ� �ƱԸ�Ʈ�� ���������� ȣȯ���� ���� ���̹Ƿ� ���õ�
		
		//���ϰ��� 0�ϰ�� Ÿ��limit�̹Ƿ� �׳� �����Ͽ� �ٽ� select�� �ϵ��� �Ѵ�.
		if (select(0, &read_set, NULL, NULL, &tv) < 0)
		{
			return;
		}

		if (FD_ISSET(m_Socket, &read_set))
		{
			char RecvBuffer[MAX_PACKET_SIZE];
			auto RecvSize = recv(m_Socket, RecvBuffer, MAX_PACKET_SIZE, 0);

			//������ �����(����� ����)
			if (RecvSize == 0)
			{
				return;
			}

			if (RecvSize < 0)
			{
				//nonblicking socket�� �̿��Ͽ� recv�� �ϱ� ������ ���� recv�۾����̸� WSAEWOULDBLICK ������ ������
				//�������� �ƴ϶�� �ٸ� �����̹Ƿ� ���� ���� ����
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					return;
				}
			}

			//���� �����÷ο� üũ!
			if (MAX_SOCK_RECV_BUFFER <= (m_RecvSize + RecvSize))
			{
				return;
			}

			memcpy(&m_RecvBuffer[m_RecvSize], RecvBuffer, RecvSize);
			m_RecvSize += RecvSize;
		}
	}

	void RecvBufferProcess()
	{
		auto				ReadPos			= 0;
		const auto			DataSize		= m_RecvSize;
		PacketHeader*		p_PacketHeader	= nullptr;

		//�ϴ� Ŭ���̾�Ʈ���忡�� �޾ƾ��� �����Ͱ� ���������� Ŀ�� �ȴ�. ��������� ���϶�� ���� �����ʹ� ���⶧���̴�.
		while ((DataSize - ReadPos) > PACKET_HEADER_SIZE)
		{
			p_PacketHeader = (PacketHeader*)(&m_RecvBuffer[ReadPos]);
			ReadPos += PACKET_HEADER_SIZE;
			
			//�̳༮�� �츮�� ������ �ִ� ��Ŷ ������� ũ�� �� ���̴�. ���� ������.
			if (p_PacketHeader->BodySize > MAX_PACKET_SIZE)
			{
				return;
			}

			//���� �츮�� ������ ���� 120����Ʈ�� �޾Ҵٰ��غ���.
			//100����Ʈ + 100 ����Ʈ ���� ��Ŷ�� ���� 120(100 + 20)����Ʈ�� �������� �� �� �ִ�.
			//�̶� 100����Ʈ ¥���� ��Ŷ���� �����, 20����Ʈ ¥���� ��Ŷ���� ���� �� �����Ƿ� ���� �ɰ����� �����س��ƾ��Ѵ�
			//��, �ɰ����� ��Ŷ�� ���� �� �������� �����صξ� ������ �����Ͱ� �ö� ����ü�� ������־�� �Ѵ�.
			if (p_PacketHeader->BodySize > (DataSize - ReadPos))
			{
				break;
			}

			//�ϼ��� ��Ŷ�� ��Ŷ ť�� �������� �׾Ƶд�.
			AddPacketQueue(p_PacketHeader->ID, p_PacketHeader->BodySize, &m_RecvBuffer[ReadPos]);
			
			//���� ��ġ ����
			ReadPos += p_PacketHeader->BodySize;
		}

		//���� ���� �����Ϳ��� ��Ŷ�� ������� ���� �ɰ��� �������� ũ�� ����
		m_RecvSize -= ReadPos;

		//�ɰ��� �����Ͱ� �ִٸ� �����س��ٰ�(��Ȯ���� ������ ��ܹ���) ������ ���� �����Ϳ� ���ļ� �ٽ� �ϼ��� ��Ŷ�� ����� �ֵ�������.
		if (m_RecvSize > 0)
		{
			memcpy(m_RecvBuffer, &m_RecvBuffer[ReadPos], m_RecvSize);
		}
	}

	void AddPacketQueue(const short PacketID, const short BodySize, char* p_DataPos)
	{
		RecvPacketInfo PacketInfo;
		PacketInfo.PacketID = PacketID;
		PacketInfo.PacketBodySIze = BodySize;
		PacketInfo.p_Data = new char[BodySize];
		
		memcpy(PacketInfo.p_Data, p_DataPos, BodySize);

		std::lock_guard<std::mutex> guard(m_Mutex);
		m_PacketQueue.push_back(PacketInfo);
	}

	void Clear()
	{
		m_IsConnected = false;
		m_RecvSize = 0;
		m_PacketQueue.clear();
	}

	bool m_IsConnected = false;

	std::thread m_Thread;
	std::mutex m_Mutex;

	SOCKET m_Socket;

	int m_RecvSize = 0;
	char m_RecvBuffer[MAX_SOCK_RECV_BUFFER] = { 0, };

	//Packetqueue�� Queue�� �ƴ� Deque�� �� ������ �����ϱ�?
	//Queue�� Dequ�� ����� �ľ��ʿ�
	std::deque<RecvPacketInfo> m_PacketQueue;
};