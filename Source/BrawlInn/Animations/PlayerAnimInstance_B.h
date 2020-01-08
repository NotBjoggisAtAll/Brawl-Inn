// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance_B.generated.h"

class APlayerCharacter_B;

UCLASS()
class BRAWLINN_API UPlayerAnimInstance_B : public UAnimInstance
{
	GENERATED_BODY()
protected:

	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UFUNCTION(BlueprintPure)
	bool IsReady() const;

	APlayerCharacter_B* Owner = nullptr;

	/// Movement variables

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Direction = 0;

	/// Action variables

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsPunching = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsHolding = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsThrowing = false;
	
};