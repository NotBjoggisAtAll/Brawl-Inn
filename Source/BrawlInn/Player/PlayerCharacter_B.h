// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter_B.generated.h"

class UHealthComponent_B;
class UHoldComponent_B;
class UThrowComponent_B;
class UPunchComponent_B;
class APlayerController_B;

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EState : uint8
{
	EWalking 	UMETA(DisplayName = "Walking"),
	EHolding 	UMETA(DisplayName = "Holding"),
	EFallen		UMETA(DisplayName = "Fallen")
};

UCLASS()
class BRAWLINN_API APlayerCharacter_B : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter_B();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UPunchComponent_B* PunchComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UHoldComponent_B* HoldComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UThrowComponent_B* ThrowComponent;

protected:

	// ** Overriden functions **
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual float TakeDamage (float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	
	// ** Overlap/Hit functions **
	UFUNCTION()
	void CapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// ** Functions **
	void HandleMovement(float DeltaTime);
	void HandleMovementHold();

	void HandleRotation();

	UFUNCTION()
	void Fall();

	void StandUp();

public:	

	UFUNCTION(BlueprintPure)
	APlayerController_B* GetPlayerController_B() const;
	
	void PunchButtonPressed();

	// ** Variables **

	FVector InputVector = FVector::ZeroVector;
	FVector RotationVector = FVector::ZeroVector;
	EState State = EState::EWalking;

protected: 

	UPROPERTY(EditAnywhere, Category = "Variables")
	float RecoveryTime = 2.0;

	UPROPERTY(EditAnywhere, Category = "Variables")
	float TimeBeforeFall = 1.f;

	UPROPERTY(EditAnywhere, Category = "Variables")
	float FallLimitMultiplier = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Variables")
	int FellOutOfWorldDamageAmount = 1;

private:
	
	FTransform RelativeMeshTransform;
	FTimerHandle TH_RecoverTimer;
	float CurrentFallTime = 0.f;

	APlayerController_B* PlayerController = nullptr;
	friend class UPunchComponent_B;
};
