// Fill out your copyright notice in the Description page of Project Settings.

#include "ThrowComponent_B.h"
#include "BrawlInn.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include "Player/PlayerCharacter_B.h"
#include "Components/HoldComponent_B.h"
#include "Items/Throwable_B.h"
#include "System/GameMode_B.h"

UThrowComponent_B::UThrowComponent_B(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UThrowComponent_B::TryThrow()
{
	if (!IsReady() || !HoldComponent->IsHolding())
		return false;

	bIsThrowing = true;
	return true;
}

bool UThrowComponent_B::IsThrowing() const
{
	return bIsThrowing;
}

void UThrowComponent_B::BeginPlay()
{
	Super::BeginPlay();

	OwningPlayer = Cast<APlayerCharacter_B>(GetOwner());

	GameMode = Cast<AGameMode_B>(UGameplayStatics::GetGameMode(GetWorld()));
	GameMode->SpawnCharacter_NOPARAM_D.AddDynamic(this, &UThrowComponent_B::OneCharacterChanged);
	GameMode->DespawnCharacter_NOPARAM_D.AddDynamic(this, &UThrowComponent_B::OneCharacterChanged);
}

bool UThrowComponent_B::AimAssist(FVector& TargetPlayerLocation)
{
	if (OtherPlayers.Num() == 0)
		return false;

	FVector PlayerLocation = OwningPlayer->GetActorLocation();
	PlayerLocation.Z = 0;
	FVector PlayerForward = PlayerLocation + OwningPlayer->GetActorForwardVector() * AimAssistRange;
	PlayerForward.Z = 0;

	FVector PlayerToForward = PlayerForward - PlayerLocation;

	TArray<APlayerCharacter_B*> PlayersInCone;

	for (const auto& OtherPlayer : OtherPlayers)
	{
		if (OtherPlayer->bHasFallen)
			continue;
		FVector OtherPlayerLocation = OtherPlayer->GetActorLocation();
		OtherPlayerLocation.Z = 0;

		FVector PlayerToOtherPlayer = OtherPlayerLocation - PlayerLocation;
		if (PlayerToOtherPlayer.Size() > AimAssistRange)
			continue;

		float AngleA = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(PlayerToForward.GetSafeNormal(), PlayerToOtherPlayer.GetSafeNormal())));
	//	BScreen("Angle: %f", AngleA);
		if (AngleA <= AimAssistAngle)
			PlayersInCone.Add(OtherPlayer);
	}
	APlayerCharacter_B* TargetPlayer = nullptr;

	switch (PlayersInCone.Num())
	{
	case 0:
		return false;
	case 1:
		TargetPlayer = PlayersInCone[0];
		break;
	default:
		PlayersInCone.Sort([&](const APlayerCharacter_B& LeftSide, const APlayerCharacter_B& RightSide)
			{
				FVector A = LeftSide.GetActorLocation();
				A.Z = 0;
				FVector B = RightSide.GetActorLocation();
				B.Z = 0;
				float DistanceA = FVector::Dist(PlayerLocation, LeftSide.GetActorLocation());
				float DistanceB = FVector::Dist(PlayerLocation, RightSide.GetActorLocation());
				return DistanceA < DistanceB;
			});
		TargetPlayer = PlayersInCone[0];
		break;
	}
//	BScreen("Using AimAssist");
	FVector TargetLocation = TargetPlayer->GetActorLocation();
	TargetLocation.Z = 0;
	FVector ThrowDirection = TargetLocation - PlayerLocation;
	TargetPlayerLocation = ThrowDirection.GetSafeNormal();
	return true;
}
void UThrowComponent_B::OneCharacterChanged()
{
	/// Finds all characters
	OtherPlayers.Empty();
	TArray<AActor*> TempArray;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter_B::StaticClass(), TempArray);
	for (const auto& Player : TempArray)
	{
		if (Cast<APlayerCharacter_B>(Player) == OwningPlayer)
			continue;
	//	BScreen("Player %s, %i", *GetNameSafe(Player), TempArray.Num());
		OtherPlayers.Add(Cast<APlayerCharacter_B>(Player));
	}
	BScreen("OtherPlayer size %i", OtherPlayers.Num());

	/// Finds the holdcomponent.
	// Dette kan kanskje flyttes til BeginPlay ? Usikker p� hvilke av komponentene som blir laget f�rst
	HoldComponent = OwningPlayer->HoldComponent;
}

void UThrowComponent_B::Throw()
{
	if (!HoldComponent->IsHolding())
		return;

	/// Prepare item to be thrown
	FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true);
	HoldComponent->GetHoldingItem()->DetachFromActor(rules);
	HoldComponent->GetHoldingItem()->Dropped();

	/// Throw with the help of AimAssist.
	FVector TargetLocation = OwningPlayer->GetActorForwardVector();
	AimAssist(TargetLocation);
	BScreen("TargetLocation %s", *TargetLocation.ToString());
	HoldComponent->GetHoldingItem()->Mesh->AddImpulse(TargetLocation * ImpulseSpeed, NAME_None, true);
	HoldComponent->SetHoldingItem(nullptr);
	bIsThrowing = false;
}

bool UThrowComponent_B::IsReady() const
{
	if (GameMode && OwningPlayer && HoldComponent)
		return true;
	return false;
}