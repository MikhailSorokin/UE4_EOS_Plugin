
// Copyright (C) Gaslight Games Ltd, 2019

#pragma once

// Engine Includes
#include "UObject/Object.h"

#include "OnlineEncryptedAppTicketInterfaceSteam.h"

// EOS Includes
#include "eos_sdk.h"
#include "eos_connect.h"
#include "eos_version.h"
#include "eos_common.h"
#include "UObject/CoreOnline.h"
#include "OnlineAuthInterfaceUtilsSteam.h"
#include "UEOSOnlineTypes.h"


#include "Connect.generated.h"

//TODO: The ContinuanceToken should be added as a parameter to this delegate; however, it needs an "Unreal-friendly" type to hold it.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnectLoginSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConnectLoginFail);

/* All credentials available to use with the connect interface.
 * TODO - Test all other credential types like Playstation, OpenID, etc.
 */
UENUM(BlueprintType)
enum class EExternalCredentialType : uint8
{
	/* Epic Games Account */
	ECT_Epic = 0				UMETA(DisplayName = "Epic"),

	/* Steam Encrypted App Ticket*/
	ECT_Steam_App_Ticket = 1	UMETA(DisplayName = "Steam"),

	ECT_Discord = 2				UMETA(DisplayName = "Discord")
};


/**
* Adapted from the sample, to work within UE4 UBT.
*/
USTRUCT(BlueprintType)
struct UEOS_API FEpicProductId
{
	GENERATED_BODY()

	/**
	* Construct wrapper from product id.
	*/
	FEpicProductId(EOS_ProductUserId InProductId)
	: ProductId(InProductId)
	{
	};

	FEpicProductId() = default;

	FEpicProductId(const FEpicProductId&) = default;

	FEpicProductId& operator=(const FEpicProductId&) = default;

	bool operator==(const FEpicProductId& Other) const
	{
		return ProductId == Other.ProductId;
	}

	bool operator!=(const FEpicProductId& Other) const
	{
		return !(this->operator==(Other));
	}

	/**
	* Checks if product ID is valid.
	*/
	operator bool() const
	{
		return (EOS_ProductUserId_IsValid(ProductId) == EOS_TRUE) ? true : false;
	}

	/**
	* Easy conversion to EOS product ID.
	*/
	operator EOS_ProductUserId() const
	{
		return ProductId;
	}

	void SetCredentialToken(EOS_ContinuanceToken InCredentialToken)
	{
		ContinuanceToken = InCredentialToken;
	}

	/**
	* Prints out account ID as hex.
	*/
	FString						ToString() const
	{
		static char TempBuffer[EOS_EPICACCOUNTID_MAX_LENGTH];
		int32_t TempBufferSize = sizeof(TempBuffer);
		EOS_ProductUserId_ToString(ProductId, TempBuffer, &TempBufferSize);
		FString returnValue(TempBuffer);
		return returnValue;
	}

	/**
	* Returns a Product ID from a String interpretation of one.
	*
	* @param ProductId the FString representation of a Product ID.
	* @return FProductId A product ID from the string, if valid.
	*/
	static FEpicProductId			FromString(const FString& ProductId)
	{
		EOS_ProductUserId Account = EOS_ProductUserId_FromString(TCHAR_TO_ANSI(*ProductId));
		return FEpicProductId(Account);
	}

	FString						TokenToString() const
	{
		return FString::Printf(TEXT("%p"), ContinuanceToken);
	}
	
	/** The EOS SDK matching Product Id. */
	EOS_ProductUserId			ProductId;

	/** The Credential token associated with this Product Id. */
	EOS_ContinuanceToken ContinuanceToken;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQueryExternalAccountsSucceeded, FEpicAccountId, ProductMappingToAccounts);


UCLASS()
class UEOS_API UEOSConnect : public UObject
{
	GENERATED_BODY()


	/* ==================== VARIABLES ======================== */

protected:
	// Variable that is used to get the display name from whatever subsystem is active
	FString PlayerDisplayName;

	// The token variable that is used to pass into the CreateUser function
	EOS_ContinuanceToken GlobalContinuanceToken;

	// Similar to authentication, stores the credentials associated with login
	FEpicProductId CurrentProductId;

	// Unique to connect interface, lets one know if one has linked this account to other subsystems.
	bool bLinkedAccount;

	//After QueryProductId is called, we can fetch information about all of these
	TArray<FEpicProductId> CurrentQueriedProductIds;

	//How we can get an account id from a product Id
	TMap<FEpicProductId, FEpicAccountId> ExternalToEpicAccountsMap;

	//Broadcast the above information to a map
	UPROPERTY(BlueprintAssignable)
		FOnQueryExternalAccountsSucceeded OnQueryExternalAccountsSucceeded;

	/* ==================== FUNCTIONS ======================== */
public:

	/**
	* EOS Connect Constructor.
	*/
	UEOSConnect();

	/**
	 * In EpicGames - this is just the display name from the UserInfo class
	 * In Steam, this is the display name from the subsystem
	 * TODO - Figure out what will happen with other subsystems
	 *
	 * @param InPlayerDisplayName - name of player as a string
	 */
	UFUNCTION(BlueprintCallable)
		void InitializeParameters(FString InPlayerDisplayName);
	
	const FString EnumToString(const TCHAR* Enum, int32 EnumValue);
	static void OnQueryUserInfoMappingsComplete(const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Info);

	FEpicProductId FORCEINLINE GetProductId() const { return CurrentProductId;  }

	UFUNCTION(BlueprintCallable)
		void QueryUserInfoMappings(TArray<FEpicProductId> UserAccounts);
	
	// Similar to authentication, signifies one has been authorized to access information of an account connected to EAS.
	bool bAuthorized;

	/**
	* Attempts to login to the EOS Connect interface using the specified type of credentials (also known as: Identity Provider).
	*
	* @param ExternalCredentialType
	*/
	UFUNCTION(BlueprintCallable)
		void Login(EExternalCredentialType ExternalCredentialType);

	void CreateUser(const EOS_ContinuanceToken* InContinuanceToken);

	/** Fires when a Login has completed successfully. 
	* According to the documentation (https://dev.epicgames.com/docs/services/en-US/Interfaces/Connect/index.html),
	* when this occurs, the player should be prompted to login with another identity provider,
	* potentially linking their account with an ID they have used before.
	*/
	UPROPERTY(BlueprintAssignable, Category = "UEOS|Connect")
		FOnConnectLoginSuccess OnConnectLoginSuccess;

	/** Fires when a Connect Login has failed through the connect interface.
	*/
	UPROPERTY(BlueprintAssignable, Category = "UEOS|Connect")
		FOnConnectLoginFail OnConnectLoginFail;
	FOnEncryptedAppTicketResponseDelegate ResSteamResult;

	
protected:

	static void CreateUserCompleteCallback(const EOS_Connect_CreateUserCallbackInfo* Data);
	static void OnLinkAccountCallback(const EOS_Connect_LinkAccountCallbackInfo* Data);

	
	static void LoginUserCompleteCallback(const EOS_Connect_LoginCallbackInfo* Data);
	
	/** Handle for Connect interface. */
	EOS_HConnect						ConnectHandle;

protected:
	bool bCallOnce;

private:

	void HandleSteamEncryptedAppTicketResponse(bool bEncryptedDataAvailable, int32 ResultCode);

	/* Helper function that calls the SDK's EOS_Connect_Login function. */
	static void DoEOSConnectLogin(const char* CredentialsToken, EOS_EExternalCredentialType CredentialsType, const EOS_Connect_UserLoginInfo* UserLoginInfo = nullptr);
	
	static FOnlineEncryptedAppTicketSteamPtr GetSteamEncryptedAppTicketInterface();
};
