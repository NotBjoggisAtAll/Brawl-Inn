// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameCamera_B.generated.h"

class USpringArmComponent;
class UCameraComponent;
class APlayerController_B;

UCLASS()
class BRAWLINN_API AGameCamera_B : public AActor
{
	GENERATED_BODY()
	
public:	
	AGameCamera_B();

	UPROPERTY()
	USceneComponent* Scene = nullptr;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* SpringArm = nullptr;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* Camera = nullptr;

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
		float SmallestSpringArmLength = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
		float LargestSpringArmLength = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
		float BorderWidth = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
		float MaxCameraHeight = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
		float MinCameraHeight = 0.f;
private:

	TArray<APlayerController_B*> PlayerControllers;

	void UpdateCamera();

	void SetSpringArmLength(float distanceToFurthestPlayer);

};