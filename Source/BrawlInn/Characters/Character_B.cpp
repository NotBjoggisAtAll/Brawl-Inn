// Fill out your copyright notice in the Description page of Project Settings.

#include "Character_B.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Sound/SoundCue.h"
#include "NiagaraComponent.h"
#include "Animation/AnimInstance.h"

#include "BrawlInn.h"
#include "System/GameInstance_B.h"
#include "Components/PunchComponent_B.h"
#include "Components/HoldComponent_B.h"
#include "Components/ThrowComponent_B.h"
#include "System/DamageTypes/Barrel_DamageType_B.h"

ACharacter_B::ACharacter_B()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	HoldComponent = CreateDefaultSubobject<UHoldComponent_B>("Hold Component");
	HoldComponent->SetupAttachment(GetMesh());
	ThrowComponent = CreateDefaultSubobject<UThrowComponent_B>("Throw Component");

	PunchComponent = CreateDefaultSubobject<UPunchComponent_B>("PunchComponent");
	PunchComponent->SetupAttachment(GetMesh(), "PunchCollisionHere");
	PunchComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->MaxWalkSpeed = 2000;
	GetCharacterMovement()->MaxAcceleration = 800;
	GetCharacterMovement()->BrakingFrictionFactor = 1;
	GetCharacterMovement()->GroundFriction = 3;

	PS_Stun = CreateDefaultSubobject<UNiagaraComponent>("Stun Particle System");
	PS_Stun->SetupAttachment(GetMesh());

	PS_Charge = CreateDefaultSubobject<UNiagaraComponent>("Charge Particle System");
	PS_Charge->SetupAttachment(GetMesh(), "PunchCollisionHere");
}

void ACharacter_B::BeginPlay()
{
	Super::BeginPlay();

	NormalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	//caches mesh transform to reset it every time player gets up.
	RelativeMeshTransform = GetMesh()->GetRelativeTransform();

	PS_Stun->Deactivate();
	PS_Charge->Deactivate();

	MakeInvulnerable(1.0f);
}

void ACharacter_B::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetState() == EState::EFallen)
	{
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), FindMeshLocation(), DeltaTime, 50));
	}
	else if (GetState() != EState::EBeingHeld)
	{
		if (!PunchComponent->bIsPunching && !bIsInvulnerable)
			CheckFall(DeltaTime);

		if (!(GetState() == EState::EStunned))
		{
			HandleMovement(DeltaTime);
		}
	}
}

void ACharacter_B::HandleMovement(float DeltaTime)
{
	//Normalizes to make sure we dont accelerate faster diagonally, but still want to allow for slower movement.
	if (InputVector.SizeSquared() >= 1.f)
		InputVector.Normalize();
	GetMovementComponent()->AddInputVector(InputVector);

	if (InputVector.SizeSquared() > 0)
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), InputVector.ToOrientationRotator(), DeltaTime, 10.f));
}

void ACharacter_B::CheckFall(float DeltaTime)
{
	float Speed = GetMovementComponent()->Velocity.Size();
	if (Speed >= NormalMaxWalkSpeed * FallLimitMultiplier)
	{
		MakeInvulnerable(FallRecoveryTime, false);
		Fall(FallRecoveryTime);
	}
}

void ACharacter_B::Fall(float RecoveryTime)
{
	if (GetCharacterMovement()->IsFalling())
		return;

	if (HoldComponent && HoldComponent->IsHolding())
		HoldComponent->Drop();

	SetState(EState::EFallen);

	GetMesh()->SetGenerateOverlapEvents(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FVector Force = GetMovementComponent()->Velocity;
	GetMesh()->AddImpulse(Force, ForceSocketName, true);	//TODO make the bone dynamic instead of a variable

	if (RecoveryTime >= 0 && bIsAlive)
		GetWorld()->GetTimerManager().SetTimer(TH_FallRecoverTimer, this, &ACharacter_B::StandUp, RecoveryTime, false);
}

void ACharacter_B::StandUp()
{
	if (GetCharacterMovement()->IsFalling())
		return;

	//Saves snapshot for blending to animation
	GetMesh()->GetAnimInstance()->SavePoseSnapshot("Ragdoll");

	GetMovementComponent()->StopMovementImmediately();

	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetGenerateOverlapEvents(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MakeInvulnerable(InvulnerabilityTime);
	SetState(EState::EWalking);
}

FVector ACharacter_B::FindMeshLocation()
{
	FVector MeshLoc = GetMesh()->GetSocketLocation("pelvis");

	FHitResult Hit;
	bool bDidHit = GetWorld()->LineTraceSingleByChannel(Hit, MeshLoc + FVector(0, 0, 0), MeshLoc + FVector(0, 0, -1000), ECollisionChannel::ECC_Visibility);

	if (bDidHit)
		return (Hit.Location - RelativeMeshTransform.GetLocation());
	else
		return (MeshLoc - RelativeMeshTransform.GetLocation());
}

void ACharacter_B::PickedUp_Implementation(ACharacter_B* Player)
{
	HoldingCharacter = Player;
	GetMovementComponent()->StopMovementImmediately();
	SetState(EState::EBeingHeld);
	GetWorld()->GetTimerManager().ClearTimer(TH_FallRecoverTimer);
	GetMesh()->SetSimulatePhysics(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetGenerateOverlapEvents(false);

	GetCharacterMovement()->StopActiveMovement();
	GetCharacterMovement()->StopMovementImmediately();
	SetActorRotation(FRotator(-90, 0, 90));
}

void ACharacter_B::Dropped_Implementation()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	Fall(FallRecoveryTime);
	GetMesh()->SetSimulatePhysics(true);
	HoldingCharacter = nullptr;
	SetActorRotation(FRotator(0, 0, 0));
}

void ACharacter_B::Use_Implementation()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	Fall(FallRecoveryTime);
	GetMesh()->SetSimulatePhysics(true);

	/// Throw with the help of AimAssist.
	FVector TargetLocation = HoldingCharacter->GetActorForwardVector();
	HoldingCharacter->ThrowComponent->AimAssist(TargetLocation);
	GetMesh()->AddImpulse(TargetLocation * HoldingCharacter->ThrowComponent->ImpulseSpeed, ForceSocketName, true);

	HoldingCharacter = nullptr;

	SetActorRotation(FRotator(0, 0, 0));
}

bool ACharacter_B::IsHeld_Implementation() const
{
	if (HoldingCharacter)
		return true;
	return false;
}

void ACharacter_B::AddStun(int Strength)
{
	if (GetState() == EState::EStunned)
		return;

	StunAmount += Strength;
	if (StunAmount >= PunchesToStun)
		SetState(EState::EStunned);

	GetWorld()->GetTimerManager().SetTimer(TH_StunTimer, this, &ACharacter_B::RemoveStun, StunTime, false);
}

void ACharacter_B::RemoveStun()
{
	if (GetState() == EState::EStunned)
		SetState(EState::EWalking);

	StunAmount = 0;
}

void ACharacter_B::MakeInvulnerable(float ITime, bool bShowInvulnerabilityEffect)
{
	bIsInvulnerable = true;

	if (bShowInvulnerabilityEffect && InvulnerableMat)
		GetMesh()->SetMaterial(SpecialMaterialIndex, InvulnerableMat);
	if (ITime > 0)
		GetWorld()->GetTimerManager().SetTimer(TH_InvincibilityTimer, this, &ACharacter_B::MakeVulnerable, ITime, false);
}

void ACharacter_B::MakeVulnerable()
{
	bIsInvulnerable = false;

	if (InvulnerableMat && !bHasShield)
		GetMesh()->SetMaterial(SpecialMaterialIndex, InvisibleMat);
}

bool ACharacter_B::IsInvulnerable()
{
	return bIsInvulnerable;
}

void ACharacter_B::ApplyShield()
{
	bHasShield = true;

	if (ShieldMat)
		GetMesh()->SetMaterial(SpecialMaterialIndex, ShieldMat);
}

void ACharacter_B::RemoveShield()
{
	bHasShield = false;

	if (ShieldMat)
		GetMesh()->SetMaterial(SpecialMaterialIndex, InvisibleMat);
}

bool ACharacter_B::HasShield() const
{
	return bHasShield;
}

void ACharacter_B::SetState(EState s)
{
	//on leaving state
	switch (State)
	{
	case EState::EStunned:
		if (IsValid(PS_Stun))
			PS_Stun->DeactivateImmediate();
		break;
	}

	State = s;
	//Entering state
	switch (State)
	{
	case EState::EWalking:
		GetCharacterMovement()->MaxWalkSpeed = NormalMaxWalkSpeed;
		break;
	case EState::EHolding:
		GetCharacterMovement()->MaxWalkSpeed = GetCharacterMovement()->MaxCustomMovementSpeed;
		break;
	case EState::EStunned:
		if (IsValid(PS_Stun))
			PS_Stun->Activate();
		break;
	}
}

EState ACharacter_B::GetState() const
{
	return State;
}

void ACharacter_B::TryPunch()
{
	if (GetState() != EState::EWalking)
		return;

	if (!PunchComponent) { BError("No Punch Component for player %s", *GetNameSafe(this)); return; }

	PunchComponent->bIsPunching = true;
}
UNiagaraComponent* ACharacter_B::GetChargeParticle() const
{
	return PS_Charge;
}

float ACharacter_B::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (DamageEvent.DamageTypeClass.GetDefaultObject()->IsA(UBarrel_DamageType_B::StaticClass()))
		ApplyDamageMomentum(DamageAmount, DamageEvent, nullptr, DamageCauser);

	if (HurtSound)
	{
		float volume = 1.f;
		UGameInstance_B* GameInstance = Cast<UGameInstance_B>(UGameplayStatics::GetGameInstance(GetWorld()));
		if (GameInstance)
			volume *= GameInstance->MasterVolume * GameInstance->SfxVolume;

		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			HurtSound,
			GetActorLocation(),
			volume
		);
	}
	return DamageAmount;
}