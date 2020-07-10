#pragma once
#include "CoreMinimal.h"
//#include "EngineMinimal.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerState.h"
//#include "Core.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "UEOSCommon.h"

#include "UEOSOnlineTypes.generated.h"	


/** Login Mode for Authentication/Login */
UENUM()
enum class ELoginMode : uint8
{
	LM_Unknown			UMETA(DisplayName = "Unknown"),		/** Unknown Login Type */
	LM_IDPassword		UMETA(DisplayName = "Password"),		/** Login using a user id and password token */
	LM_ExchangeCode		UMETA(DisplayName = "Exchange Code"),	/** Login using an exchange code */
	LM_PinGrant			UMETA(DisplayName = "Pin Grant"),		/** Login using a pin grant */
	LM_DevTool			UMETA(DisplayName = "Dev Tool")		/** Login using the EOS SDK Dev Auth Tool */,
	LM_AccountPortal    UMETA(DisplayName = "Account Portal")		/** Logins using a web portal login through your browser. */,
	LM_PersistentAuth	UMETA(DisplayName = "Persistent Authentication") /** Remembers your credential tokens and keeps that throughout the project. */,
	LM_ExternalAuth		UMETA(DisplayName = "External Authentication")	/** Logins with an identity provider and links that account to an Epic Games Account. */,
};

/**
* Adapted from the sample, to work within UE4 UBT.
*/
USTRUCT(BlueprintType)
struct FEpicAccountId
{
	GENERATED_BODY()

	/**
	* Construct wrapper from account id.
	*/
	FEpicAccountId(EOS_EpicAccountId InAccountId)
	: EpicAccountId(InAccountId)
	{
		AccountIdAsString = ToString();
	};

	FEpicAccountId() = default;

	FEpicAccountId(const FEpicAccountId&) = default;

	FEpicAccountId& operator=(const FEpicAccountId&) = default;

	bool operator==(const FEpicAccountId& Other) const
	{
		return EpicAccountId == Other.EpicAccountId;
	}

	bool operator!=(const FEpicAccountId& Other) const
	{
		return !(this->operator==(Other));
	}

	bool operator<(const FEpicAccountId& Other) const
	{
		return EpicAccountId < Other.EpicAccountId;
	}

	/**
	* Checks if account ID is valid.
	*/
	operator bool() const;

	/**
	* Easy conversion to EOS account ID.
	*/
	operator EOS_EpicAccountId() const
	{
		return EpicAccountId;
	}

	UPROPERTY(BlueprintReadOnly)
		FString AccountIdAsString;

	/**
	* Prints out account ID as hex.
	*/
	FString						ToString() const;

	/**
	* Returns an Account ID from a String interpretation of one.
	*
	* @param AccountId the FString representation of an Account ID.
	* @return FEpicAccountId An Account ID from the string, if valid.
	*/
	static FEpicAccountId		FromString(const FString& AccountId);

	/** The EOS SDK matching Account Id. */
	EOS_EpicAccountId			EpicAccountId;

};

/**
 * An enumeration of the different friendship statuses. Modified from eos_friends_types.h
 */
UENUM(BlueprintType)
enum class EBPFriendStatus : uint8
{
	/** The two accounts have no friendship status */
	NotFriends = 0,
	/** The local account has sent a friend invite to the other account */
	InviteSent = 1,
	/** The other account has sent a friend invite to the local account */
	InviteReceived = 2,
	/** The accounts have accepted friendship */
	Friends = 3
};

/** The current status of a friend's online presence.  EPresenceStatus as a UENUM. */
UENUM(BlueprintType)
enum class EPresenceStatus : uint8
{
	Offline = 0			UMETA(DisplayName = "Offline"),		/** Offline Presence */
	Online = 1			UMETA(DisplayName = "Online"),		/** Online Presence */
	Away = 2			UMETA(DisplayName = "Away"),		/** Away Presence */
	ExtendedAway = 3	UMETA(DisplayName = "ExtendedAway"),/** Extended Away - Longer than 1 hour away **/
	DoNotDisturb = 4	UMETA(DisplayName = "DND")			/** Do not Disturb **/
};

/**
 * Different platforms that will be in use.
 */
UENUM(BlueprintType)
enum class EPlatformType : uint8
{
	Epic = 0,
	Steam = 1,
	Playstation = 2,
	Other = 4
};

/* Presence Info struct that is needed in the friends class. */
USTRUCT(BlueprintType)
struct FBPPresenceInfo
{
	GENERATED_USTRUCT_BODY()
	
	/** Presence status */
	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		EPresenceStatus Presence;

	/** Rich text - honestly not sure what this is */
	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		FString RichText;

	/** Tells user what application one is playing. */
	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		FString Application;

	/** Platform the user is logged in on */
	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		FString Platform;
	
	/** Equaity operator */
	bool operator==(const FBPPresenceInfo& Other) const
	{
		return Presence == Other.Presence &&
			RichText == Other.RichText &&
			Application == Other.Application &&
			Platform == Other.Platform;
	}
};

USTRUCT(BlueprintType)
struct FBPCrossPlayInfo
{

	GENERATED_BODY()

	FBPCrossPlayInfo() = default;

	FBPCrossPlayInfo(const FBPCrossPlayInfo&) = default;

	FBPCrossPlayInfo& operator=(const FBPCrossPlayInfo&) = default;

	bool operator==(const FBPCrossPlayInfo& Other) const
	{
		return AccountId == Other.AccountId;
	}

	bool operator!=(const FBPCrossPlayInfo& Other) const
	{
		return !(this->operator==(Other));
	}

	bool operator<(const FBPCrossPlayInfo& Other) const
	{
		return AccountId < Other.AccountId;
	}

	//TODO - Possibly will have to use productId rather than accountId? Maybe have both?
	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		FEpicAccountId AccountId;

	/* Status of friendship of this account in relation to us */
	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		EBPFriendStatus FriendshipStatus;

	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		FString DisplayName;

	/* Full presence information including platform type, application, status, rich text. */
	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		FBPPresenceInfo Presence;

	UPROPERTY(BlueprintReadWrite, Category = "UEOS|Friends|CrossPlay")
		EPlatformType PlatformType = EPlatformType::Epic;

	/**
	* Returns an Account ID from a String interpretation of one.
	*
	* @param AccountId the FString representation of an Account ID.
	* @return FEpicAccountId An Account ID from the string, if valid.
	*/
	/*static FEpicAccountId		FromString(const FString& AccountId)
	{
		EOS_EpicAccountId Account = EOS_EpicAccountId_FromString(TCHAR_TO_ANSI(*AccountId));
		return FEpicAccountId(Account);
	}*/
};


// Wanted this to be switchable in the editor
UENUM(BlueprintType)
enum class EBPOnlinePresenceState : uint8
{
	Online,
	Offline,
	Away,
	ExtendedAway,
	DoNotDisturb,
	Chat
};

UENUM(BlueprintType)
enum class EBPOnlineSessionState : uint8
{
	/** An online session has not been created yet */
	NoSession,
	/** An online session is in the process of being created */
	Creating,
	/** Session has been created but the session hasn't started (pre match lobby) */
	Pending,
	/** Session has been asked to start (may take time due to communication with backend) */
	Starting,
	/** The current session has started. Sessions with join in progress disabled are no longer joinable */
	InProgress,
	/** The session is still valid, but the session is no longer being played (post match lobby) */
	Ending,
	/** The session is closed and any stats committed */
	Ended,
	/** The session is being destroyed */
	Destroying
};

// Boy oh boy is this a dirty hack, but I can't figure out a good way to do it otherwise at the moment
// The UniqueNetId is an abstract class so I can't exactly re-initialize it to make a shared pointer on some functions
// So I made the blueprintable UniqueNetID into a dual variable struct with access functions and I am converting the const var for the pointer
// I really need to re-think this later
USTRUCT(BlueprintType)
struct FBPUniqueNetId
{
	GENERATED_USTRUCT_BODY()

private:
	bool bUseDirectPointer;


public:
	TSharedPtr<const FUniqueNetId> UniqueNetId;
	const FUniqueNetId * UniqueNetIdPtr;

	void SetUniqueNetId(const TSharedPtr<const FUniqueNetId> &ID)
	{
		bUseDirectPointer = false;
		UniqueNetIdPtr = nullptr;
		UniqueNetId = ID;
	}

	void SetUniqueNetId(const FUniqueNetId *ID)
	{
		bUseDirectPointer = true;
		UniqueNetIdPtr = ID;
	}

	bool IsValid() const
	{
		if (bUseDirectPointer && UniqueNetIdPtr != nullptr && UniqueNetIdPtr->IsValid())
		{
			return true;
		}
		else if (UniqueNetId.IsValid())
		{
			return true;
		}
		else
			return false;

	}

	const FUniqueNetId* GetUniqueNetId() const
	{
		if (bUseDirectPointer && UniqueNetIdPtr != nullptr)
		{
			// No longer converting to non const as all functions now pass const UniqueNetIds
			return /*const_cast<FUniqueNetId*>*/(UniqueNetIdPtr);
		}
		else if (UniqueNetId.IsValid())
		{
			return UniqueNetId.Get();
		}
		else
			return nullptr;
	}

	FBPUniqueNetId()
	{
		bUseDirectPointer = false;
		UniqueNetIdPtr = nullptr;
	}

    virtual bool Equals(const FBPUniqueNetId& Other) const
    {
        return UniqueNetId == Other.UniqueNetId && bUseDirectPointer == Other.bUseDirectPointer;
    }
};

FORCEINLINE uint32 GetTypeHash(const FBPUniqueNetId& Other)
{
    return GetTypeHash(Other.UniqueNetId);
}

FORCEINLINE bool operator== (const FBPUniqueNetId& First, const FBPUniqueNetId& Second)
{
    return First.Equals(Second);
}

template<>
struct TStructOpsTypeTraits<FBPUniqueNetId> : public TStructOpsTypeTraitsBase2<FBPUniqueNetId>
{
    enum
    {
        WithIdenticalViaEquality = true,
    };
};

USTRUCT(BluePrintType)
struct FBPOnlineUser
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FBPUniqueNetId UniqueNetId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Online|Friend")
		FString RealName;
};


/** The types of comparison operations for a given search query */
// Used to compare session properties
UENUM(BlueprintType)
enum class EOnlineComparisonOpRedux : uint8
{
	Equals,
	NotEquals,
	GreaterThan,
	GreaterThanEquals,
	LessThan,
	LessThanEquals,
};
