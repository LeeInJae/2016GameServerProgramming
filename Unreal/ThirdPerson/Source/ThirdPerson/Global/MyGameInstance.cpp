// Fill out your copyright notice in the Description page of Project Settings.

#include "ThirdPerson.h"
#include "MyGameInstance.h"
#include "Engine/GameInstance.h"
#include "Network/NetworkClient.h"



UMyGameInstance::UMyGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UMyGameInstance::Init()
{
	Super::Init();

	NetworkClient = NewObject<UNetworkClient>();
	if (IsValid(NetworkClient))
	{
		NetworkClient->Init();
	}

	
}