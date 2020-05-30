// Copyright (C) Gaslight Games Ltd, 2019-2020

#pragma once

// UEOS Includes
#include "Friends/Friends.h"

#include "UEOSModule.h"
#include "UEOSManager.h"
#include "UserInfo/UserInfo.h"
#include "Presence/Presence.h"

UEOSFriends::UEOSFriends()
{

}

void UEOSFriends::SubscribeToFriendUpdates()
{
	FEpicAccountId AccountId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();
	UnsubscribeFromFriendUpdates(AccountId);

	//Subscribe for friend status updates
	EOS_Friends_AddNotifyFriendsUpdateOptions Options;
	Options.ApiVersion = EOS_FRIENDS_ADDNOTIFYFRIENDSUPDATE_API_LATEST;

	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());

	//We add the friend status in a map for every local user that we have
	EOS_NotificationId NotificationId = EOS_Friends_AddNotifyFriendsUpdate(FriendsHandle, &Options, NULL, OnFriendsUpdateCallback);
	if (!NotificationId)
	{
		UE_LOG(UEOSLog, Error, TEXT("[EOS SDK]: could not subscribe to friend updates."));
	}
	else
	{
		FriendNotifications.Add(AccountId, NotificationId);
	}

	UEOSManager::GetEOSManager()->GetPresence()->SubscribeToFriendPresenceUpdates();
}


void UEOSFriends::OnFriendsUpdateCallback(const EOS_Friends_OnFriendsUpdateInfo* Data)
{
	// NOTE: SendInvite, AcceptInvite, RejectInvite, AddFriend and RemoveFriend will force updates here
	UEOSManager::GetFriends()->FriendStatusChanged(Data->LocalUserId, Data->TargetUserId, Data->CurrentStatus);
}

void UEOSFriends::FriendStatusChanged(FEpicAccountId LocalUserId, FEpicAccountId TargetUserId, EOS_EFriendsStatus NewStatus)
{
	for (FBPCrossPlayInfo& CrossPlayFriend : CrossPlayFriends)
	{
		if (CrossPlayFriend.AccountId == TargetUserId)
		{
			//TODO - See if this actually transfers properly
			EBPFriendStatus BPNewFriendStatus = (EBPFriendStatus)NewStatus;
			if (CrossPlayFriend.FriendshipStatus != BPNewFriendStatus)
			{
				CrossPlayFriend.FriendshipStatus = BPNewFriendStatus;
			}
			return;
		}
	}

	//Either no such friend, or need to query a new friends list to refresh list (must be new friend?) unless initial friend query is not finished yet.
	QueryFriends(LocalUserId);
}
void UEOSFriends::UnsubscribeFromFriendUpdates(FEpicAccountId UserId)
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());

	if (FriendNotifications.Contains(UserId)) {
		EOS_Friends_RemoveNotifyFriendsUpdate(FriendsHandle, FriendNotifications[UserId]);
		FriendNotifications.Remove(UserId);
	}
	UEOSManager::GetPresence()->UnsubscribeFromFriendPresenceUpdates(UserId);
}

void UEOSFriends::QueryFriends()
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_QueryFriendsOptions Options;
	Options.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
	Options.LocalUserId = UEOSManager::GetAuthentication()->GetEpicAccountId();
	EOS_Friends_QueryFriends(FriendsHandle, &Options, nullptr, QueryFriendsCallback);

}

void UEOSFriends::QueryFriends(FEpicAccountId InLocalUserId)
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_QueryFriendsOptions Options;
	Options.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
	Options.LocalUserId = InLocalUserId;
	EOS_Friends_QueryFriends( FriendsHandle, &Options, nullptr, QueryFriendsCallback );

}

void UEOSFriends::QueryFriendsCallback( const EOS_Friends_QueryFriendsCallbackInfo* Data )
{
	if (Data != nullptr) {
		UEOSFriends* EOSFriends = UEOSManager::GetFriends();
		if (EOSFriends != nullptr)
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				int32_t FriendCount = UEOSManager::GetFriends()->GetFriendsCount();

				EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());

				TArray<FBPCrossPlayInfo> NewFriends;
				
				EOS_Friends_GetFriendAtIndexOptions IndexOptions;
				IndexOptions.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
				IndexOptions.LocalUserId = Data->LocalUserId;

				for (int32_t FriendIdx = 0; FriendIdx < FriendCount; ++FriendIdx)
				{
					IndexOptions.Index = FriendIdx;

					EOS_EpicAccountId FriendUserId = EOS_Friends_GetFriendAtIndex(FriendsHandle, &IndexOptions);

					if (EOS_EpicAccountId_IsValid(FriendUserId))
					{
						EOS_Friends_GetStatusOptions StatusOptions;
						StatusOptions.ApiVersion = EOS_FRIENDS_GETSTATUS_API_LATEST;
						StatusOptions.LocalUserId = Data->LocalUserId;
						StatusOptions.TargetUserId = FriendUserId;

						EOS_EFriendsStatus FriendStatus = EOS_Friends_GetStatus(FriendsHandle, &StatusOptions);

						UE_LOG(UEOSLog, Log, TEXT("[EOS SDK] FriendStatus: %s"), *FEpicAccountId(FriendUserId).ToString());

						FBPCrossPlayInfo FriendDataEntry;
						FriendDataEntry.AccountId = FriendUserId;
						//NOTE: We only get the actual info from the account IDs
						FriendDataEntry.DisplayName = "Pending...";
						UEOSManager::GetUserInfo()->QueryUserInfoByAccountId(FriendUserId);
						FriendDataEntry.FriendshipStatus = (EBPFriendStatus)FriendStatus;

						NewFriends.Add(FriendDataEntry);
					} else
					{
						UE_LOG(UEOSLog, Error, TEXT("%s [EOS SDK | Plugin] Friend ID on iteration: %d was invalid!"), __FUNCTIONW__, FriendIdx);
					}
				}

				UEOSManager::GetFriends()->CrossPlayFriends = NewFriends;
				//TODO - Figure out what account friends are connected to and do query mappings
			}
			else
			{
				FString CallbackError = FString::Printf(TEXT("[EOS SDK | Plugin] Error %s when querying friends: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));
				//NOTE: Leave it to blueprint to log the warning
				if (EOSFriends->OnFriendActionError.IsBound()) {
					EOSFriends->OnFriendActionError.Broadcast(CallbackError);
				}
			}
		}
	} else {
		UE_LOG(UEOSLog, Error, TEXT("%s [EOS SDK | Plugin] data is invalid!"), __FUNCTIONW__);
	}
}

int32 UEOSFriends::GetFriendsCount()
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_GetFriendsCountOptions Options;
	Options.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
	Options.LocalUserId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();

	int32_t FriendCount = EOS_Friends_GetFriendsCount(FriendsHandle, &Options);
	UE_LOG(UEOSLog, Log, TEXT("[EOS SDK | Plugin] Number of Friends is: %d"), FriendCount);
	
	return FriendCount;
}

void UEOSFriends::RequestFriendId( int32 Index )
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_GetFriendAtIndexOptions Options;
	Options.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
	Options.LocalUserId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();
	Options.Index = Index;
	EOS_EpicAccountId EpicAccountId = EOS_Friends_GetFriendAtIndex( FriendsHandle, &Options );

	UEOSUserInfo* UserInfo = UEOSManager::GetEOSManager()->GetUserInfo();
	UserInfo->QueryUserInfoByAccountId(EpicAccountId);
}

/* =================== FRIEND MESSAGING FUNCTIONS ============================= */
void UEOSFriends::SendInvite(const FEpicAccountId& FriendInfo)
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_SendInviteOptions Options;
	Options.ApiVersion = EOS_FRIENDS_SENDINVITE_API_LATEST;
	Options.LocalUserId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();

	FEpicAccountId BlankId;
	FEpicAccountId FriendId = BlankId.FromString(FriendInfo.ToString());
	Options.TargetUserId = FriendId;
	EOS_Friends_SendInvite(FriendsHandle, &Options, NULL, SendInviteCallback);
}

void UEOSFriends::SendInviteCallback(const EOS_Friends_SendInviteCallbackInfo* Data)
{
	if (Data != nullptr) {

		UEOSFriends* EOSFriends = UEOSManager::GetFriends();
		if (EOSFriends != nullptr)
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				if (EOSFriends->OnFriendInviteSent.IsBound()) {
					EOSFriends->OnFriendInviteSent.Broadcast();
				}
			}
			else
			{
				FString DisplayName = UEOSManager::GetEOSManager()->GetUserInfo()->GetDisplayName(Data->TargetUserId);
				FString CallbackError = FString::Printf(TEXT("[EOS SDK | Plugin] Error %s when sending invite to %s"), *UEOSCommon::EOSResultToString(Data->ResultCode), * DisplayName);
				//NOTE: Leave it to blueprint to log the warning
				if (EOSFriends->OnFriendActionError.IsBound()) {
					EOSFriends->OnFriendActionError.Broadcast(CallbackError);
				}
			}
		}
	} else {
		UE_LOG(UEOSLog, Error, TEXT("%s [EOS SDK | Plugin] data is invalid!"), __FUNCTIONW__);
	}
}

void UEOSFriends::AcceptInvite(const FEpicAccountId& FriendInfo)
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_AcceptInviteOptions Options;
	Options.ApiVersion = EOS_FRIENDS_ACCEPTINVITE_API_LATEST;
	Options.LocalUserId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();
	
	FEpicAccountId BlankId;
	FEpicAccountId FriendId = BlankId.FromString(FriendInfo.ToString());
	Options.TargetUserId = FriendId;
	EOS_Friends_AcceptInvite(FriendsHandle, &Options, NULL, AcceptInviteCallback);
}

void UEOSFriends::AcceptInviteCallback(const EOS_Friends_AcceptInviteCallbackInfo* Data)
{
	if (Data != nullptr) {

		UEOSFriends* EOSFriends = UEOSManager::GetFriends();
		if (EOSFriends != nullptr)
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				EOSFriends->OnFriendInviteAccepted.Broadcast();
			}
			else
			{
				FString DisplayName = UEOSManager::GetEOSManager()->GetUserInfo()->GetDisplayName(Data->TargetUserId);
				FString CallbackError = FString::Printf(TEXT("[EOS SDK | Plugin] Error %s when accepting invite from friend: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode), *DisplayName);
				//NOTE: Leave it to blueprint to log the warning
				if (EOSFriends->OnFriendActionError.IsBound()) {
					EOSFriends->OnFriendActionError.Broadcast(CallbackError);
				}
			}
		}
	} else {
		UE_LOG(UEOSLog, Error, TEXT("%s [EOS SDK | Plugin] data is invalid!"), __FUNCTIONW__);
	}
}

void UEOSFriends::RejectInvite(const FEpicAccountId& FriendInfo)
{
	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_RejectInviteOptions Options;
	Options.ApiVersion = EOS_FRIENDS_REJECTINVITE_API_LATEST;
	Options.LocalUserId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();
	FEpicAccountId BlankId;
	FEpicAccountId FriendId = BlankId.FromString(FriendInfo.ToString());
	Options.TargetUserId = FriendId;
	EOS_Friends_RejectInvite(FriendsHandle, &Options, NULL, RejectInviteCallback);
}

void UEOSFriends::RejectInviteCallback(const EOS_Friends_RejectInviteCallbackInfo* Data)
{
	if (Data != nullptr) {
		UEOSFriends* EOSFriends = UEOSManager::GetFriends();
		if (EOSFriends != nullptr)
		{
			if (Data->ResultCode == EOS_EResult::EOS_Success)
			{
				EOSFriends->OnFriendInviteRejected.Broadcast();
			}
			else
			{
				FString DisplayName = UEOSManager::GetEOSManager()->GetUserInfo()->GetDisplayName(Data->TargetUserId);
				FString CallbackError = FString::Printf(TEXT("[EOS SDK | Plugin] Error %s when rejecting invite from friend: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode), *DisplayName);
				//NOTE: Leave it to blueprint to log the warning
				if (EOSFriends->OnFriendActionError.IsBound()) {
					EOSFriends->OnFriendActionError.Broadcast(CallbackError);
				}
			}
		}
	} else {
		 UE_LOG(UEOSLog, Error, TEXT("%s [EOS SDK | Plugin] data is invalid!"), __FUNCTIONW__);
	}
}
/* =================== END FRIEND MESSAGING FUNCTIONS ============================= */

EFriendStatus UEOSFriends::GetStatus( const FEpicAccountId& FriendInfo)
{

	EOS_HFriends FriendsHandle = EOS_Platform_GetFriendsInterface(UEOSManager::GetPlatformHandle());
	EOS_Friends_GetStatusOptions StatusOptions;
	StatusOptions.ApiVersion = EOS_FRIENDS_GETSTATUS_API_LATEST;
	StatusOptions.LocalUserId = UEOSManager::GetEOSManager()->GetAuthentication()->GetEpicAccountId();
	FEpicAccountId BlankId;
	FEpicAccountId FriendId = BlankId.FromString(FriendInfo.ToString());
	StatusOptions.TargetUserId = FriendId;
	EOS_EFriendsStatus Status = EOS_Friends_GetStatus( FriendsHandle, &StatusOptions );
	return (EFriendStatus)Status;
}


