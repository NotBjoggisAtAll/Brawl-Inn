// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Player/PlayerController_B.h"
#include "GamePlayerController_B.generated.h"

class ARespawnPawn_B;
class UHealthWidget_B;

UCLASS()
class BRAWLINN_API AGamePlayerController_B : public APlayerController_B
{
	GENERATED_BODY()

protected:

	void OnPossess(APawn* InPawn) override;

		void DPadUpPressed() override;

	void DPadDownPressed() override;

	void SpecialRightPressed() override;

	void LeftShoulderPressed() override;
	void RightShoulderPressed() override;

	void LeftTriggerPressed() override;
	void LeftTriggerRepeat() override;

	void RightTriggerPressed() override;
	void RightTriggerReleased() override;

	void LeftStickXAxis(float Value) override;

	void LeftStickYAxis(float Value) override;

	void TryPauseGame();

	// ********** Respawn **********
public:
	ARespawnPawn_B* RespawnPawn = nullptr;
protected:
	bool bCanRespawn = true;

	void Respawn() const;

	FTimerHandle TH_RespawnTimer;

public:
	void TryRespawn(const float ReSpawnDelay);


	void SetHealthWidget(UHealthWidget_B* Widget);
protected:

	UPROPERTY()
		UHealthWidget_B* HealthWidget = nullptr;


	// ********** EditorOnly **********
	void Debug_Spawn() const;
	void Debug_DeSpawn();
};