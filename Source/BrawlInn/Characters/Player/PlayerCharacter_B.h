// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Character_B.h"
#include "PlayerCharacter_B.generated.h"

UCLASS()
class BRAWLINN_API APlayerCharacter_B : public ACharacter_B
{
	GENERATED_BODY()

public:

	APlayerCharacter_B();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* DirectionIndicatorPlane = nullptr;

};
