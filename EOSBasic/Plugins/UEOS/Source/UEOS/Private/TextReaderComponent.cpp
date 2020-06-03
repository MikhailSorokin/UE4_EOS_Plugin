// Fill out your copyright notice in the Description page of Project Settings.


#include "TextReaderComponent.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"

UTextReaderComponent::UTextReaderComponent()
{

}


FString UTextReaderComponent::ReadFile(FString filePath)
{
	//Read file from [project]/filePath/
	FString FolderPath = FPaths::ProjectContentDir() + "Credentials";
	FString Directory = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FolderPath);

	FString Result = "";
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();
	if (file.CreateDirectory(*Directory)) {
		FString myFile = Directory + "/" + filePath;

		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, "Attempting to locate: " + myFile);
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, "in directory: " + Directory);
		}

		FFileHelper::LoadFileToString(Result, *myFile);

		if (GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, "Result: " + Result);
		}
	}

	return Result;
}