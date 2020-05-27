// Fill out your copyright notice in the Description page of Project Settings.

#include "Character_B.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstance.h"
#include "Materials/Material.h"
#include "Sound/SoundCue.h"
#include "NiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

#include "BrawlInn.h"
#include "System/GameInstance_B.h"
#include "System/Camera/GameCamera_B.h"
#include "System/GameModes/MainGameMode_B.h"
#include "System/DamageTypes/Barrel_DamageType_B.h"
#include "System/DamageTypes/OutOfWorld_DamageType_B.h"
#include "Components/PunchComponent_B.h"
#include "Components/HoldComponent_B.h"
#include "Components/ThrowComponent_B.h"
#include "Items/Throwable_B.h"
#include "DestructibleComponent.h"

ACharacter_B::ACharacter_B()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = false;

	HoldComponent = CreateDefaultSubobject<UHoldComponent_B>("Hold Component");
	HoldComponent->SetupAttachment(GetMesh());
	ThrowComponent = CreateDefaultSubobject<UThrowComponent_B>("Throw Component");

	PunchComponent = CreateDefaultSubobject<UPunchComponent_B>("PunchComponent");
	PunchComponent->SetupAttachment(GetCapsuleComponent());
	PunchComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->MaxWalkSpeed = 1400.f;
	GetCharacterMovement()->MaxAcceleration = 3000.f;
	GetCharacterMovement()->BrakingFrictionFactor = 1;
	GetCharacterMovement()->GroundFriction = 3;

	GetCapsuleComponent()->SetCapsuleHalfHeight(91.f);
	GetCapsuleComponent()->SetCapsuleRadius(60.f);
	GetCapsuleComponent()->SetCollisionProfileName("Capsule");

	GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
	GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));

	PS_Charge = CreateDefaultSubobject<UNiagaraComponent>("Charge Particle System");

	HoldRotation = FRotator(0, 0, 180);
	HoldLocation = FVector(10, -50, -15);
}

void ACharacter_B::BeginPlay()
{
	Super::BeginPlay();

	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ACharacter_B::OnCapsuleOverlapBegin);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &ACharacter_B::OnCapsuleOverlapEnd);

	NormalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	//caches mesh transform to reset it every time player gets up.
	RelativeMeshTransform = GetMesh()->GetRelativeTransform();

	PS_Charge->Deactivate();

	AGameMode_B* GameMode = Cast<AGameMode_B>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GameMode)
	{
		GameCamera = GameMode->GetGameCamera();
	}
}

void ACharacter_B::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void ACharacter_B::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetState() == EState::EFallen)
	{
		if (bShouldStand)
			StandUp(); 
		else
			SetActorLocation(FMath::VInterpTo(GetActorLocation(), FindMeshLocation(), DeltaTime, 50));
	}
	else if (GetState() != EState::EBeingHeld && GetState() != EState::EPoweredUp && GetCanMove())
	{
		HandleMovement(DeltaTime);
	}

	if (GetState() == EState::EBeingHeld && HoldingCharacter)
	{
		//BWarn("%s RelativeRotation: %s", *GetNameSafe(this), *Relative); // if we can find an actor's relative transform and see if it changes we might be able to see if this solution actually works or not
		SetActorRelativeLocation(Execute_GetHoldLocation(this));
	}
}

void ACharacter_B::SetInputVectorX(const float X)
{
	InputVector.X = X;
}

void ACharacter_B::SetInputVectorY(const float Y)
{
	InputVector.Y = Y;
}

void ACharacter_B::HandleMovement(float DeltaTime)
{
	if (InputVector.IsNearlyZero(0.1))
		return;

	//Normalizes to make sure we dont accelerate faster diagonally, but still want to allow for slower movement.
	if (InputVector.SizeSquared() >= 1.f)
		InputVector.Normalize();

	if (GameCamera)
	{
		AddMovementInput(GameCamera->GetActorForwardVector(), InputVector.X);
		AddMovementInput(GameCamera->GetActorRightVector(), InputVector.Y);
	}

	if (InputVector.SizeSquared() > 0)
	{
		FVector vec = GameCamera->GetActorForwardVector().ToOrientationRotator().RotateVector(InputVector);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), vec.ToOrientationRotator(), DeltaTime, RotationInterpSpeed));
	}
}

void ACharacter_B::SetCanMove(bool Value)
{
	bCanMove = Value;
}

bool ACharacter_B::GetCanMove()
{
	return bCanMove;
}

void ACharacter_B::SetGameCamera(AGameCamera_B* Camera)
{
	GameCamera = Camera;
}

void ACharacter_B::CheckFall(FVector MeshForce)
{
	if (bIsInvulnerable)
		return;
	Fall(MeshForce, FallRecoveryTime, true);
}

void ACharacter_B::Fall(FVector MeshForce, float RecoveryTime, bool bPlaySound)
{
	if (HoldComponent && HoldComponent->IsHolding())
		HoldComponent->Drop();
	SetCanMove(false);
	bCanBeHeld = true;

	SetState(EState::EFallen);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	GetMesh()->AddImpulse(MeshForce, ForceSocketName, false);	//TODO make the bone dynamic instead of a variable

	if (RecoveryTime >= 0 && bIsAlive)
		GetWorld()->GetTimerManager().SetTimer(TH_FallRecoverTimer, this, &ACharacter_B::StandUp, RecoveryTime, false);
}

void ACharacter_B::StandUp()
{
	if (!bIsAlive)
		return;

	bCanBeHeld = false;

	FVector GroundLoc;
	if (!FindMeshGroundLocation(GroundLoc))
	{
		bShouldStand = true;
		return;
	}
	bShouldStand = false;

	SetActorLocation(GroundLoc);

	GetCapsuleComponent()->SetCollisionProfileName("Capsule");

	HoldingCharacter = nullptr; //moved this here so I can check if you hit the character that threw you

	if (IsValid(GetMesh()))
	{
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetCollisionProfileName("CharacterMesh");
		GetMesh()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		GetMesh()->SetRelativeTransform(RelativeMeshTransform);

		//Saves snapshot for blending to animation
		GetMesh()->GetAnimInstance()->SavePoseSnapshot("Ragdoll");
	}
	else
	{
		BError("Mesh invalid for character %s", *GetNameSafe(this));
	}

	if (IsValid(GetMovementComponent()))
		GetMovementComponent()->StopMovementImmediately();

	if (PunchComponent)
	{
		PunchComponent->SetCanDash(true);
		PunchComponent->SetCanPunch(true);
	}

	SetState(EState::EWalking);
	StunAmount = 0;
}

FVector ACharacter_B::FindMeshLocation() const
{
	const FVector MeshLoc = GetMesh()->GetSocketLocation("pelvis");
	FHitResult Hit;
	const bool bDidHit = GetWorld()->LineTraceSingleByChannel(Hit, MeshLoc + FVector(0, 0, 0), MeshLoc + FVector(0, 0, -200), ECollisionChannel::ECC_Visibility);

	if (bDidHit)
	{
		return (Hit.Location - RelativeMeshTransform.GetLocation());
	}
	return (MeshLoc - RelativeMeshTransform.GetLocation());
}

bool ACharacter_B::FindMeshGroundLocation(FVector& OutGroundLocation) const
{
	const FVector MeshLoc = GetMesh()->GetSocketLocation("pelvis");

	FHitResult Hit;
	const bool bDidHit = GetWorld()->LineTraceSingleByChannel(Hit, MeshLoc + FVector(0, 0, 0), MeshLoc + FVector(0, 0, -200), ECollisionChannel::ECC_Visibility);

	if (bDidHit)
	{
		OutGroundLocation = (Hit.Location - RelativeMeshTransform.GetLocation());
		return true;
	}

	OutGroundLocation = (MeshLoc - RelativeMeshTransform.GetLocation());
	return false;
}

bool ACharacter_B::IsFacingUp()
{
	FRotator rot = GetMesh()->GetSocketRotation("pelvis");
	FVector right = -rot.RotateVector(FVector::RightVector);
	GetMesh()->GetSocketByName("pelvis");

	float dot = FVector::DotProduct(right, FVector::UpVector);
	if (dot > 0)
		return true;

	return false;
}

void ACharacter_B::PickedUp_Implementation(ACharacter_B* Player)
{
	HoldingCharacter = Player;
	GetMovementComponent()->StopMovementImmediately();
	SetState(EState::EBeingHeld);
	GetWorld()->GetTimerManager().ClearTimer(TH_FallRecoverTimer);

	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepWorldTransform);
	GetCapsuleComponent()->SetCollisionProfileName("Capsule");

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionProfileName("CharacterMesh");

	GetCharacterMovement()->StopActiveMovement();
	GetMesh()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetMesh()->SetRelativeTransform(RelativeMeshTransform);
}

void ACharacter_B::Dropped_Implementation()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Fall(FVector::ZeroVector, FallRecoveryTime, true);
	//	GetMesh()->SetSimulatePhysics(true); //should be unnecceCharacterMeshsary
	HoldingCharacter = nullptr;
	SetActorRotation(FRotator(0, 0, 0));
}

void ACharacter_B::Use_Implementation()
{
	FVector TargetLocation = HoldingCharacter->GetActorForwardVector();
	//AddActorLocalOffset(TargetLocation * 250);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (IsValid(HoldingCharacter) && IsValid(HoldingCharacter->ThrowComponent))
	{
		HoldingCharacter->ThrowComponent->AimAssist(TargetLocation);
		float ImpulseStrength = 0.f;
		IThrowableInterface_B* Interface = Cast<IThrowableInterface_B>(this);
		if (Interface)
		{
			ImpulseStrength = Interface->Execute_GetThrowStrength(this);
		}
		GetCharacterMovement()->StopMovementImmediately();
		GetMesh()->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
#if WITH_EDITOR
		if (HoldingCharacter->ThrowComponent->bDrawAimAssistDebug)
		{
			FVector start = HoldingCharacter->GetMesh()->GetComponentLocation();
			DrawDebugLine(GetWorld(), start, start + TargetLocation * ImpulseStrength, FColor::Blue, false, 5.f);
		}
#endif
		Fall(TargetLocation * ImpulseStrength, ThrowRecoveryTime, true);
	}

	GetWorld()->GetTimerManager().SetTimer(TH_FallCollisionTimer, this, &ACharacter_B::SetCapsuleCollisionProfileNameThrown, TimeBeforeThrowCollision, false);

	SetActorRotation(FRotator(0, 0, 0));
}

void ACharacter_B::SetCapsuleCollisionProfileNameThrown()
{
	GetCapsuleComponent()->SetCollisionProfileName(FName("Capsule-Thrown"));
}

bool ACharacter_B::IsHeld_Implementation() const
{
	if (HoldingCharacter)
		return true;
	return false;
}

bool ACharacter_B::CanBeHeld_Implementation() const
{
	return bCanBeHeld && !bIsInvulnerable;
}

float ACharacter_B::GetThrowStrength_Implementation() const
{
	return ThrowStrength;
}

float ACharacter_B::GetMovementSpeedWhenHeld_Implementation() const
{
	return MovementSpeedWhenHeld;
}

float ACharacter_B::GetPickupWeight_Implementation() const
{
	return PickupWeight;
}

void ACharacter_B::AddStun(const int Strength)
{
	if (StunAmount == PunchesToStun - 1)
	{
		StunAmount += Strength;
		return;
	}
	StunAmount += Strength;
}

int ACharacter_B::GetPunchesToStun()
{
	return PunchesToStun;
}

int ACharacter_B::GetStunAmount()
{
	return StunAmount;
}

void ACharacter_B::SetSpecialMaterial(UMaterialInstance* Material)
{
	if (Material)
		GetMesh()->SetMaterial(SpecialMaterialIndex, Material);
}

void ACharacter_B::RemoveSpecialMaterial()
{
	if (InvisibleMat)
		GetMesh()->SetMaterial(SpecialMaterialIndex, InvisibleMat);
}

void ACharacter_B::MakeInvulnerable(float ITime, bool bShowInvulnerabilityEffect)
{
	bIsInvulnerable = true;

	if (bShowInvulnerabilityEffect && InvulnerableMat)
		SetSpecialMaterial(InvulnerableMat);
	if (ITime > 0)
		GetWorld()->GetTimerManager().SetTimer(TH_InvincibilityTimer, this, &ACharacter_B::MakeVulnerable, ITime, false);
}

void ACharacter_B::MakeVulnerable()
{
	bIsInvulnerable = false;

	if (!bHasShield)
		RemoveSpecialMaterial();
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
	if (State == EState::EPoweredUp)
	{
		AMainGameMode_B* GameMode = Cast<AMainGameMode_B>(UGameplayStatics::GetGameMode(GetWorld()));
		if (GameMode)
		{
			GameMode->ResetMusic();
		}
	}

	State = s;
	//Entering state
	switch (State)
	{
	case EState::EWalking:
		GetCharacterMovement()->MaxWalkSpeed = NormalMaxWalkSpeed;
		break;
	case EState::EHolding:
		IThrowableInterface_B* Interface = Cast<IThrowableInterface_B>(HoldComponent->GetHoldingItem());
		if (Interface)
			GetCharacterMovement()->MaxWalkSpeed = Interface->Execute_GetMovementSpeedWhenHeld(HoldComponent->GetHoldingItem());
		break;
	}
}

EState ACharacter_B::GetState() const
{
	return State;
}

void ACharacter_B::Die()
{	
	AActor* Item = HoldComponent->GetHoldingItem();
	if (Item)
	{
		ACharacter_B* C = Cast<ACharacter_B>(Item);
		if(C)
			C->Die();

		AThrowable_B* T = Cast<AThrowable_B>(Item);
		if (T)
		{
			T->GetDestructibleComponent()->SetSimulatePhysics(true);
			T->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			//T->OnComponentFracture(T->GetActorLocation(), FVector(0.f, 0.f, -1.f));
		}
	}
	
	Fall(FVector::ZeroVector, -1, false);
	bIsAlive = false;
}

void ACharacter_B::TryStartCharging()
{
	if (GetState() != EState::EWalking)
		return;

	if (!PunchComponent) { BError("No Punch Component for player %s", *GetNameSafe(this)); return; }

	SetIsCharging(true);
}

UNiagaraComponent* ACharacter_B::GetChargeParticle() const
{
	return PS_Charge;
}

float ACharacter_B::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (DamageEvent.DamageTypeClass.GetDefaultObject()->IsA(UBarrel_DamageType_B::StaticClass()))
	{
		//Does a lot of ApplyDamageMomentum manually, so we can do it to the mesh instead of movementcomponent
		UDamageType const* const DmgTypeCDO = DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>();
		float const ImpulseScale = DmgTypeCDO->DamageImpulse;
		FHitResult Hit;
		FVector ImpulseDir;
		DamageEvent.GetBestHitInfo(this, nullptr, Hit, ImpulseDir);
		bool const bMassIndependentImpulse = !DmgTypeCDO->bScaleMomentumByMass;

		CheckFall(ImpulseDir * ImpulseScale);
	}

	if (GetState() == EState::EHolding)
	{
		//either turn off collision so it doesnt fall or make the item fly into the air
		HoldComponent->Drop();
	}

	//Play sound if hurt, but not when falling out of the map
	if (HurtSound && !DamageEvent.DamageTypeClass.GetDefaultObject()->IsA(UOutOfWorld_DamageType_B::StaticClass()))
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			HurtSound,
			GetActorLocation(),
			1.f
		);
	}
	return DamageAmount;
}

bool ACharacter_B::IsAlive() const
{
	return bIsAlive;
}

void ACharacter_B::OnCapsuleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter_B* HitCharacter = Cast<ACharacter_B>(OtherActor);
	UCapsuleComponent* HitComponent = Cast<UCapsuleComponent>(OtherComp);
	if (HitComponent && HitCharacter && HitCharacter != HoldingCharacter && !HitCharacter->IsInvulnerable() && (GetCapsuleComponent()->GetCollisionProfileName() == "Capsule-Thrown"))
	{
		if (HitCharacter->HasShield())
		{
			HitCharacter->RemoveShield();
		}
		else
		{
			HitCharacter->GetCharacterMovement()->AddImpulse(GetVelocity());
			HitCharacter->CheckFall(GetVelocity());
			if (IsValid(HitCharacter) && IsValid(HitCharacter->GetController()))
				UGameplayStatics::ApplyDamage(HitCharacter, 1, HitCharacter->GetController(), this, UDamageType::StaticClass());
		}
	}
}

void ACharacter_B::OnCapsuleOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

const FRotator ACharacter_B::GetHoldRotation_Implementation()
{
	return HoldRotation;
}

const FVector ACharacter_B::GetHoldLocation_Implementation()
{
	return HoldLocation;
}

EChargeLevel ACharacter_B::GetChargeLevel()
{
	return ChargeLevel;
}

void ACharacter_B::SetChargeLevel(EChargeLevel chargeLevel)
{

	ChargeLevel = chargeLevel;
	bool ShouldPlaySound = true;
	float SoundPitch = 1.0f;

	switch (ChargeLevel)
	{
	case EChargeLevel::EChargeLevel1:
		ShouldPlaySound = false;
		SoundPitch = 0.8f;
		if (IsCharging())
		{
			GetCharacterMovement()->MaxWalkSpeed = Charge1MoveSpeed;
			GetCharacterMovement()->Velocity = GetVelocity().GetClampedToMaxSize(Charge2MoveSpeed);
		}
		break;
	case EChargeLevel::EChargeLevel2:
		GetCharacterMovement()->MaxWalkSpeed = Charge2MoveSpeed;
		GetCharacterMovement()->Velocity = GetVelocity().GetClampedToMaxSize(Charge2MoveSpeed);
		SoundPitch = 1.0f;
		break;
	case EChargeLevel::EChargeLevel3:
		GetCharacterMovement()->MaxWalkSpeed = Charge3MoveSpeed;
		GetCharacterMovement()->Velocity = GetVelocity().GetClampedToMaxSize(Charge3MoveSpeed);
		SoundPitch = 1.2f;
		break;
	default:
		GetCharacterMovement()->MaxWalkSpeed = NormalMaxWalkSpeed;
		ShouldPlaySound = false;
		break;
	}

	if (ShouldPlaySound && ChargeLevelSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ChargeLevelSound,
			GetActorLocation(),
			1.f,
			SoundPitch
		);
	}
}

void ACharacter_B::SetIsCharging(bool Value)
{
	bIsCharging = Value;
}

bool ACharacter_B::IsCharging() const
{
	return bIsCharging;
}