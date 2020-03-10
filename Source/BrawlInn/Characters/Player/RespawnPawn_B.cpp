// Fill out your copyright notice in the Description page of Project Settings.

#include "RespawnPawn_B.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "BrawlInn.h"
#include "Characters/Player/PlayerController_B.h"
#include "System/GameModes/MainGameMode_B.h"
#include "Hazards/BounceActor/BounceActorSpawner_B.h"
#include "Hazards/BounceActor/BounceActor_B.h"

// Sets default values
ARespawnPawn_B::ARespawnPawn_B()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere);
	
	Decal = CreateDefaultSubobject<UDecalComponent>("Decal");
	Decal->AddLocalRotation(FRotator(90, 0, 0));
	Decal->SetupAttachment(Sphere);
}

// Called when the game starts or when spawned
void ARespawnPawn_B::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(TH_ThrowTimer, this, &ARespawnPawn_B::ThrowBarrel, TimeUntilThrow);

}

void ARespawnPawn_B::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void ARespawnPawn_B::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//set the color for the decal
	if (Decal)
	{
		auto MI_ColoredDecal = UMaterialInstanceDynamic::Create(Decal->GetMaterial(0), this);
		APlayerController_B* PlayerController = Cast<APlayerController_B>(NewController);
		if (PlayerController)
			MI_ColoredDecal->SetVectorParameterValue(FName("Color"), PlayerController->GetPlayerInfo().CharacterVariant.TextColor);
		else
			BWarn("No player controller found for RespawnPawn %s", *GetNameSafe(this));
		Decal->SetMaterial(0, MI_ColoredDecal);
	}
}

// Called every frame
void ARespawnPawn_B::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bBarrelIsThrown)
	{
	//	AddMovementInput(InputVector, DeltaTime * MovementSpeed);
		 AddActorWorldOffset(InputVector * DeltaTime * MovementSpeed,true);
	}
}

void ARespawnPawn_B::ThrowBarrel()
{
	if (!bBarrelIsThrown)
	{
		ABounceActorSpawner_B* BarrelSpawner = Cast<ABounceActorSpawner_B>(UGameplayStatics::GetActorOfClass(GetWorld(), ABounceActorSpawner_B::StaticClass()));
		if (!BarrelSpawner) return;

		Barrel = BarrelSpawner->SpawnBounceActor(GetActorLocation());

		//Barrel now spawns the player...
		if (Barrel)
		{
			Barrel->SetupBarrel(Cast<APlayerController_B>(GetController()));
			AMainGameMode_B* GameMode = Cast<AMainGameMode_B>(UGameplayStatics::GetGameMode(GetWorld()));
			if (GameMode)
			{
				GameMode->AddCameraFocusPoint(Barrel);
				GameMode->RemoveCameraFocusPoint(this);
			}
		}
		bBarrelIsThrown = true;
	}
	else if (Barrel)
	{
		Barrel->SpawnPlayerCharacter();
	}
}

void ARespawnPawn_B::SetInputVectorX(float X)
{
	InputVector.X = X;
}

void ARespawnPawn_B::SetInputVectorY(float Y)
{
	InputVector.Y = Y;
}

