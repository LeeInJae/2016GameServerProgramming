// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Object.h"
#include "Networking.h"

#include "AllowWindowsPlatformTypes.h" 
#include <WinSock2.h>
#include "Runtime/Engine/Public/Tickable.h"
#include "Runtime/Sockets/Private/BSDSockets/SocketsBSD.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Core/Public/Containers/Queue.h"
#include "NetworkClient.generated.h"


#define DebugPrint(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text)
#define SEND_BUFFER_SIZE	1024
#define RECV_BUFFER_SIZE	1024
#define MAX_PACKET_SIZE		1024

#pragma pack(push, 1)
USTRUCT(BlueprintType)
struct THIRDPERSON_API FPacketHeader
{
	GENERATED_USTRUCT_BODY()

public:
	uint16 ID;
	uint16 BodySize;
};
#pragma pack(pop)

const int32 PACKET_HEADER_SIZE = sizeof(FPacketHeader);

USTRUCT(BlueprintType)
struct THIRDPERSON_API FRecvPacketInfo
{
	GENERATED_USTRUCT_BODY()

public:
	uint16 PacketID = 0;
	uint16 PacketBodySize = 0;
	TArray<uint8> pData;
};

/**
 * 
 */
UCLASS()
class THIRDPERSON_API UNetworkClient : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	bool Init();
	void Tick(float DeltaTime) override;
	bool IsTickable() const override
	{
		return true;
	}

	//이게 뭔지 체크좀 해볼것
	TStatId GetStatId() const
	{
		return TStatId();
	}
	
private:
	bool CreateSocket();
	bool SetServerInfo();
	bool ConnectToServer();
	void SendSocket(const uint16 PacketID, const uint16 DataSize, uint8* pData);
	void RecvSocket();
	void RecvBuffProcess();
	void AddPacketQueue(const uint16 PacketID, const uint16 PacketBodySize, uint8* pDataPos);
	void Clear();
	void DisConnect();
	FString StringFromBinaryArray(const TArray<uint8>& BinaryArray);

private:
	FString			m_Address;
	int32			m_Port;
	FIPv4Address	m_IP;

	FSocket* m_Socket;

	TArray<uint8> m_SendBuffer;
	TArray<uint8> m_RecvBuffer;

	TQueue<FRecvPacketInfo> m_PacketQueue;

	bool m_IsConncted = false;
	int32 m_RecvSize;
};
