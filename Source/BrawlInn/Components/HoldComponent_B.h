// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "HoldComponent_B.generated.h"

class ACharacter_B;

UCLASS()
class BRAWLINN_API UHoldComponent_B : public USphereComponent
{
	GENERATED_BODY()

public:
	UHoldComponent_B(const FObjectInitializer& ObjectInitializer);

	// ********** UActorComponent **********

protected:
	virtual void BeginPlay() override;

	// ********** Pickup **********
public:
	bool TryPickup();

protected:
	void Pickup(AActor* Item);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PickupAngle = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float InnerPickupRange = 25.f;

	// ********** Holding **********
public:
	bool IsHolding() const;

	AActor* GetHoldingItem() const;

	void SetHoldingItem(AActor* Item);
protected:

	UPROPERTY()
		AActor* HoldingItem = nullptr;

	UPROPERTY(EditAnywhere)
		FName HoldingSocketName = FName("middle1_L_export_jnt");

	// ********** Misc. **********
public:
	void Drop();

protected:
	UPROPERTY()
		ACharacter_B* OwningCharacter = nullptr;

};
