// Fill out your copyright notice in the Description page of Project Settings.

#include "VictoryGameMode_B.h"

#include "BrawlInn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Characters/Player/PlayerCharacter_B.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "TimerManager.h"
#include "LevelSequencePlayer.h"
#include "Characters/Player/PlayerController_B.h"

#include "System/GameInstance_B.h"
#include "UI/Widgets/VictoryScreenWidget_B.h"

void AVictoryGameMode_B::BeginPlay()
{
	Super::BeginPlay();

	ACameraActor* IntroCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), GameInstance->GetCameraSwapTransform());
	check(IsValid(IntroCamera));
	UpdateViewTargets(IntroCamera);

	TArray<AActor*> OutActors;
	TArray<AActor*> CharactersToKeep;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Players", OutActors);

	FindPlayerCharactersAndUpdateMeshMaterial(OutActors, CharactersToKeep);

	DestroyNonRelevantPlayerCharacters(OutActors, CharactersToKeep);

	InitFadeToStatScreenSequence();

	Skip = CreateWidget<UUserWidget>(GetWorld(), Skip_BP);
	Skip->AddToViewport();

	if (GameInstance)
		GameInstance->SetAndPlayMusic(Music);
}

void AVictoryGameMode_B::PostLevelLoad()
{
	checkf(IsValid(VictoryCamera_BP), TEXT("VictoryCamera_BP is not set! Make sure to set it in the Blueprint!"));

	ACameraActor* VictoryCamera = Cast<ACameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), VictoryCamera_BP));

	UpdateViewTargets(VictoryCamera, 3);

	OpenBarDoor();

	FTimerHandle StartFadeToScoreHandle;
	GetWorld()->GetTimerManager().SetTimer(StartFadeToScoreHandle, this, &AVictoryGameMode_B::FadeToStatScreenDelayed, ShowSkipTextDelay, false);

}

void AVictoryGameMode_B::FadeToStatScreenDelayed()
{
	GetWorld()->GetTimerManager().SetTimer(FadeHandle, this, &AVictoryGameMode_B::FadeToStatScreen, StartFadeToScoreDelay, false);
}

void AVictoryGameMode_B::SetCanContinue(const bool Value)
{
	bCanContinue = Value;
}

void AVictoryGameMode_B::FadeToStatScreen()
{
	GetWorld()->GetTimerManager().ClearTimer(FadeHandle);
	FadeToBlackSequencePlayer->Play();
	HideSkipButton();
}

void AVictoryGameMode_B::FadeToStatScreenFinished()
{
	UVictoryScreenWidget_B* VictoryScreen = CreateWidget<UVictoryScreenWidget_B>(GetWorld(), VictoryScreen_BP);
	checkf(VictoryScreen, TEXT("VictoryScreen_BP is not set! Make sure to set it in the Blueprint!"));
	VictoryScreen->AddToViewport();

	for (auto Controller : PlayerControllers)
	{
		Controller->SetInputMode(FInputModeGameOnly());
	}
}

void AVictoryGameMode_B::FindPlayerCharactersAndUpdateMeshMaterial(TArray<AActor*>& OutActors, TArray<AActor*>& CharactersToKeep)
{
	TArray<FName> VictoryTags;
	VictoryTags.Add("First");
	VictoryTags.Add("Second");
	VictoryTags.Add("Third");
	VictoryTags.Add("Fourth");
	
	for (int i = 0; i < GameInstance->GetPlayerInfos().Num(); i++)
	{
		const auto Character = OutActors.FindByPredicate([&](AActor* Actor)
		{
			return Actor->ActorHasTag(VictoryTags[i]);
		});
		CharactersToKeep.Add(*Character);
		APlayerCharacter_B* PlayerCharacter = Cast<APlayerCharacter_B>(*Character);
		PlayerCharacter->GetMesh()->CreateAndSetMaterialInstanceDynamicFromMaterial(1, GameInstance->GetPlayerInfos()[i].CharacterVariant.MeshMaterial);
		PlayerCharacter->GetDirectionIndicatorPlane()->DestroyComponent();
	}
}

void AVictoryGameMode_B::DestroyNonRelevantPlayerCharacters(TArray<AActor*>& OutActors, TArray<AActor*>& CharactersToKeep)
{
	for (auto Actor : CharactersToKeep)
		OutActors.Remove(Actor);

	for (auto Actor : OutActors)
		Actor->Destroy();
}

void AVictoryGameMode_B::InitFadeToStatScreenSequence()
{
	checkf(IsValid(FadeToBlackSequence), TEXT("Remember to set FadeToBlackSequence in the blueprint!"));
	ALevelSequenceActor* SequenceActor;
	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	Settings.bPauseAtEnd = true;
	FadeToBlackSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), FadeToBlackSequence, Settings, SequenceActor);
	FadeToBlackSequencePlayer->OnFinished.AddDynamic(this, &AVictoryGameMode_B::FadeToStatScreenFinished);
	check(IsValid(FadeToBlackSequencePlayer));
}