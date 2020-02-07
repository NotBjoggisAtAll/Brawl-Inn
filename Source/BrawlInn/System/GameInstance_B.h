// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "System/BrawlEnums.h"
#include "GameInstance_B.generated.h"

USTRUCT(BlueprintType)
struct FPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ID = 0;

//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//		EPlayerCharacterType Type = EPlayerCharacterType::YUGGO;
};

class USoundBase;

DECLARE_MULTICAST_DELEGATE(FNumberOfPlayerControllersChanged);

UCLASS()
class BRAWLINN_API UGameInstance_B : public UGameInstance
{
	GENERATED_BODY()

public:

	UGameInstance_B();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables|Audio")
		float MasterVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables|Audio")
		float MusicVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables|Audio")
		float SfxVolume = 1.f;

	UFUNCTION()
		void AddPlayerInfo(FPlayerInfo PlayerInfo);

	UFUNCTION()
		void RemovePlayerInfo(int ID);

	UFUNCTION()
		TArray<FPlayerInfo> GetPlayerInfos() const;

	UFUNCTION()
		void SetPlayerInfos(TArray<FPlayerInfo> NewPlayerInfos);

	FNumberOfPlayerControllersChanged OnPlayerInfoChanged;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables|Players")
		TArray<FPlayerInfo> ActivePlayerInfos;
};
