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

//보낼 패킷의 사이즈와 Socket Recv Buffer의 사이즈를 지정
//만약 Recv Buffer의 사이즈를 작게해놓으면, 서버에서 보낸 데이터들을 계속해서 받지못하는 경우가 있다
//즉 미리미리 받아놓는다는 의미로 넉넉하게 잡음??

const int MAX_PACKET_SIZE		= 1024;
const int MAX_SOCK_RECV_BUFFER	= 8016;

//구조체를 1바이트 정렬 시키겠다는 의미(구조체의 사이즈와 최적화에 관한 선지식)
#pragma pack(push, 1)
struct PacketHeader
{
	short ID; //이 패킷이 어떤 패킷인지 구분할 수 있는 ID
	short BodySize; //실제 데이터 사이즈
};
#pragma pack(pop)

//패킷 헤더 사이즈 지정
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

		//connect 성공시 0 return
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
		//실제로 보낼 Packet을 만들기위한 byte 배열
		char Data[MAX_PACKET_SIZE] = { 0, };

		//Header 정의
		PacketHeader Header = { PacketID, DataSize };

		//위에서 만든 Hedaer 정보를 패킷의 맨 앞으로 해서 Header 정보 만듦
		memcpy(Data, (char*)&Header, PACKET_HEADER_SIZE);

		//그리고 실제로 보내고자하는 데이터가 있는 경우에는 헤더 다음 정보부터는 실제 데이터를 채워서 Packet을 만들자
		if (DataSize > 0)
		{
			memcpy(&Data[PACKET_HEADER_SIZE], p_Data, DataSize);
		}

		//이제 헤더 + 실제 데이터 로 만든 패킷을 보내자
		send(m_Socket, Data, DataSize + PACKET_HEADER_SIZE, 0);
	}

	//Thread에서 매번 네트워크 update를 쳐주는 부분이다
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
	//Socket을 참조로 넘겨야하지않을까 생각할 수 있다(나만 그랬나..)
	//하지만 Socket은 커널에서 관리하는 객체로, 우리는 이 객체에 대한 핸들값을 들고 있는것이다
	//즉 100번 커널 객체인 소켓의 정보를 바꿔주세요~ 라는 것이기때문에
	//참조로 넘길 필요가없다.
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
		
		//select의 맨앞 아규먼트는 리눅스와의 호환성을 위한 것이므로 무시됨
		
		//리턴값이 0일경우 타임limit이므로 그냥 리턴하여 다시 select를 하도록 한다.
		if (select(0, &read_set, NULL, NULL, &tv) < 0)
		{
			return;
		}

		if (FD_ISSET(m_Socket, &read_set))
		{
			char RecvBuffer[MAX_PACKET_SIZE];
			auto RecvSize = recv(m_Socket, RecvBuffer, MAX_PACKET_SIZE, 0);

			//연결이 끊긴것(우아한 종료)
			if (RecvSize == 0)
			{
				return;
			}

			if (RecvSize < 0)
			{
				//nonblicking socket을 이용하여 recv를 하기 때문에 아직 recv작업중이면 WSAEWOULDBLICK 에러가 나지만
				//그이유가 아니라면 다른 에러이므로 역시 연결 종료
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					return;
				}
			}

			//버퍼 오버플로우 체크!
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

		//일단 클라이언트입장에서 받아야할 데이터가 헤더사이즈보다 커야 된다. 헤더사이즈 이하라면 실제 데이터는 없기때문이다.
		while ((DataSize - ReadPos) > PACKET_HEADER_SIZE)
		{
			p_PacketHeader = (PacketHeader*)(&m_RecvBuffer[ReadPos]);
			ReadPos += PACKET_HEADER_SIZE;
			
			//이녀석은 우리가 정의한 최대 패킷 사이즈보다 크게 온 놈이다. 따라서 버린다.
			if (p_PacketHeader->BodySize > MAX_PACKET_SIZE)
			{
				return;
			}

			//만약 우리가 서버로 부터 120바이트를 받았다고해보자.
			//100바이트 + 100 바이트 실제 패킷이 현재 120(100 + 20)바이트로 합쳐져서 올 수 있다.
			//이때 100바이트 짜리는 패킷으로 만들되, 20바이트 짜리는 패킷으로 만들 수 없으므로 남는 쪼가리는 복사해놓아야한다
			//즉, 쪼가리로 패킷을 만들 수 없을때는 복사해두어 다음번 데이터가 올때 완전체로 만들어주어야 한다.
			if (p_PacketHeader->BodySize > (DataSize - ReadPos))
			{
				break;
			}

			//완성된 패킷은 패킷 큐에 차곡차곡 쌓아둔다.
			AddPacketQueue(p_PacketHeader->ID, p_PacketHeader->BodySize, &m_RecvBuffer[ReadPos]);
			
			//읽은 위치 갱신
			ReadPos += p_PacketHeader->BodySize;
		}

		//실제 받은 데이터에서 패킷을 만들고나서 남은 쪼가리 데이터의 크기 갱신
		m_RecvSize -= ReadPos;

		//쪼가리 데이터가 있다면 복사해놨다가(정확히는 앞으로 당겨버림) 다음번 오는 데이터와 합쳐서 다시 완성된 패킷을 만들수 있도록하자.
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

	//Packetqueue를 Queue가 아닌 Deque로 한 이유는 무엇일까?
	//Queue와 Dequ의 장단점 파악필요
	std::deque<RecvPacketInfo> m_PacketQueue;
};