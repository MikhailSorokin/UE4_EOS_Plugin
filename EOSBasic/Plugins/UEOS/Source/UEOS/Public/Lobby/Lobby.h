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

USTRUCT(BlueprintType)
struct FBPAttribute
{
	GENERATED_BODY()
	/**
	* Construct wrapper from product id.
	*/
	FBPAttribute(const EOS_Lobby_AttributeData& InAttributeData)
		: AttributeData(InAttributeData)
	{
	};

	FBPAttribute() = default;

	FBPAttribute(const FBPAttribute&) = default;

	FBPAttribute& operator=(const FBPAttribute&) = default;

	/*bool operator==(const FBPAttribute& Other) const
	{
		return AttributeData == Other.AttributeData;
	}

	bool operator!=(const FBPAttribute& Other) const
	{
		return !(this->operator==(Other));
	}*/
	/**
	* Easy conversion to EOS attribute ID.
	*/
	operator EOS_Lobby_AttributeData() const
	{
		return AttributeData;
	}

	/** The EOS SDK matching Product Id. */
	EOS_Lobby_AttributeData			AttributeData;
	
	//TODO - Pass in types here
	
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

protected:
	//Enums that can be used to get all the updates needed from the lobby
	EOS_NotificationId LobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyInviteNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId LobbyInviteAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
	EOS_NotificationId JoinLobbyAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
	bool bIsServer;

public:

	UFUNCTION(BlueprintCallable, Category="UEOS|Lobby")
		void CreateLobby(int32 InLobbyMembers);

	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		void DestroyLobby();

	/**
	 * Takes in a desired player number and maximum search results to have visible.
	 *
	 * @param DesiredPlayerNum the filter used for determining the gamemode
	 * @param InMaxSearchResults how many search results we should retrieve in one search
	 */
	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		void FindLobby(int32 DesiredPlayerNum, int32 InMaxSearchResults);
	
	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		void JoinLobby();

	/* Acquires a EOS_HLobbyModification to start a lobby modification transaction. */
	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		void StartLobbyModification();
	/* Used to commit lobby information to have the rest of the lobby get updates. */
	UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		void CommitLobbyModification();

	//Subscription to events that can be used for when a party wants to have lobby travel together
	UFUNCTION(BlueprintCallable)
		void SubscribeToLobbyInvites();
	UFUNCTION(BlueprintCallable)
		void UnsubscribeFromLobbyInvites();
	
	static void OnLobbyInviteReceived(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* Data);
	static void OnLobbyInviteAccepted(const EOS_Lobby_LobbyInviteAcceptedCallbackInfo* Data);
	static void OnJoinLobbyAccepted(const EOS_Lobby_JoinLobbyAcceptedCallbackInfo* Data);

	//Subscription to events that can be called through joining/finding a lobby
	void SubscribeToLobbyUpdates();
	void UnsubscribeFromLobbyUpdates();
	
	static void OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data);
	static void OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* Data);
	static void OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data);

	void SendInviteToUser(EOS_LobbyId InLobbyId);
	void RequestLobbyInformation(EOS_LobbyId OwnerId,
	                             EOS_LobbyDetails_Info** DetailsInfo);

	void AddStringMemberAttribute(const FString& Key, const FString& Value, EOS_Lobby_AttributeData& AttributeData);
	void AddBooleanMemberAttribute(const FString& Key, bool Value, EOS_Lobby_AttributeData& AttributeData);
	/* Note that 'double' is not legal in Blueprint. The float Value will be casted to a double.*/
	void AddDoubleMemberAttribute(const FString& Key, float Value, EOS_Lobby_AttributeData& AttributeData);
	/* Note that 'int64' is not legal in Blueprints. The int32 Value will be casted to an int64. */
	void AddInt64MemberAttribute(const FString& Key, int32 Value, EOS_Lobby_AttributeData& AttributeData);

	//TODO - Wrap as a variant type to get it to work as BlueprintCallable
	//UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		//void AddStringMemberAttribute(const FString& Key, const FString& Value, const FBPAttribute& AttributeData);
	//UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		//void AddBooleanMemberAttribute(const FString& Key, bool Value, const FBPAttribute& AttributeData);
	//UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		/* Note that 'double' is not legal in Blueprint. The float Value will be casted to a double.*/
		//void AddDoubleMemberAttribute(const FString& Key, float Value, const FBPAttribute& AttributeData);
	//UFUNCTION(BlueprintCallable, Category = "UEOS|Lobby")
		/* Note that 'int64' is not legal in Blueprints. The int32 Value will be casted to an int64. */
		//void AddInt64MemberAttribute(const FString& Key, int32 Value, const FBPAttribute& AttributeData);

protected:
	void UpdateLobby(EOS_LobbyId OwnerId);

	static void JoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data);

	static void OnSearchResultsReceived(const EOS_LobbySearch_FindCallbackInfo* Data);
	static void OnLobbyUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* Data);

	static void OnInviteToLobbyCallback(const EOS_Lobby_SendInviteCallbackInfo* Data);

	//Sets the lobby id based on the server you have found or started
	EOS_LobbyId CurrentLobbyId;

	//Called upon fetching current lobby details
	EOS_HLobbyDetails ChosenLobbyToJoin;

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
	
	TArray<EOS_HLobbyDetails> HandlesForRemoval;


	static void OnCreateLobbyCallback(const EOS_Lobby_CreateLobbyCallbackInfo* Data);
	static void OnDestroyLobbyCallback(const _tagEOS_Lobby_DestroyLobbyCallbackInfo* Data);

private:
	void AddMemberAttribute(const FString& Key, EOS_Lobby_AttributeData& AttributeDataWithValueFilledIn);
};
