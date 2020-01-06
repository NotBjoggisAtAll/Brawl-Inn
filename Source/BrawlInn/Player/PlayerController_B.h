// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerController_B.generated.h"

/**
 * 
 */

class APlayerCharacter_B;

UCLASS()
class BRAWLINN_API APlayerController_B : public APlayerController
{
	GENERATED_BODY()

public:
		virtual void BeginPlay() override;
		virtual void SetupInputComponent() override;

private:

	void MoveUp(float Value);
	void MoveRight(float Value);

	APlayerCharacter_B* PlayerCharacter = nullptr;
};
