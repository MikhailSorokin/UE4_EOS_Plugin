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
	EOS_Lobby_CreateLobby(LobbyHandle, &Options, this, CallBackLobbyTest);
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

void UEOSLobby::UpdateLobby(EOS_LobbyId OwnerId)
{
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	
	EOS_Lobby_UpdateLobbyModificationOptions ModifyOptions = {};
	ModifyOptions.ApiVersion = EOS_LOBBY_UPDATELOBBYMODIFICATION_API_LATEST;
	ModifyOptions.LobbyId = OwnerId;
	ModifyOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

	EOS_HLobbyModification LobbyModification = nullptr;
	EOS_EResult Result = EOS_Lobby_UpdateLobbyModification(LobbyHandle, &ModifyOptions, &LobbyModification);

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

	Result = EOS_LobbyModification_AddAttribute(LobbyModification, &AddAttributeModOptions);
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
	UpdateOptions.LobbyModificationHandle = LobbyModification;

	EOS_Lobby_UpdateLobby(LobbyHandle, &UpdateOptions, nullptr, OnLobbyUpdateFinished);
}


/* ===================================== Callbacks ============================ */

void UEOSLobby::CallBackLobbyTest(const EOS_Lobby_CreateLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSManager::GetLobby()->CurrentLobbyId = Data->LobbyId;
		UEOSManager::GetLobby()->UpdateLobby(Data->LobbyId);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
		
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}

//TODO this and create the UI for a joined lobby
void UEOSLobby::JoinLobbyCallback(const EOS_Lobby_JoinLobbyCallbackInfo* Data)
{
	check(Data != nullptr);

	if (Data->ResultCode == EOS_EResult::EOS_Success)
	{
		UEOSManager::GetLobby()->CurrentLobbyId = Data->LobbyId;
		//EOS_LobbyDetails_Info* LobbyInfo;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
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

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, (TEXT("Lobby succeeded: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (TEXT("Lobby failed: %s"), *UEOSCommon::EOSResultToString(Data->ResultCode)));
	}
}


void UEOSLobby::JoinLobby(EOS_HLobbyDetails result)
{
	EOS_HLobbyDetails DetailsHandle = result; //need singleton lobby info?

	EOS_Lobby_JoinLobbyOptions Options = EOS_Lobby_JoinLobbyOptions();
	Options.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
	Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();
	Options.LobbyDetailsHandle = DetailsHandle;
	
	EOS_HLobby LobbyHandle = EOS_Platform_GetLobbyInterface(UEOSManager::GetPlatformHandle());
	EOS_Lobby_JoinLobby(LobbyHandle, &Options, nullptr, JoinLobbyCallback);
}

/*
EOS_HLobbyDetails DetailsHandle;

EOS_LOBBYI

EOS_Lobby_CopyLobbyDetailsHandleOptions CopyHandleOptions = {};
CopyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
CopyHandleOptions.LobbyId = ;
CopyHandleOptions.LocalUserId = UEOSManager::GetConnect()->GetProductId();

EOS_HLobbyDetails LobbyDetailsHandle = nullptr;
Result = EOS_Lobby_CopyLobbyDetailsHandle(LobbyHandle, &CopyHandleOptions, &LobbyDetailsHandle);

EOS_LobbySearch_(UEOSManager::GetLobby()->LobbySearchHandle, &IndexOptions, &NextLobbyDetails);

//Found a lobby, let us join a random one (if possible)
EOS_Lobby_JoinLobbyOptions Options;
Options.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
Options.LobbyDetailsHandle = DetailsHandle;
Options.LocalUserId = UEOSManager::GetConnect()->GetProductId();


EOS_Lobby_JoinLobby(LobbyHandle, &Options, nullptr, JoinLobbyCallback);
*/