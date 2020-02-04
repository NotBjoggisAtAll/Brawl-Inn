// Fill out your copyright notice in the Description page of Project Settings.

#include "Useable_B.h"
#include "BrawlInn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"

AUseable_B::AUseable_B()
{

	NiagaraSystemComponent = CreateDefaultSubobject<UNiagaraComponent>("Particle System");
	NiagaraSystemComponent->SetupAttachment(RootComponent);

	Mesh->SetSimulatePhysics(false);

	PickupCapsule->SetRelativeLocation(FVector(0, 37, 0));
	PickupCapsule->SetCapsuleHalfHeight(60);
	PickupCapsule->SetCapsuleRadius(60);

	DrinkMesh = CreateDefaultSubobject<UStaticMeshComponent>("Drink Mesh");
	DrinkMesh->SetupAttachment(RootComponent);
	DrinkMesh->SetRelativeLocation(FVector(2.3f, 36.5f, 38));
	DrinkMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.05f));
	DrinkMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void AUseable_B::PickedUp_Implementation(ACharacter_B* Player)
{
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OwningPlayer = Player;
}

void AUseable_B::Dropped_Implementation()
{
	FDetachmentTransformRules rules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true);
	DetachFromActor(rules);
	Mesh->SetCollisionProfileName(FName("BlockAllDynamic"));
	Mesh->SetSimulatePhysics(true);
}

void AUseable_B::Use_Implementation()
{
	if (Duration > 0)
		GetWorld()->GetTimerManager().SetTimer(TH_DrinkHandle, this, &AUseable_B::ResetBoost, Duration, false);

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), DrinkSound, GetActorLocation());
	DrinkMesh->DestroyComponent();
	PickupCapsule->DestroyComponent();
	NiagaraSystemComponent->DestroyComponent();

	Execute_Dropped(this);
}

void AUseable_B::ResetBoost()
{
}

void AUseable_B::FellOutOfWorld(const UDamageType& dmgType)
{
	if (!GetWorld()->GetTimerManager().TimerExists(TH_DrinkHandle))
	{
		Super::FellOutOfWorld(dmgType);
	}
}