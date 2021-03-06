// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/Lobby.h"
#include "UEOSModule.h"
#include "UEOSManager.h"
#include "Connect/Connect.h"
#include "Authentication/Authentication.h"

/* ================================= Blueprint function calls =========================== */

//NOTE: Can only be used if Connect interface is called after authentication login
void UEOSLobby::CreateLobby(int32 InLobbyMembers)
{
	if (!UEOSManager::GetConnect()->bAuthorized)
	{
		UE_LOG(UEOSLog, Error, TEXT("%s: You have not connected to your EAS account! Unknown how to authorize."), __FUNCTIONW__);
	}

	UE_LOG(UEOSLog, Log, TEXT("%s: lobby being created."), __FUNCTIONW__);

	EOS_Lobby_CreateLobbyOptions Options = EOS_Lobby_CreateLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.MaxLobbyMembers = InLobbyMembers;
	Options.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_PUBLICADVERTISED;

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_CreateLobby(LobbyHandle, &Options, this, OnCreateLobbyCallback);

	
}


void UEOSLobby::DestroyLobby()
{
	EOS_Lobby_DestroyLobbyOptions Options = EOS_Lobby_DestroyLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.LobbyId = CurrentLobbyId;

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_DestroyLobby(LobbyHandle, &Options, this, OnDestroyLobbyCallback);
}


void UEOSLobby::FindLobby(int32 InMaxSearchResults)
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_CreateLobbySearchOptions Options = EOS_Lobby_CreateLobbySearchOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	Options.MaxResults = InMaxSearchResults;

	EOS_HLobbySearch OutLobbySearchHandle = nullptr;

	EOS_EResult Result = EOS_Lobby_CreateLobbySearch(LobbyHandle, &Options, &OutLobbySearchHandle);
	if (Result == EOS_EResult::EOS_Success)
	{
		LobbySearchHandle = OutLobbySearchHandle;

		//Set up a custom attribute to being the search query
		/*FLobbyAttribute LevelAttribute;
		LevelAttribute.Key = "LEVEL";
		LevelAttribute.ValueType = FLobbyAttribute::String;
		LevelAttribute.AsString = SearchLevelName;
		LevelAttribute.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

		Attributes.push_back(LevelAttribute);*/


		EOS_LobbySearch_SetParameterOptions ParamOptions = {};
		ParamOptions.ApiVersion = EOS_LOBBYSEARCH_SETPARAMETER_API_LATEST;
		ParamOptions.ComparisonOp = EOS_EComparisonOp::EOS_CO_EQUAL;

		EOS_Lobby_AttributeData AttrData;
		AttrData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
		AttrData.Key = "RANK";
		AttrData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
		std::string MyRank = "Corporal";
		AttrData.Value.AsUtf8 = MyRank.c_str();

		ParamOptions.Parameter = &AttrData;

		Result = EOS_LobbySearch_SetParameter(LobbySearchHandle, &ParamOptions);
		if (EOS_EResult::EOS_Success == Result) {

			EOS_LobbySearch_FindOptions FindOptions;
			FindOptions.ApiVersion = EOS_LOBBYSEARCH_FIND_API_LATEST;
			FindOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

			if (OnLobbySearchAttempt.IsBound()) {
				OnLobbySearchAttempt.Broadcast();
			}

			EOS_LobbySearch_Find(LobbySearchHandle, &FindOptions, nullptr, OnSearchResultsReceived);
		} else
		{
			FString ErrorText = FString::Printf(TEXT("[EOS SDK | Lobby] Lobby search attribute setting failed - Error Code: %s"), *UEOSCommon::EOSResultToString(Result));
			UE_LOG(UEOSLog, Error, TEXT("%s:%s"), __FUNCTIONW__, *ErrorText);
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

void UEOSLobby::UpdateLobby(EOS_LobbyId OwnerId)
{
	UE_LOG(UEOSLog, Error, TEXT("%s:OwnerId: %s"), __FUNCTIONW__, *UEOSManager::GetConnect()->GetProductId().ToString());

	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	
	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = OwnerId;
	ModifyOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

	EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModificationHandle);
	
	//TODO - Hardcode attribute for now
	EOS_LobbyModification_AddAttributeOptions AddAttributeModOptions = {};
	AddAttributeModOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDATTRIBUTE_API_LATEST;

	EOS_Lobby_AttributeData AttrData;
	AttrData.ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	AttrData.Key = "RANK";
	AttrData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
	std::string MyRank = "Corporal";
	AttrData.Value.AsUtf8 = MyRank.c_str();
	
	AddAttributeModOptions.Attribute = &AttrData;
	AddAttributeModOptions.Visibility = EOS_ELobbyAttributeVisibility::EOS_LAT_PUBLIC;

	Result = EOS_LobbyModification_AddAttribute(LobbyModificationHandle, &AddAttributeModOptions);
	if (Result != EOS_EResult::EOS_Success)
	{
		FString ErrorText = FString::Printf(TEXT("[EOS SDK | Lobby] Lobby attribute update failed - Error Code: %s"), *UEOSCommon::EOSResultToString(Result));
		UE_LOG(UEOSLog, Error, TEXT("%s:%s"), __FUNCTIONW__, *ErrorText);

		if (UEOSManager::GetLobby()->OnLobbySearchFailed.IsBound()) {
			UEOSManager::GetLobby()->OnLobbySearchFailed.Broadcast();
		}
	}

	//Trigger lobby update
	EOS_Lobby_UpdateLobbyOptions UpdateOptions = {};
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = LobbyModificationHandle;

	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyUpdateFinished);

	EOS_Lobby_SendInviteOptions InviteOptions = {};
	InviteOptions.LobbyId = CurrentLobbyId;
	InviteOptions.ApiVersion = EOS_LOBBY_CREATELOBBYSEARCH_API_LATEST;
	InviteOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	InviteOptions.TargetUserId = FEpicProductId::FromString("0002f010460e47bebed6f752ab89ad91");
	
	//send invite
	EOS_Lobby_SendInvite(LobbyHandle, &InviteOptions, nullptr, OnInviteToLobbyCallback);
}

void UEOSLobby::AddStringMemberAttribute(const FString& Key, const FString& Value)
{
	EOS_Lobby_AttributeData AttributeData;
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_STRING;
	AttributeData.Value.AsUtf8 = TCHAR_TO_UTF8(*Value);

	AddMemberAttribute(Key, &AttributeData);
}

void UEOSLobby::AddBooleanMemberAttribute(const FString& Key, bool Value)
{
	EOS_Lobby_AttributeData AttributeData;
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_BOOLEAN;
	AttributeData.Value.AsBool = Value;

	AddMemberAttribute(Key, &AttributeData);
}

void UEOSLobby::AddDoubleMemberAttribute(const FString& Key, float Value)
{
	EOS_Lobby_AttributeData AttributeData;
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_DOUBLE;
	AttributeData.Value.AsDouble = (double) Value;

	AddMemberAttribute(Key, &AttributeData);
}

void UEOSLobby::AddInt64MemberAttribute(const FString& Key, int32 Value)
{
	EOS_Lobby_AttributeData AttributeData;
	AttributeData.ValueType = EOS_EAttributeType::EOS_AT_INT64;
	AttributeData.Value.AsInt64 = (int64) Value;

	AddMemberAttribute(Key, &AttributeData);
}

void UEOSLobby::AddMemberAttribute(const FString& Key, EOS_Lobby_AttributeData* AttributeDataWithValueFilledIn)
{	
	AttributeDataWithValueFilledIn->ApiVersion = EOS_LOBBY_ATTRIBUTEDATA_API_LATEST;
	AttributeDataWithValueFilledIn->Key = TCHAR_TO_UTF8(*Key);
	
	EOS_LobbyModification_AddMemberAttributeOptions AddMemberAttributeOptions;
	AddMemberAttributeOptions.ApiVersion = EOS_LOBBYMODIFICATION_ADDMEMBERATTRIBUTE_API_LATEST;
	AddMemberAttributeOptions.Attribute = AttributeDataWithValueFilledIn;
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

void UEOSLobby::CommitLobbyModification()
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());

	EOS_Lobby_UpdateLobbyOptions UpdateOptions;
	UpdateOptions.ApiVersion = EOS_LOBBY_UPDATELOBBY_API_LATEST;
	UpdateOptions.LobbyModificationHandle = LobbyModificationHandle;
	
	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, this, OnLobbyUpdateFinished);
}


/* ===================================== Callbacks ============================ */

void UEOSLobby::OnCreateLobbyCallback(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSLobby* InstigatingLobbyObject = (UEOSLobby *) Data->ClientData;
		InstigatingLobbyObject->CurrentLobbyId = Data->LobbyId;

		//UEOSManager::GetLobby()->UpdateLobby(Data->LobbyId);
				
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
		//lobby is created
		if (InstigatingLobbyObject->OnCreateLobbySucceeded.IsBound())
		{
			InstigatingLobbyObject->OnCreateLobbySucceeded.Broadcast();
		}
		
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
			UEOSManager::GetLobby()->CurrentLobbyDetailsHandle = ResultHandles[RandomIndex];

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


void UEOSLobby::JoinLobby()
{
	EOS_HLobbyDetails DetailsHandle = UEOSManager::GetLobby()->CurrentLobbyDetailsHandle; //need singleton lobby info?

	EOS_Lobby_JoinLobbyOptions Options = EOS_Lobby_JoinLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.LobbyDetailsHandle = DetailsHandle;
	
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_JoinLobby(LobbyHandle, &Options, nullptr, JoinLobbyCallback);
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