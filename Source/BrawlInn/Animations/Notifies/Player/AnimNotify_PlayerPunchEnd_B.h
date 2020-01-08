// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_PlayerPunchEnd_B.generated.h"

class APlayerCharacter_B;

UCLASS()
class BRAWLINN_API UAnimNotify_PlayerPunchEnd_B : public UAnimNotify
{
	GENERATED_BODY()
	
public:

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

private:
	APlayerCharacter_B* Player = nullptr;
};
