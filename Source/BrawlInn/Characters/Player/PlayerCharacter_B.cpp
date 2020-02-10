// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter_B.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "BrawlInn.h"
#include "System/GameModes/GameMode_B.h"
#include "Characters/Player/PlayerController_B.h"
#include "Components/HealthComponent_B.h"
#include "Components/ThrowComponent_B.h"
#include "Components/SkeletalMeshComponent.h"
#include "UI/Game/HealthWidget_B.h"
APlayerCharacter_B::APlayerCharacter_B()
{
	DirectionIndicatorPlane = CreateDefaultSubobject<UStaticMeshComponent>("Direction Indicator Plane");
	DirectionIndicatorPlane->SetupAttachment(RootComponent);
	DirectionIndicatorPlane->SetRelativeLocation(FVector(20, 0, -70));
	DirectionIndicatorPlane->SetRelativeRotation(FRotator(0, 90, 0));
	DirectionIndicatorPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DirectionIndicatorPlane->SetRelativeScale3D(FVector(3.327123, 3.327123, 1));



}

void APlayerCharacter_B::BeginPlay()
{
	Super::BeginPlay();

	//Create new material instance and assign it
	if (DirectionIndicatorPlane)
	{
		auto MI_ColoredDecal = UMaterialInstanceDynamic::Create(DirectionIndicatorPlane->GetMaterial(0), this);
		MI_ColoredDecal->SetVectorParameterValue(FName("Color"), PlayerInfo.PlayerColor);
		BWarn("No player controller found for RespawnPawn %s", *GetNameSafe(this));
		DirectionIndicatorPlane->SetMaterial(0, MI_ColoredDecal);
	}
}

void APlayerCharacter_B::Die()
{
	Fall(-1);
	DirectionIndicatorPlane->DestroyComponent();

	PlayerController->TryRespawn(RespawnDelay);
	PlayerController->UnPossess();
}

void APlayerCharacter_B::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	PlayerController = Cast<APlayerController_B>(NewController);
	if (PlayerController && PlayerController->HealthComponent)
	{
		PlayerInfo.ID = UGameplayStatics::GetPlayerControllerID(PlayerController);

		PlayerController->HealthComponent->HealthIsZero_D.AddUObject(this, &APlayerCharacter_B::Die);
		PlayerController->HealthComponent->RespawnIsZero_D.AddUObject(this, &APlayerCharacter_B::Die);
		PlayerController->PlayerInfo = PlayerInfo;
	}
}

void APlayerCharacter_B::FellOutOfWorld(const UDamageType& dmgType)
{
	UGameplayStatics::ApplyDamage(this, FellOutOfWorldDamageAmount, PlayerController, this, dmgType.StaticClass());
	Super::FellOutOfWorld(dmgType);
}