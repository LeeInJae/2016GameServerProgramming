// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"
class UNetworkClient;

/**
 * 
 */
UCLASS()
class THIRDPERSON_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UMyGameInstance(const FObjectInitializer& ObjectInitializer);

	void Init() override;
	
	
private:
	UPROPERTY()
	UNetworkClient* NetworkClient;
};
