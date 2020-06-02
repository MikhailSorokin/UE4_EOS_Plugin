// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "eos_lobby.h"
#include "Lobby.generated.h"

/**
 * 
 */
UCLASS()
class UEOS_API UEOSLobby : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
		void CreateLobby(int32 InLobbyMembers);

	static void CallBackLobbyTest(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	
};
