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

	UFUNCTION( BlueprintCallable, Category = "UEOS|Presence" )
		void QueryFriendPresence( const FBPCrossPlayInfo& InFriendInfo );

	/**
	 * Begins an async process that requests the presence statuses of the friends.
	 */
	UFUNCTION(BlueprintCallable, Category = "UEOS|Presence")
		void SubscribeToFriendPresenceUpdates();
	
	void UnsubscribeFromFriendPresenceUpdates(FEpicAccountId UserId);

	UPROPERTY( BlueprintAssignable , Category = "UEOS|Presence")
		FOnFriendPresenceQueryComplete OnFriendPresenceQueryComplete;

	TMap<FEpicAccountId, EOS_NotificationId> PresenceNotifications;

protected:

	static FBPCrossPlayInfo UpdatePresenceStatus(FBPCrossPlayInfo& InFriendInfo, FEpicAccountId TargetId);

	static void SetPresenceCallback(const EOS_Presence_SetPresenceCallbackInfo* Data);

	static void QueryPresenceCompleteCallback(const EOS_Presence_QueryPresenceCallbackInfo* Data);

	static void OnPresenceChangedCallback(const EOS_Presence_PresenceChangedCallbackInfo* Data);

	static void SetPresence(FEpicAccountId TargetUserId);

};

