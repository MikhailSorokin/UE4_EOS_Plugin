// Copyright (C) Gaslight Games Ltd, 2019-2020

#pragma once

#include "UObject/Object.h"

#include "Authentication/Authentication.h"
#include "eos_sdk.h"
#include "eos_presence.h"
#include "eos_presence_types.h"

#include "Presence.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnFriendPresenceQueryComplete, const FBPCrossPlayInfo&, CompleteFriendInfo);

UCLASS()
class UEOS_API UEOSPresence : public UObject
{
	GENERATED_BODY()

public:

	UEOSPresence();

	/* Queries for ALL the friends. We can then do a subscribe call afterwords to get any updates. */
	UFUNCTION( BlueprintCallable, Category = "UEOS|Presence" )
		void QueryFriendsPresenceInfo();

protected:
	void QueryPresenceInfo(FEpicAccountId LocalAccount, FEpicAccountId FriendAccount);

public:
	/**
	 * Begins an async process that requests the presence statuses of the friends.
	 */
	void SubscribeToFriendPresenceUpdates();
	
	void UnsubscribeFromFriendPresenceUpdates(FEpicAccountId UserId);

	UPROPERTY( BlueprintAssignable , Category = "UEOS|Presence")
		FOnFriendPresenceQueryComplete OnFriendPresenceQueryComplete;

protected:
	// Presence account for each local player that wants to subscribe to updates
	TMap<FEpicAccountId, EOS_NotificationId> PresenceNotifications;

	static FBPPresenceInfo UpdatePresenceStatus(FEpicAccountId LocalId, FEpicAccountId FriendId);

	static void SetPresenceCallback(const EOS_Presence_SetPresenceCallbackInfo* Data);

	static void QueryUserPresenceCompleteCallback(const EOS_Presence_QueryPresenceCallbackInfo* Data);

	static void OnPresenceChangedCallback(const EOS_Presence_PresenceChangedCallbackInfo* Data);

	static void SetPresence(FEpicAccountId TargetUserId);

};

