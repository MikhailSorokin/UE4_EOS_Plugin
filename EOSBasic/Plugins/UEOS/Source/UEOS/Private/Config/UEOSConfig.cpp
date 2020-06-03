// Copyright (C) Gaslight Games Ltd, 2019-2020

#include "Config/UEOSConfig.h"
#include "TextReaderComponent.h"

UEOSConfig::UEOSConfig()
	: ProductName( "EOS Plugin" )
	, ProductVersion( "1.0" )
	, ProductId( "" )
	, SandboxId( "" )
	, DeploymentId( "" )
	, ClientId( "" )
	, ClientSecret( "" )
	, bReadFiles(false)
	, bIsServer( false )
	, LogLevel( ELogLevel::LL_VeryVerbose )
{

	//UE_LOG(UEOSLog, Log, TEXT("HERE"));
}

void UEOSConfig::SetVariables(FString InProductId, FString InDeploymentId, FString InSandboxId, FString InClientId, FString InClientSecret)
{
	ProductId = InProductId;
	DeploymentId = InDeploymentId;
	SandboxId = InSandboxId;
	ClientId = InClientId;
	ClientSecret = InClientSecret;
}
