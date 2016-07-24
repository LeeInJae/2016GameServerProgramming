// Fill out your copyright notice in the Description page of Project Settings.

#include "ThirdPerson.h"
#include "NetworkClient.h"
#include "Public/EngineGlobals.h"
#include "Runtime/Engine/Classes/Engine/Engine.h"
#include "Runtime/Networking/Public/Common/TcpSocketBuilder.h"

#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Endpoint.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h"
#include "Runtime/Core/Public/HAL/UnrealMemory.h"

#include <string>



bool UNetworkClient::Init()
{
	m_RecvBuffer.SetNum(RECV_BUFFER_SIZE);
	m_SendBuffer.SetNum(SEND_BUFFER_SIZE);

	if (CreateSocket() == false)
		return false;

	if (SetServerInfo() == false)
		return false;

	if (ConnectToServer() == false)
		return false;
	
	
	/*
	for(int32 i =0;i<1000;++i)
	{
		FString Serialized = TEXT("loadPlayer|1");
		TCHAR* SerializedChar = Serialized.GetCharArray().GetData();
		int32 Size = FCString::Strlen(SerializedChar);
		int32 Sent = 0;
		int32 Read = 0;

		TArray<uint8> ReceivedData;
		
	}
	*/

	return true;
}

bool UNetworkClient::CreateSocket()
{
	m_Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("cow"), false);
	if (m_Socket == nullptr)
		return false;

	int NewSize;
	m_Socket->SetReceiveBufferSize(RECV_BUFFER_SIZE, NewSize);
	m_Socket->SetSendBufferSize(SEND_BUFFER_SIZE, NewSize);

	return true;
}

bool UNetworkClient::SetServerInfo()
{
	m_Address = TEXT("127.0.0.1");
	m_Port = 3000;
	FIPv4Address::Parse(m_Address, m_IP);

	return true;
}

bool UNetworkClient::ConnectToServer()
{
	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Addr->SetIp(m_IP.Value);
	Addr->SetPort(m_Port);
	while (1)
	{
		m_IsConncted = m_Socket->Connect(*Addr);
		if (m_IsConncted == true)
			break;
	}

	return true;
}

void UNetworkClient::SendSocket(const uint16 PacketID, const uint16 DataSize, uint8* pData) 
{
	TArray<uint8> SendBuffer;
	SendBuffer.SetNum(MAX_PACKET_SIZE);
	FPacketHeader PacketHeader{ PacketID, DataSize };
	FMemory::Memcpy((uint8*)&SendBuffer[0], (uint8*)&PacketHeader, PACKET_HEADER_SIZE);

	if (DataSize > 0)
	{
		FMemory::Memcpy((uint8*)&SendBuffer[PACKET_HEADER_SIZE], pData, DataSize);
	}

	int32 SendSize = 0;
	
	bool Successful = m_Socket->Send((uint8*)&SendBuffer[0], PACKET_HEADER_SIZE + DataSize, SendSize);
	if (Successful == false)
		return;
}

FString UNetworkClient::StringFromBinaryArray(const TArray<uint8>& BinaryArray)
{
	//Create a string from a byte array!
	const std::string cstr(reinterpret_cast<const char*>(BinaryArray.GetData()), BinaryArray.Num());

	//FString can take in the c_str() of a std::string
	return FString(cstr.c_str());
}

void UNetworkClient::Tick(float DeltaTime)
{
	if (m_IsConncted)
	{
		RecvSocket();
		RecvBuffProcess();
		TArray<uint8> DummySendBytes;
		DummySendBytes.SetNum(15);
		for (int32 i = 0; i < 15; ++i)
		{
			DummySendBytes[i] = 5;
		}

		SendSocket(1, 15, (uint8*)&DummySendBytes);
	}
}

void UNetworkClient::RecvSocket()
{
	TArray<uint8> RecvBuffer;
	RecvBuffer.SetNum(RECV_BUFFER_SIZE);
	int32 RecvSize = 0;


	bool Successful = m_Socket->Recv((uint8*)&RecvBuffer[0], RECV_BUFFER_SIZE, RecvSize);

	if (RecvSize == 0)
	{
		DisConnect();
		return;
	}

	if (RecvSize < 0)
	{
		if (ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode() != SE_EWOULDBLOCK)
		{
			return;
		}
	}

	if ((m_RecvSize + RecvSize) >= RECV_BUFFER_SIZE)
	{
		return;
	}

	FMemory::Memcpy((uint8*)&m_RecvBuffer[m_RecvSize], (uint8*)&RecvBuffer[0], RecvSize);
	m_RecvSize += RecvSize;

	//DebugPrint(StringFromBinaryArray(RecvBuffer));
}

void UNetworkClient::RecvBuffProcess()
{
	auto ReadPos		= 0;
	const auto DataSize = m_RecvSize;

	FPacketHeader* pPktHeader;

	while ((DataSize - ReadPos) > PACKET_HEADER_SIZE)
	{
		pPktHeader = (FPacketHeader*)&m_RecvBuffer[ReadPos];
		ReadPos += PACKET_HEADER_SIZE;

		if (pPktHeader->BodySize > (DataSize - ReadPos))
		{
			break;
		}

		if (pPktHeader->BodySize > MAX_PACKET_SIZE)
		{
			return;
		}

		AddPacketQueue(pPktHeader->ID, pPktHeader->BodySize, &m_RecvBuffer[ReadPos]);

		ReadPos += pPktHeader->BodySize;

	}

	m_RecvSize -= ReadPos;
	if (m_RecvSize > 0)
	{
		FMemory::Memcpy((uint8*)&m_RecvBuffer[0], (uint8*)&m_RecvBuffer[ReadPos], m_RecvSize);
	}
}

void UNetworkClient::AddPacketQueue(const uint16 PacketID, const uint16 PacketBodySize, uint8* pDataPos)
{
	FRecvPacketInfo PacketInfo;
	PacketInfo.PacketID = PacketID;
	PacketInfo.PacketBodySize = PacketBodySize;
	PacketInfo.pData.SetNum(PacketBodySize);
	FMemory::Memcpy(PacketInfo.pData.GetData(), pDataPos, PacketBodySize);
	
	m_PacketQueue.Enqueue(PacketInfo);
}

void UNetworkClient::DisConnect()
{
	if (m_IsConncted)
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(m_Socket);
		Clear();
	}
}

void UNetworkClient::Clear()
{
	m_IsConncted = false;
	m_RecvSize = 0;
	m_PacketQueue.Empty();
}