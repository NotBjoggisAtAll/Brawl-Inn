// Fill out your copyright notice in the Description page of Project Settings.

#include "Throwable_B.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DestructibleComponent.h"
#include "TimerManager.h"

#include "BrawlInn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HoldComponent_B.h"
#include "Components/ThrowComponent_B.h"
#include "Characters/Character_B.h"
#include "GameFramework/CharacterMovementComponent.h"

AThrowable_B::AThrowable_B()
{
	Mesh->SetVisibility(false);
	Mesh->SetCollisionProfileName("BlockAllDynamicDestructible");
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);

	DestructibleComponent = CreateDefaultSubobject<UDestructibleComponent>("Destructible");
	DestructibleComponent->SetupAttachment(GetRootComponent());
	DestructibleComponent->SetSimulatePhysics(true);
	DestructibleComponent->OnComponentFracture.RemoveAll(this);
}

void AThrowable_B::BeginPlay()
{
	Super::BeginPlay();

	DestructibleComponent->OnComponentFracture.AddDynamic(this, &AThrowable_B::OnComponentFracture);
}

void AThrowable_B::OnComponentFracture(const FVector& HitPoint, const FVector& HitDirection)
{
	if (Mesh)
		Mesh->DestroyComponent();
	if (PickupCapsule)
		PickupCapsule->DestroyComponent();
	SetRootComponent(DestructibleComponent);

	FTimerHandle Handle;

	OnFracture.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(Handle, [&]() {
		Destroy();
		}, 5.f, false);
}

void AThrowable_B::PickedUp_Implementation(ACharacter_B* Player)
{
	Mesh->SetVisibility(true);

	DestructibleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DestructibleComponent->SetSimulatePhysics(false);

	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OwningCharacter = Player;
}

void AThrowable_B::Dropped_Implementation()
{
	FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true);
	DetachFromActor(rules);
	Mesh->SetCollisionProfileName(FName("BlockAllDynamicDestructible"));
	Mesh->SetSimulatePhysics(true);
	PickupCapsule->SetCollisionProfileName(FName("Throwable-AfterThrow"));
}

void AThrowable_B::OnHit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter_B* HitPlayer = Cast<ACharacter_B>(OtherActor);
	if (HitPlayer && !HitPlayer->IsInvulnerable())
	{
		if (HitPlayer->HasShield())
		{
			HitPlayer->RemoveShield();
		}
		else
		{
			HitPlayer->GetCharacterMovement()->AddImpulse(GetVelocity() * ThrowHitStrength);
			HitPlayer->CheckFall(GetVelocity() * ThrowHitStrength);
			UGameplayStatics::ApplyDamage(HitPlayer, DamageAmount, OwningCharacter->GetController(), this, BP_DamageType);
		}
	}
	if (Mesh)
		Mesh->DestroyComponent();
	if (PickupCapsule)
		PickupCapsule->DestroyComponent();
}

void AThrowable_B::Use_Implementation()
{
	FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
	DetachFromActor(rules);
	Mesh->SetCollisionProfileName(FName("BlockAllDynamicDestructible"));
	Mesh->SetSimulatePhysics(true);
	PickupCapsule->SetCollisionProfileName(FName("Throwable-AfterThrow"));

	/// Throw with the help of AimAssist.
	if (IsValid(OwningCharacter))
	{
		FVector TargetLocation = OwningCharacter->GetActorForwardVector();   //Had a crash here, called from notify PlayerThrow_B. Added pointer check at top of function
		OwningCharacter->ThrowComponent->AimAssist(TargetLocation);
		Mesh->AddImpulse(TargetLocation.GetSafeNormal() * OwningCharacter->ThrowComponent->ImpulseSpeed * 0.05f, NAME_None, true);
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DestructibleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		DestructibleComponent->SetSimulatePhysics(true);
	}
	else
	{
		BError("No OwningCharacter for Throwable %s", *GetNameSafe(this))
	}
}
