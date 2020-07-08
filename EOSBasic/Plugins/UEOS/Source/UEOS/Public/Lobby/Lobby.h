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

	/* The account id of the owner. */
	UPROPERTY(BlueprintReadOnly)
		FEpicAccountId OwnerAccountId;

	/** Lobby id */
	EOS_LobbyId LobbyId;

	
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbySearchAttempt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbySearchFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyJoinSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyCreatedSucceeded);
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
		void DestroyLobby();

	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		void FindLobby(int32 InMaxSearchResults);
	
	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		static void JoinLobby();

	
	void UpdateLobby(EOS_LobbyId OwnerId);

	static void JoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data);

	static void OnSearchResultsReceived(const EOS_LobbySearch_FindCallbackInfo* Data);
	static void OnLobbyUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);

	static void OnInviteToLobbyCallback(const EOS_Lobby_SendInviteCallbackInfo* Data);

	//Sets the lobby id based on the server you have found or started
	EOS_LobbyId CurrentLobbyId;

	//Called upon fetching current lobby details
	EOS_HLobbyDetails CurrentLobbyDetailsHandle;

	//Called upon update function call
	EOS_HLobbyModification LobbyModificationHandle;

	
	//Lobby search result - used to search through all the available sessions
	EOS_HLobbySearch LobbySearchHandle;

	UPROPERTY(BlueprintAssignable)
		FOnLobbySearchAttempt OnLobbySearchAttempt;
	
	UPROPERTY(BlueprintAssignable)
		FOnLobbySearchFailed OnLobbySearchFailed;

	UPROPERTY(BlueprintAssignable)
		FOnLobbySearchSucceeded OnLobbySearchSucceeded;

	UPROPERTY(BlueprintAssignable)
		FOnLobbyJoinSucceeded OnJoinLobbySucceeded;

	UPROPERTY(BlueprintAssignable)
		FOnLobbyCreatedSucceeded OnCreateLobbySucceeded;


	static void OnCreateLobbyCallback(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void OnDestroyLobbyCallback(const _tagEOS_Lobby_DestroyLobbyCallbackInfo* Data);
	

	
};
