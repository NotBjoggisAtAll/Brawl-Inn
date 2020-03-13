// Fill out your copyright notice in the Description page of Project Settings.

#include "MainGameMode_B.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"
#include "Sound/SoundCue.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"
#include "Camera/CameraComponent.h"

#include "BrawlInn.h"
#include "System/Camera/GameCamera_B.h"
#include "Characters/Player/GamePlayerController_B.h"
#include "GameFramework/SpringArmComponent.h"
#include "System/GameInstance_B.h"
#include "System/SubSystems/ScoreSubSystem_B.h"
#include "UI/Menus/PauseMenu_B.h"
#include "UI/Game/VictoryScreenWidget_B.h"
#include "UI/Game/GameOverlay_B.h"

void AMainGameMode_B::BeginPlay()
{
	Super::BeginPlay();

	if (GameInstance && Birds)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Birds, 0.75 * GameInstance->GetMasterVolume() * GameInstance->GetSfxVolume(), 1.0f, FMath::FRandRange(0, 100));
	}
	if (GameInstance && River)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), River, 0.75 * GameInstance->GetMasterVolume() * GameInstance->GetSfxVolume(), 1.0f, FMath::FRandRange(0, 100));
	}

	/// Spawns and setups camera
	GameCamera = GetWorld()->SpawnActor<AGameCamera_B>(BP_GameCamera, GameInstance->GetCameraSwapTransform());
	//GameCamera->SpringArm->TargetArmLength = 0;
//	GameCamera->SpringArm->SetRelativeRotation(FRotator());
	FromCharacterSelectionCamera = GetWorld()->SpawnActor<ACameraActor>(BP_FromCharacterSelectionCamera, GameInstance->GetCameraSwapTransform());

	/// Create overlay
	Overlay = CreateWidget<UGameOverlay_B>(GetWorld(), BP_GameOverlay);
	Overlay->UpdateTimerText(TimeRemaining);
	Overlay->AddToViewport();

	/// Spawns characters for the players
	for (const FPlayerInfo Info : GameInstance->GetPlayerInfos())
	{
		AGamePlayerController_B* PlayerController = Cast<AGamePlayerController_B>(UGameplayStatics::GetPlayerControllerFromID(GetWorld(), Info.ID));
		if (!PlayerController) { BError("PlayerController for id %i not found. Check IDs in GameInstance", Info.ID); continue; }

		SpawnCharacter_D.Broadcast(Info, false, FTransform());

	}
	OnGameOver.AddUObject(this, &AMainGameMode_B::EndGame);

	UpdateViewTargets(FromCharacterSelectionCamera);


	////GameCamera->SetActorTickEnabled(false);

	GetWorld()->GetTimerManager().SetTimer(h, [&]()
		{
			GameCamera->FakeTick();
			BLog("GameStart");
		}, 0.05f, true);

	GetWorld()->GetTimerManager().SetTimer(h3, [&]()
		{
			GetWorld()->GetTimerManager().ClearTimer(h);
			UpdateViewTargets(nullptr, 3, true);

			BLog("Clear GameStart timer");
		}, 0.1f, false, 1);



	GetWorld()->GetTimerManager().SetTimer(h2, [&]()
		{
			GameCamera->SetActorTickEnabled(true);
			BLog("Tick turned on");
		}, 0.1f, false, 4.f);

	StartGame();

}

void AMainGameMode_B::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AMainGameMode_B::StartGame()
{
	if (GameInstance->GameIsScoreBased())
	{
		GetWorld()->GetTimerManager().SetTimer(TH_CountdownTimer, [&]() {
			if (Overlay)
			{
				Overlay->UpdateTimerText(--TimeRemaining);
				if (TimeRemaining < 0)
					OnGameOver.Broadcast();
			}
			}, 1.f, true);
	}
}

void AMainGameMode_B::EndGame()
{
	GetWorld()->GetTimerManager().PauseTimer(TH_CountdownTimer);
	TArray<AGamePlayerController_B*> TempPlayerControllers = PlayerControllers;
	TempPlayerControllers.Sort([&](const AGamePlayerController_B& Left, const AGamePlayerController_B& Right)
		{
			return Left.GetLocalPlayer()->GetSubsystem<UScoreSubSystem_B>()->GetScoreValues().Score > Right.GetLocalPlayer()->GetSubsystem<UScoreSubSystem_B>()->GetScoreValues().Score;
		});
	UVictoryScreenWidget_B* VictoryScreen = CreateWidget<UVictoryScreenWidget_B>(UGameplayStatics::GetPlayerControllerFromID(GetWorld(), UGameplayStatics::GetPlayerControllerID(TempPlayerControllers[0])), BP_VictoryScreen);
	VictoryScreen->AddToViewport();
}

void AMainGameMode_B::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsValid(PauseMenuWidget))
		return;
	PauseMenuWidget->MenuTick();
}

void AMainGameMode_B::PauseGame(AGamePlayerController_B* ControllerThatPaused)
{
	PlayerControllerThatPaused = ControllerThatPaused;
	PauseMenuWidget = CreateWidget<UPauseMenu_B>(ControllerThatPaused, BP_PauseMenu);
	PauseMenuWidget->AddToViewport();
	const FInputModeUIOnly InputMode;
	ControllerThatPaused->SetInputMode(InputMode);
	SetTickableWhenPaused(true);
	UGameplayStatics::SetGamePaused(GetWorld(), true);
}

void AMainGameMode_B::ResumeGame()
{
	PauseMenuWidget->RemoveFromViewport();
	if (PlayerControllerThatPaused)
	{
		const FInputModeGameOnly InputMode;
		PlayerControllerThatPaused->SetInputMode(InputMode);
	}
	SetTickableWhenPaused(true);
	UGameplayStatics::SetGamePaused(GetWorld(), false);
}

