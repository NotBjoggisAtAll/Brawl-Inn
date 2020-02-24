// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Character_B.h"
#include "Characters/Player/PlayerInfo.h"
#include "PlayerCharacter_B.generated.h"

class AController;
class APlayerController_B;

UCLASS()
class BRAWLINN_API APlayerCharacter_B : public ACharacter_B
{
	GENERATED_BODY()

public:
	APlayerCharacter_B();

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* DirectionIndicatorPlane = nullptr;

	// ********** AActor **********
protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	// ********** Falling **********

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	UFUNCTION()
		void Die();

	virtual void Fall(float RecoveryTime = -1) override;

	// ********** Hold players **********

	virtual void Dropped_Implementation() override;

public:
	void BreakFreeButtonMash();

protected:
	void BreakFree();

	UPROPERTY(EditAnywhere, Category = "Variables", meta = (Tooltip = "The longest amount of time this character can be held"))
		float MaxHoldTime = 4.f;

	float CurrentHoldTime = 0.f;

	// ********** Stun **********

	virtual void RemoveStun() override;

	// ********** Damage **********

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|UI")
		UTexture2D* ColoredHealthIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|UI")
		UTexture2D* GreyHealthIcon = nullptr;

	UPROPERTY(EditAnywhere, Category = "Variables|Info")
		float RespawnDelay = 3.f;

	// ********** Misc. **********

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
		FPlayerInfo PlayerInfo;

	UPROPERTY(EditAnywhere)
		class UScoreDataTable_B* Table;

protected:
	UPROPERTY()
		APlayerController_B* PlayerController = nullptr;

	virtual void PossessedBy(AController* NewController) override;
};
