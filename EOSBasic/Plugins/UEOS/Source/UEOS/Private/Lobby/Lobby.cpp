// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/Lobby.h"
#include "UEOSModule.h"
#include "UEOSManager.h"
#include "Connect/Connect.h"
#include "Authentication/Authentication.h"

/* ================================= Blueprint function calls =========================== */

/* ============================================= CREATION/DESTROY ==================================== */
//NOTE: Can only be used after EOS_Connect_Login is called
void UEOSLobby::CreateLobby(int32 InLobbyMembers)
{
	if (!UEOSManager::GetConnect()->bAuthorized)
	{
		UE_LOG(UEOSLog, Error, TEXT("%s: You have not connected to your EAS account! Unknown how to authorize."), __FUNCTIONW__);
	}

	UE_LOG(UEOSLog, Log, TEXT("%s: lobby being created."), __FUNCTIONW__);

	bIsServer = true;
	EOS_Lobby_CreateLobbyOptions Options = EOS_Lobby_CreateLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.MaxLobbyMembers = InLobbyMembers;
	Options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED; 
	Options.bPresenceEnabled = false; //TODO - only one lobby at a time can have presence... what does this really mean?
	
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_CreateLobby(LobbyHandle, &Options, this, OnCreateLobbyCallback);

	SubscribeToLobbyUpdates();
}


void UEOSLobby::DestroyLobby()
{
	EOS_Lobby_DestroyLobbyOptions Options = EOS_Lobby_DestroyLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.LobbyId = CurrentLobbyId;

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_DestroyLobby(LobbyHandle, &Options, this, OnDestroyLobbyCallback);

	UnsubscribeFromLobbyInvites();
	UnsubscribeFromLobbyUpdates();
}

/* ============================================= FIND/JOIN ==================================== */
void UEOSLobby::FindLobby(int32 DesiredPlayerNum, int32 InMaxSearchResults)
{
	bIsServer = false;
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_CreateLobbySearchOptions Options = {};
	Options.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	Options.MaxResults = InMaxSearchResults;

	EOS_HLobbySearch OutLobbySearchHandle = nullptr;

	EOS_EResult Result = EOS_Lobby_CreateLobbySearch(LobbyHandle, &Options, &OutLobbySearchHandle);
	if (Result == EOS_EResult::EOS_Success)
	{
		LobbySearchHandle = OutLobbySearchHandle;

		//Set up a custom attribute to being the search query
		EOS_LobbySearch_SetParameterOptions ParamOptions = {};
		ParamOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;

		int32 AttributeAddAttempts = 0;
		EOS_Lobby_AttributeData AttrData;
		EOS_EResult CurrentResult;
		TArray<EOS_EResult> Results = TArray<EOS_EResult>();

		//AddInt64MemberAttribute(FString("GAMEMODE"), DesiredPlayerNum, AttrData);
		ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;
		AttrData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		AttrData.Key = "GAMEMODE";//TCHAR_TO_UTF8("GAMEMODE");
		AttrData.ValueType = EOS_EAttributeType::EOS_AT_INT64;
		AttrData.Value.AsInt64 = (int64)(DesiredPlayerNum);
		ParamOptions.Parameter = &AttrData;
		CurrentResult = EOS_LobbySearch_SetParameter(LobbySearchHandle, &ParamOptions);
		AttributeAddAttempts = (CurrentResult == EOS_EResult::EOS_Success) ? (AttributeAddAttempts + 1) : AttributeAddAttempts;
		Results.Add(CurrentResult);

		//UE_LOG(UEOSLog, Log, TEXT("Value of key after setting is: %s"), UTF8_TO_TCHAR(AttrData.Key));

		//AddBooleanMemberAttribute(FString("SPECTATOR"), true, AttrData);
		/*ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_NOTEQUAL;
		AttrData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		AttrData.Key = "SPECTATOR";
		AttrData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
		AttrData.Value.AsBool = false;
		ParamOptions.Parameter = &AttrData;
		CurrentResult = EOS_LobbySearch_SetParameter(LobbySearchHandle, &ParamOptions);
		Results.Add(CurrentResult);

		AttributeAddAttempts = (CurrentResult == EOS_EResult::EOS_Success) ? (AttributeAddAttempts + 1) : AttributeAddAttempts;*/

		if (AttributeAddAttempts != Results.Num())
		{
			FString ConcactenatedErrorString = "[EOS SDK | Lobby] Lobby search attribute setting failed - Error Codes:";
			for (int32 i = 0; i < Results.Num(); i++) {
				ConcactenatedErrorString += " " + UEOSCommon::EOSResultToString(Results[i]);
			}
			UE_LOG(UEOSLog, Error, TEXT("%s:%s"), __FUNCTIONW__, *ConcactenatedErrorString);
		}
		else
		{

			if (OnLobbySearchAttempt.IsBound()) {
				OnLobbySearchAttempt.Broadcast();
			}

			EOS_LobbySearch_FindOptions FindOptions;
			FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
			FindOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

			EOS_LobbySearch_Find(LobbySearchHandle, &FindOptions, nullptr, OnSearchResultsReceived);
		}
	}
	else
	{
		FString ErrorText = FString::Printf(TEXT("[EOS SDK | Lobby] Lobby search Failed - Error Code: %s"), *UEOSCommon::EOSResultToString(Result));
		UE_LOG(UEOSLog, Error, TEXT("%s:%s"), __FUNCTIONW__, *ErrorText);

		if (OnLobbySearchFailed.IsBound()) {
			OnLobbySearchFailed.Broadcast();
		}
	}

}


void UEOSLobby::JoinLobby()
{
	EOS_HLobbyDetails DetailsHandle = UEOSManager::GetLobby()->ChosenLobbyToJoin; //need singleton lobby info?

	EOS_Lobby_JoinLobbyOptions Options = EOS_Lobby_JoinLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.LobbyDetailsHandle = DetailsHandle;

	SubscribeToLobbyUpdates();

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_JoinLobby(LobbyHandle, &Options, nullptr, JoinLobbyCallback);
}

/* ============================================= UPDATES/SUBSCRIPTIONS ==================================== */
void UEOSLobby::StartLobbyModification()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = CurrentLobbyId;
	ModifyOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

	EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModificationHandle);
	if (Result == EOS_EResult::EOS_Success)
	{
		UE_LOG(UEOSLog, Log, TEXT("Started lobby modification."))
	}
	else
	{
		UE_LOG(UEOSLog, Error, TEXT("Start lobby modification failed; error message: %s"), *UEOSCommon::EOSResultToString(Result))
	}
}


void UEOSLobby::CommitLobbyModification()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_UpdateLobbyOptions UpdateOptions;
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = LobbyModificationHandle;

	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, this, OnLobbyUpdateFinished);
}


void UEOSLobby::RequestLobbyInformation(EOS_LobbyId OwnerId, EOS_LobbyDetails_Info** DetailsInfo)
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_LobbyDetails_CopyInfoOptions LobbyDetailOptions = {
		EOS_LOBBYDETAILS_COPYATTRIBUTEBYKEY_API_LATEST
	};

	EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions = {};
	CopyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
	CopyHandleOptions.LobbyId = OwnerId;
	CopyHandleOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

	EOS_HLobbyDetails CreatedSessionDetails = nullptr;
	EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyHandleOptions, &CreatedSessionDetails);

	EOS_LobbyDetails_CopyInfoOptions CopyInfoOptions = {};
	CopyInfoOptions.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
	EOS_LobbyDetails_CopyInfo(CreatedSessionDetails, &CopyInfoOptions, DetailsInfo);

	EOS_LobbyDetails_Release(CreatedSessionDetails);
}

void UEOSLobby::UpdateLobby(EOS_LobbyId OwnerId)
{
	UE_LOG(UEOSLog, Log, TEXT("%s: OwnerId: %s"), __FUNCTIONW__, *UEOSManager::GetConnect()->GetProductId().ToString());

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = OwnerId;
	ModifyOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

	EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModificationHandle);

	EOS_LobbyDetails_Info* DetailsInfo = nullptr;
	RequestLobbyInformation(OwnerId, &DetailsInfo);

	EOS_Lobby_AttributeData AttributeData;

	UE_LOG(UEOSLog, Log, TEXT("Number of members: %d"), DetailsInfo->MaxMembers);
	//Add searchable attribute based on gamemode type and if spectators can join or not
	AddInt64MemberAttribute(FString("GAMEMODE"), DetailsInfo->MaxMembers, AttributeData);
	//AddBooleanMemberAttribute(FString("ALLOW_SPECTATORS"), false, AttributeData);

	//Release information we needed from copying the function
	EOS_LobbyDetails_Info_Release(DetailsInfo);

	if (Result != EOS_EResult::EOS_Success)
	{
		FString ErrorText = FString::Printf(TEXT("[EOS SDK | Lobby] Lobby attribute update failed - Error Code: %s"), *UEOSCommon::EOSResultToString(Result));
		UE_LOG(UEOSLog, Error, TEXT("%s:%s"), __FUNCTIONW__, *ErrorText);

		if (UEOSManager::GetLobby()->OnLobbySearchFailed.IsBound()) {
			UEOSManager::GetLobby()->OnLobbySearchFailed.Broadcast();
		}
	}

	//Trigger lobby update
	CommitLobbyModification();

}


void UEOSLobby::SubscribeToLobbyUpdates()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_AddNotifyLobbyUpdateReceivedOptions UpdateNotifyOptions = {};
	UpdateNotifyOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYUPDATERECEIVED_API_LATEST;
	LobbyUpdateNotification = EOS_Lobby_AddNotifyLobbyUpdateReceived(LobbyHandle, &UpdateNotifyOptions, nullptr, OnLobbyUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberUpdateReceivedOptions MemberUpdateOptions = {};
	MemberUpdateOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERUPDATERECEIVED_API_LATEST;
	LobbyMemberUpdateNotification = EOS_Lobby_AddNotifyLobbyMemberUpdateReceived(LobbyHandle, &MemberUpdateOptions, nullptr, OnMemberUpdateReceived);

	EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions MemberStatusOptions = {};
	MemberStatusOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
	LobbyMemberStatusNotification = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(LobbyHandle, &MemberStatusOptions, nullptr, OnMemberStatusReceived);
}

void UEOSLobby::UnsubscribeFromLobbyUpdates()
{
	if (LobbyUpdateNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyUpdateReceived(LobbyHandle, LobbyUpdateNotification);
		LobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if (LobbyMemberUpdateNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyMemberUpdateReceived(LobbyHandle, LobbyMemberUpdateNotification);
		LobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
	}

	if (LobbyMemberStatusNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(LobbyHandle, LobbyMemberStatusNotification);
		LobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
	}
}

void UEOSLobby::SubscribeToLobbyInvites()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_AddNotifyLobbyInviteReceivedOptions InviteOptions = {};
	InviteOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYINVITERECEIVED_API_LATEST;
	LobbyInviteNotification = EOS_Lobby_AddNotifyLobbyInviteReceived(LobbyHandle, &InviteOptions, nullptr, OnLobbyInviteReceived);

	EOS_Lobby_AddNotifyLobbyInviteAcceptedOptions AcceptedOptions = {};
	AcceptedOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYINVITEACCEPTED_API_LATEST;
	LobbyInviteAcceptedNotification = EOS_Lobby_AddNotifyLobbyInviteAccepted(LobbyHandle, &AcceptedOptions, nullptr, OnLobbyInviteAccepted);

	EOS_Lobby_AddNotifyJoinLobbyAcceptedOptions JoinGameOptions = {};
	JoinGameOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYJOINLOBBYACCEPTED_API_LATEST;
	JoinLobbyAcceptedNotification = EOS_Lobby_AddNotifyJoinLobbyAccepted(LobbyHandle, &JoinGameOptions, nullptr, OnJoinLobbyAccepted);
}

void UEOSLobby::UnsubscribeFromLobbyInvites()
{
	if (LobbyInviteNotification != EOS_INVALID_NOTIFICATIONID)
	{
		EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

		EOS_Lobby_RemoveNotifyLobbyInviteReceived(LobbyHandle, LobbyInviteNotification);
		LobbyInviteNotification = EOS_INVALID_NOTIFICATIONID;

		EOS_Lobby_RemoveNotifyLobbyInviteAccepted(LobbyHandle, LobbyInviteAcceptedNotification);
		LobbyInviteAcceptedNotification = EOS_INVALID_NOTIFICATIONID;

		EOS_Lobby_RemoveNotifyJoinLobbyAccepted(LobbyHandle, JoinLobbyAcceptedNotification);
		JoinLobbyAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
	}
}

/* ================================== INVITES ============================= */
//TODO - Make this pass in a struct of current lobby information
void UEOSLobby::SendInviteToUser(EOS_LobbyId InLobbyId)
{
	EOS_Lobby_SendInviteOptions InviteOptions = {};
	InviteOptions.LobbyId = InLobbyId;
	InviteOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	InviteOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	InviteOptions.TargetUserId = FEpicProductId::FromString("0002f010460e47bebed6f752ab89ad91");
	
	//send invite
	//EOS_Lobby_SendInvite(LobbyHandle, &InviteOptions, nullptr, OnInviteToLobbyCallback);
}

/* ====================================== SETTING VARIOUS ATTRIBUTE TYPES ========================== */
void UEOSLobby::AddStringMemberAttribute(const FString& Key, const FString& Value, EOS_Lobby_AttributeData& AttributeData)
{
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
	AttributeData.Value.AsUtf8 = TCHAR_TO_UTF8(*Value);
	AttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	AttributeData.Key = TCHAR_TO_UTF8(*Key);

	//NOTE: Only the server can add member attributes like this! Clients just need to set it for their search parameter
	if (bIsServer) {

		EOS_LobbyModification_AddMemberAttributeOptions AddMemberAttributeOptions;
		AddMemberAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
		AddMemberAttributeOptions.Attribute = &AttributeData;
		AddMemberAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC; // TODO: Allow private visibility

		EOS_EResult Result = EOS_LobbyModification_AddMemberAttribute(LobbyModificationHandle, &AddMemberAttributeOptions);

		if (Result == EOS_EResult::EOS_Success)
		{
			UE_LOG(UEOSLog, Log, TEXT("Successfully added member attribute %s"), *Key)
		}
		else
		{
			UE_LOG(UEOSLog, Error, TEXT("Failed to add member attribute %s; Status code: %d; Error message: %s"), *Key, Result, *UEOSCommon::EOSResultToString(Result))
		}
	}
	//AddMemberAttribute(Key, AttributeData);
}

void UEOSLobby::AddBooleanMemberAttribute(const FString& Key, bool Value, EOS_Lobby_AttributeData& AttributeData)
{
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
	AttributeData.Value.AsBool = Value;
	AttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	AttributeData.Key = TCHAR_TO_UTF8(*Key);

	//NOTE: Only the server can add member attributes like this! Clients just need to set it for their search parameter
	if (bIsServer) {

		EOS_LobbyModification_AddMemberAttributeOptions AddMemberAttributeOptions;
		AddMemberAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
		AddMemberAttributeOptions.Attribute = &AttributeData;
		AddMemberAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC; // TODO: Allow private visibility

		EOS_EResult Result = EOS_LobbyModification_AddMemberAttribute(LobbyModificationHandle, &AddMemberAttributeOptions);

		if (Result == EOS_EResult::EOS_Success)
		{
			UE_LOG(UEOSLog, Log, TEXT("Successfully added member attribute %s"), *Key)
		}
		else
		{
			UE_LOG(UEOSLog, Error, TEXT("Failed to add member attribute %s; Status code: %d; Error message: %s"), *Key, Result, *UEOSCommon::EOSResultToString(Result))
		}
	}
	//AddMemberAttribute(Key, AttributeData);
}

void UEOSLobby::AddDoubleMemberAttribute(const FString& Key, float Value, EOS_Lobby_AttributeData& AttributeData)
{
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_DOUBLE;
	AttributeData.Value.AsDouble = (double) Value;
	AttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	AttributeData.Key = TCHAR_TO_UTF8(*Key);

	//NOTE: Only the server can add member attributes like this! Clients just need to set it for their search parameter
	if (bIsServer) {

		EOS_LobbyModification_AddMemberAttributeOptions AddMemberAttributeOptions;
		AddMemberAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
		AddMemberAttributeOptions.Attribute = &AttributeData;
		AddMemberAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC; // TODO: Allow private visibility

		EOS_EResult Result = EOS_LobbyModification_AddMemberAttribute(LobbyModificationHandle, &AddMemberAttributeOptions);

		if (Result == EOS_EResult::EOS_Success)
		{
			UE_LOG(UEOSLog, Log, TEXT("Successfully added member attribute %s"), *Key)
		}
		else
		{
			UE_LOG(UEOSLog, Error, TEXT("Failed to add member attribute %s; Status code: %d; Error message: %s"), *Key, Result, *UEOSCommon::EOSResultToString(Result))
		}
	}
	//AddMemberAttribute(Key, AttributeData);
}

void UEOSLobby::AddInt64MemberAttribute(const FString& Key, int32 Value, EOS_Lobby_AttributeData& AttributeData)
{
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_INT64;
	AttributeData.Value.AsInt64 = (int64)Value;
	AttributeData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	AttributeData.Key = TCHAR_TO_UTF8(*Key);


	//NOTE: Only the server can add member attributes like this! Clients just need to set it for their search parameter
	if (bIsServer) {

		EOS_LobbyModification_AddMemberAttributeOptions AddMemberAttributeOptions;
		AddMemberAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
		AddMemberAttributeOptions.Attribute = &AttributeData;
		AddMemberAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC; // TODO: Allow private visibility

		EOS_EResult Result = EOS_LobbyModification_AddMemberAttribute(LobbyModificationHandle, &AddMemberAttributeOptions);

		if (Result == EOS_EResult::EOS_Success)
		{
			UE_LOG(UEOSLog, Log, TEXT("Successfully added member attribute %s"), *Key)
		}
		else
		{
			UE_LOG(UEOSLog, Error, TEXT("Failed to add member attribute %s; Status code: %d; Error message: %s"), *Key, Result, *UEOSCommon::EOSResultToString(Result))
		}
	}
	//AddMemberAttribute(Key, AttributeData);
}

void UEOSLobby::AddMemberAttribute(const FString& Key, EOS_Lobby_AttributeData& AttributeDataWithValueFilledIn)
{

	//NOTE: Only the server can add member attributes like this! Clients just need to set it for their search parameter
	if (bIsServer) {

		EOS_LobbyModification_AddMemberAttributeOptions AddMemberAttributeOptions;
		AddMemberAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
		AddMemberAttributeOptions.Attribute = &AttributeDataWithValueFilledIn;
		AddMemberAttributeOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC; // TODO: Allow private visibility

		EOS_EResult Result = EOS_LobbyModification_AddMemberAttribute(LobbyModificationHandle, &AddMemberAttributeOptions);

		if (Result == EOS_EResult::EOS_Success)
		{
			UE_LOG(UEOSLog, Log, TEXT("Successfully added member attribute %s"), *Key)
		}
		else
		{
			UE_LOG(UEOSLog, Error, TEXT("Failed to add member attribute %s; Status code: %d; Error message: %s"), *Key, Result, *UEOSCommon::EOSResultToString(Result))
		}
	}
}

/* ===================================== Callbacks ============================ */

void UEOSLobby::OnCreateLobbyCallback(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSLobby* InstigatingLobbyObject = (UEOSLobby *) Data->ClientData;
		InstigatingLobbyObject->CurrentLobbyId = Data->LobbyId;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
		//Lobby is created, add callback to any widget listeners
		if (InstigatingLobbyObject->OnCreateLobbySucceeded.IsBound())
		{
			InstigatingLobbyObject->OnCreateLobbySucceeded.Broadcast();
		}

		//In order for any lobby to be seen, it needs to be updated at least once
		InstigatingLobbyObject->UpdateLobby(Data->LobbyId);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}


void UEOSLobby::OnDestroyLobbyCallback(const _tagEOS_Lobby_DestroyLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Destroy lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Destroy lobby  failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}


void UEOSLobby::OnSearchResultsReceived(const EOS_LobbySearch_FindCallbackInfo* Data)
{
	if (Data->ResultCode == EOS_EResult::EOS_Success) {
		EOS_LobbySearch_GetSearchResultCountOptions SearchResultOptions = {};
		SearchResultOptions.ApiVersion = EOS_LOBBYSEARCH_GETSEARCHRESULTCOUNT_API_LATEST;
		uint32_t NumSearchResults = EOS_LobbySearch_GetSearchResultCount(UEOSManager::GetLobby()->LobbySearchHandle, &SearchResultOptions);

		//Num search results will be 0 if search did not finish
		if (NumSearchResults == 0.f)
		{
			UE_LOG(UEOSLog, Log, TEXT("Could not find any results - search is incomplete."));
			return;
		}

		TArray<FBPLobbySearchResult> SearchResults;
		TArray<EOS_HLobbyDetails> ResultHandles;

		EOS_LobbySearch_CopySearchResultByIndexOptions IndexOptions = {};
		IndexOptions.ApiVersion = EOS_LOBBYSEARCH_COPYSEARCHRESULTBYINDEX_API_LATEST;
		for (uint32_t SearchIndex = 0; SearchIndex < NumSearchResults; ++SearchIndex)
		{
			FBPLobbySearchResult NextLobby;

			EOS_HLobbyDetails NextLobbyDetails = nullptr;
			IndexOptions.LobbyIndex = SearchIndex;
			EOS_EResult Result = EOS_LobbySearch_CopySearchResultByIndex(UEOSManager::GetLobby()->LobbySearchHandle, &IndexOptions, &NextLobbyDetails);
			if (Result == EOS_EResult::EOS_Success && NextLobbyDetails)
			{

				//get owner
				EOS_LobbyDetails_GetLobbyOwnerOptions GetOwnerOptions = {};
				GetOwnerOptions.ApiVersion = EOS_LOBBYDETAILS_GETLOBBYOWNER_API_LATEST;
				FEpicProductId NewLobbyOwner = FEpicProductId(EOS_LobbyDetails_GetLobbyOwner(NextLobbyDetails, &GetOwnerOptions));

				//copy lobby info
				EOS_LobbyDetails_CopyInfoOptions CopyInfoDetails;
				CopyInfoDetails.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;
				EOS_LobbyDetails_Info* LobbyInfo = nullptr;
				Result = EOS_LobbyDetails_CopyInfo(NextLobbyDetails, &CopyInfoDetails, &LobbyInfo);
				if (Result != EOS_EResult::EOS_Success || !LobbyInfo)
				{
					continue;
				}

				NextLobby.LobbyId = LobbyInfo->LobbyId;

				NextLobby.OwnerId = NewLobbyOwner;
				NextLobby.OwnerAccountId = FEpicAccountId();
				
				SearchResults.Add(NextLobby);
				ResultHandles.Add(NextLobbyDetails);
			}
		}

		//Search is done, we can release this memory
		EOS_LobbySearch_Release(UEOSManager::GetLobby()->LobbySearchHandle);

		//Right now we choose to join by a random index
		int32 RandomIndex = FMath::RandRange(0, NumSearchResults - 1);
		if (UEOSManager::GetLobby()->OnLobbySearchSucceeded.IsBound()) {
			//Lobby details handle has to be set first
			UEOSManager::GetLobby()->ChosenLobbyToJoin = ResultHandles[RandomIndex];

			//We have the details handle added in memory, we can remove them
			UEOSManager::GetLobby()->HandlesForRemoval = TArray<EOS_HLobbyDetails>();
			for (EOS_HLobbyDetails ResultHandle : ResultHandles)
			{
				UEOSManager::GetLobby()->HandlesForRemoval.Add(ResultHandle);
			}

			UEOSManager::GetLobby()->OnLobbySearchSucceeded.Broadcast(SearchResults[RandomIndex]);
		}
	}
	else
	{
		FString ErrorText = FString::Printf(TEXT("[EOS SDK | Lobby] Lobby search Failed - Error Code: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));
		UE_LOG(UEOSLog, Error, TEXT("%s:%s"), __FUNCTIONW__, *ErrorText);

		if (UEOSManager::GetLobby()->OnLobbySearchFailed.IsBound()) {
			UEOSManager::GetLobby()->OnLobbySearchFailed.Broadcast();
		}
	}
}

void UEOSLobby::OnLobbyUpdateFinished(const EOS_Lobby_UpdateLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSManager::GetLobby()->CurrentLobbyId = Data->LobbyId;
		//EOS_LobbyDetails_Info* LobbyInfo;
	
		EOS_LobbyModification_Release(UEOSManager::GetLobby()->LobbyModificationHandle);
		

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));

	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}

void UEOSLobby::OnInviteToLobbyCallback(const EOS_Lobby_SendInviteCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UE_LOG(UEOSLog, Error, TEXT("%s:Invite was successful, %s"), __FUNCTIONW__, *UEOSCommon::EOSResultToString(Data->ResultCode));

	} else
	{
		UE_LOG(UEOSLog, Error, TEXT("%s:Invite was a terrible terrible failure, %s"), __FUNCTIONW__,  *UEOSCommon::EOSResultToString(Data->ResultCode));
	}
}

void UEOSLobby::JoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
	UE_LOG(UEOSLog, Log, TEXT("%s: got inside"), __FUNCTIONW__);

	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSManager::GetLobby()->CurrentLobbyId = Data->LobbyId;
		//EOS_LobbyDetails_Info* LobbyInfo;

		UE_LOG(UEOSLog, Log, TEXT("%s: lobby join succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));
		if (UEOSManager::GetLobby()->OnJoinLobbySucceeded.IsBound())
		{
			UEOSManager::GetLobby()->OnJoinLobbySucceeded.Broadcast();
		}

		//Remove all lobby details after done with the results
		for (EOS_HLobbyDetails ResultHandle : UEOSManager::GetLobby()->HandlesForRemoval)
		{
			EOS_LobbyDetails_Release(ResultHandle);
		}

		UEOSManager::GetLobby()->HandlesForRemoval.Empty();
	}
	else
	{
		UE_LOG(UEOSLog, Log, TEXT("%s: lobby join failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode));
	}
}

/* =================================== Subscription to lobby events ========================== */
void UEOSLobby::OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* Data)
{
	UE_LOG(UEOSLog, Log, TEXT("%s: successfully added member attribute %s"), *FString(__FUNCTION__), Data->LobbyId);

	//TODO - ADd update here
	//FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);
}

void UEOSLobby::OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* Data)
{
	UE_LOG(UEOSLog, Log, TEXT("%s: successfully added member %s"), *FString(__FUNCTION__), *FEpicProductId(Data->TargetUserId).ToString());

	//TODO - ADd update here
	//FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);
}

void UEOSLobby::OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* Data)
{
	UE_LOG(UEOSLog, Log, TEXT("%s: successfully added member %s"), *FString(__FUNCTION__), *FEpicProductId(Data->TargetUserId).ToString());

	//TODO - ADd update here
	//FGame::Get().GetLobbies()->OnLobbyUpdate(Data->LobbyId);
}

void UEOSLobby::OnLobbyInviteReceived(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* Data)
{

}

void UEOSLobby::OnLobbyInviteAccepted(const EOS_Lobby_LobbyInviteAcceptedCallbackInfo* Data)
{

}

void UEOSLobby::OnJoinLobbyAccepted(const EOS_Lobby_JoinLobbyAcceptedCallbackInfo* Data)
{

}
