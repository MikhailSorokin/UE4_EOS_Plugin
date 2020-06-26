// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "eos_lobby.h"
#include "Connect/Connect.h"
#include "Lobby.generated.h"

USTRUCT(BlueprintType)
struct FBPLobbySearchResult {

	GENERATED_BODY()
	
	/* The id of the owner. */
	UPROPERTY(BlueprintReadOnly)
		FEpicProductId OwnerId;

	UPROPERTY(BlueprintReadOnly)
		FString OwnerIdString;

	/** Lobby id */
	EOS_LobbyId LobbyId;

	
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbySearchFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbySearchSucceeded, const FBPLobbySearchResult&, SessionInfo);

/**
 * 
 */
UCLASS()
class UEOS_API UEOSLobby : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="UEOS|Lobby")
		void CreateLobby(int32 InLobbyMembers);

	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		void FindLobby(int32 InMaxSearchResults);
	
	static void JoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data);

	static void OnSearchResultsReceived();

	//Sets the lobby id based on the server you have found or started
	EOS_LobbyId CurrentLobbyId;

	//Lobby search result - used to search through all the available sessions
	EOS_HLobbySearch LobbySearchHandle;
	
	UPROPERTY(BlueprintAssignable)
		FOnLobbySearchFailed OnLobbySearchFailed;

	UPROPERTY(BlueprintAssignable)
		FOnLobbySearchSucceeded OnLobbySearchSucceeded;
	
	static void CallBackLobbyTest(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	
};
