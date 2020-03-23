// Fill out your copyright notice in the Description page of Project Settings.

#include "HoldComponent_B.h"
#include "Components/SkeletalMeshComponent.h"

#include "BrawlInn.h"
#include "Characters/Character_B.h"
#include "Components/PunchComponent_B.h"
#include "Items/Useable_B.h"

UHoldComponent_B::UHoldComponent_B(const FObjectInitializer& ObjectInitializer)
{
	SphereRadius = 200.f;
	PrimaryComponentTick.bCanEverTick = false;
}

void UHoldComponent_B::BeginPlay()
{
	Super::BeginPlay();
	
	SetCollisionProfileName("Pickup-Trigger");
	
	OwningCharacter = Cast<ACharacter_B>(GetOwner());
}

bool UHoldComponent_B::TryPickup()
{
	if (!OwningCharacter || OwningCharacter->GetState() != EState::EWalking) return false;
	if (OwningCharacter->PunchComponent->GetIsPunching()) return false;
	BWarn("Trying Pickup!");
	TArray<AActor*> OverlappingThrowables;
	GetOverlappingActors(OverlappingThrowables, UThrowableInterface_B::StaticClass());
	OverlappingThrowables.Remove(OwningCharacter);

	if (OverlappingThrowables.Num() == 0)
		return false;

	FVector PlayerLocation = OwningCharacter->GetMesh()->GetComponentLocation();
	PlayerLocation.Z = 0;
	FVector PlayerForward = PlayerLocation + OwningCharacter->GetActorForwardVector() * SphereRadius;
	PlayerForward.Z = 0;

	const FVector PlayerToForward = PlayerForward - PlayerLocation;

	TArray<AActor*> ThrowableItemsInCone;
	for (const auto& Item : OverlappingThrowables)
	{
		if (!IsValid(Item) || Item == OwningCharacter)
			continue;

		IThrowableInterface_B* Interface = Cast<IThrowableInterface_B>(Item);
		if (!Interface) continue;

		if (Interface->Execute_IsHeld(Item)) continue; //Had a crash here before adding cleanup

		FVector ItemLocation = Item->GetActorLocation();
		ItemLocation.Z = 0;

		FVector PlayerToItem = ItemLocation - PlayerLocation;

		const float AngleA = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(PlayerToForward.GetSafeNormal(), PlayerToItem.GetSafeNormal())));
		if (AngleA <= PickupAngle)
			ThrowableItemsInCone.Add(Item);
	}
	AActor* NearestItem;

	switch (ThrowableItemsInCone.Num())
	{
	case 0:
		OverlappingThrowables.Sort([&](const AActor& LeftSide, const AActor& RightSide)
			{
				const float DistanceA = FVector::Dist(PlayerLocation, LeftSide.GetActorLocation());
				const float DistanceB = FVector::Dist(PlayerLocation, RightSide.GetActorLocation());
				return DistanceA < DistanceB;
			});
		NearestItem = OverlappingThrowables[0];
		break;
	case 1:
		NearestItem = ThrowableItemsInCone[0]; //crashed?
		break;
	default:
		ThrowableItemsInCone.Sort([&](const AActor& LeftSide, const AActor& RightSide)
			{
				const float DistanceA = FVector::Dist(PlayerLocation, LeftSide.GetActorLocation());
				const float DistanceB = FVector::Dist(PlayerLocation, RightSide.GetActorLocation());
				return DistanceA < DistanceB;
			});
		NearestItem = ThrowableItemsInCone[0];
		break;
	}
	ACharacter_B* Character = Cast<ACharacter_B>(NearestItem);
	if (Character)
	{
		if (Character->Execute_CanBeHeld(Character) && !Character->IsInvulnerable())
		{
			Pickup(NearestItem);
			return true;
		}
		return false;
	}

	IThrowableInterface_B* Interface = Cast<IThrowableInterface_B>(NearestItem);
	if (Interface->Execute_CanBeHeld(NearestItem))
		Pickup(NearestItem);

	return true;
}

void UHoldComponent_B::Pickup(AActor* Item)
{
	IThrowableInterface_B* Interface = Cast<IThrowableInterface_B>(Item);
	if (Interface)
		Interface->Execute_PickedUp(Item, OwningCharacter);

	FVector Offset = FVector::ZeroVector;
	FName HoldSocketName = HoldingSocketName;
	if (Item->IsA(AUseable_B::StaticClass()))
	{
		Offset = OwningCharacter->HoldingDrinkOffset;
		HoldSocketName = FName("HoldingDrinkSocket");
	}
	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	Item->AttachToComponent(Cast<USceneComponent>(OwningCharacter->GetMesh()), Rules, HoldSocketName);
	HoldingItem = Item;

	ACharacter_B* Character = Cast<ACharacter_B>(Item);

	if (Character)
	{
		Character->AddActorLocalOffset(FVector(0, 0, 75));
		Character->SetActorRelativeRotation(Character->GetHoldRotation());
	}
	else
	{
		Item->AddActorLocalOffset(Offset);
		Item->SetActorRelativeRotation(Cast<AItem_B>(Item)->GetHoldRotation());
	}
	OwningCharacter->SetState(EState::EHolding);
}

bool UHoldComponent_B::IsHolding() const
{
	return (HoldingItem ? true : false);
}

AActor* UHoldComponent_B::GetHoldingItem() const
{
	return HoldingItem;
}

void UHoldComponent_B::SetHoldingItem(AActor* Item)
{
	HoldingItem = Item;
}

void UHoldComponent_B::Drop()
{
	IThrowableInterface_B* Interface = Cast<IThrowableInterface_B>(GetHoldingItem());
	if (Interface)
		Interface->Execute_Dropped(GetHoldingItem());

	SetHoldingItem(nullptr);
	if (IsValid(OwningCharacter))
		OwningCharacter->SetState(EState::EWalking);
}