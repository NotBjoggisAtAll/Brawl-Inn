// Fill out your copyright notice in the Description page of Project Settings.

#include "PunchComponent_B.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Sound/SoundCue.h"

#include "BrawlInn.h"
#include "System/DataTable_B.h"
#include "System/GameInstance_B.h"
#include "System/Camera/GameCamera_B.h"
#include "Components/ThrowComponent_B.h"
#include "Characters/Player/GamePlayerController_B.h"
#include "Characters/Character_B.h"
#include "Characters/Player/PlayerCharacter_B.h"

UPunchComponent_B::UPunchComponent_B()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPunchComponent_B::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ACharacter_B>(GetOwner());

	OnComponentBeginOverlap.AddDynamic(this, &UPunchComponent_B::OnOverlapBegin);

	UGameInstance_B* GameInstance = Cast<UGameInstance_B>(OwningCharacter->GetGameInstance());
	if (!GameInstance) { BError("%s can't find the GameInstance_B! ABORT", *GetNameSafe(this)); return; }

	if (GameInstance->ShouldUseSpreadSheets())
	{
		UDataTable_B* ScoreTable = NewObject<UDataTable_B>();
		ScoreTable->LoadCSVFile(FScoreTable::StaticStruct(), "DefaultScoreValues.csv");
		Level1ScoreValue = ScoreTable->GetRow<FScoreTable>("PunchLevel1ScoreValue")->Value;
		Level2ScoreValue = ScoreTable->GetRow<FScoreTable>("PunchLevel2ScoreValue")->Value;
		Level3ScoreValue = ScoreTable->GetRow<FScoreTable>("PunchLevel3ScoreValue")->Value;
		ScoreTable->ConditionalBeginDestroy();
	}
}

void UPunchComponent_B::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void UPunchComponent_B::PunchStart()
{
	if (!OwningCharacter) { BError("%s No OwningCharacter found for PunchComponent!", *GetNameSafe(this)); return; }
	OwningCharacter->SetIsCharging(false);
	SetIsPunching(true);
	OwningCharacter->MakeVulnerable();

	SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	TArray<UPrimitiveComponent*> OurOverlappingComponents;
	GetOverlappingComponents(OurOverlappingComponents);
	for (auto& comp : OurOverlappingComponents)
	{
		if (IsValid(comp))
			CheckPunchHit(comp->GetOwner(), comp);
	}

	if (PunchSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			PunchSound,
			GetComponentLocation(),
			1.f
		);
	}

	PunchDash();
}

void UPunchComponent_B::CheckPunchHit(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	if (HitActors.Find(OtherActor) != INDEX_NONE)
		return;

	ACharacter_B* OtherCharacter = Cast<ACharacter_B>(OtherActor);
	UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(OtherComp);

	if (OtherActor != OwningCharacter)
	{
		if (IsValid(OtherCharacter) && (OtherCharacter->GetState() != EState::EFallen))
		{
			if (IsValid(Capsule))
			{
				HitActors.Add(OtherActor);
				PunchHit(OtherCharacter);
			}
		}
		else
		{
			HitActors.Add(OtherActor);
			PunchHit(OtherComp);
		}
	}
}

void UPunchComponent_B::PunchDash()
{
	VelocityBeforeDash = OwningCharacter->GetCharacterMovement()->Velocity;

	float PunchDashSpeed = 0.f;
	switch (OwningCharacter->GetChargeLevel())
	{
	case EChargeLevel::EChargeLevel1: PunchDashSpeed = Charge1PunchDashSpeed;
		break;
	case EChargeLevel::EChargeLevel2: PunchDashSpeed = Charge2PunchDashSpeed;
		break;
	case EChargeLevel::EChargeLevel3: PunchDashSpeed = Charge3PunchDashSpeed;
		break;
	default:
		break;
	}

	OwningCharacter->GetCharacterMovement()->Velocity = FVector(OwningCharacter->GetActorForwardVector() * PunchDashSpeed);

}

void UPunchComponent_B::Dash()
{
	if (!OwningCharacter) { BError("No OwningCnaracter for PunchComponent!"); return; }

	if (bIsDashing || !OwningCharacter)
		return;
	bIsDashing = true;

	OwningCharacter->GetCapsuleComponent()->SetCollisionProfileName("Capsule-Dash");

	OwningCharacter->MakeInvulnerable(0.3f, false);
	VelocityBeforeDash = OwningCharacter->GetCharacterMovement()->Velocity;

	FVector NormInput = OwningCharacter->InputVector.GetSafeNormal();
	if (NormInput.IsNearlyZero())
	{
		//think setting velocity is more predictable and therefore better than adding force here.
		OwningCharacter->GetCharacterMovement()->Velocity = FVector(OwningCharacter->GetActorForwardVector() * DashSpeed);
	}
	else
	{
		NormInput = OwningCharacter->GameCamera->GetActorForwardVector().ToOrientationRotator().RotateVector(NormInput);
		OwningCharacter->GetCharacterMovement()->Velocity = FVector(NormInput * DashSpeed);
		OwningCharacter->SetActorRotation(OwningCharacter->InputVector.Rotation());
	}

	GetWorld()->GetTimerManager().SetTimer(
		TH_DashAgainHandle, this, &UPunchComponent_B::SetIsDashingToFalse,
		DashCooldown,
		false);

	GetWorld()->GetTimerManager().SetTimer(
		TH_DashDoneHandle, this, &UPunchComponent_B::DashDone,
		DashTime,
		false);
}

void UPunchComponent_B::SetIsDashingToFalse()
{
	bIsDashing = false;
}

void UPunchComponent_B::DashDone()
{
	OwningCharacter->GetCapsuleComponent()->SetCollisionProfileName("Capsule");
	OwningCharacter->GetCharacterMovement()->Velocity = OwningCharacter->GetCharacterMovement()->Velocity * PostDashRemainingVelocityPercentage;
}

void UPunchComponent_B::PunchEnd()
{
	if (!OwningCharacter) { BError("%s No OwningCharacter found for PunchComponent!", *GetNameSafe(this)); return; }

	OwningCharacter->SetIsCharging(false);
	OwningCharacter->SetChargeLevel(EChargeLevel::ENotCharging);

	HitActors.Empty();
	if (!GetIsPunching()) { return; }
	SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OwningCharacter->GetCharacterMovement()->MaxWalkSpeed = OwningCharacter->NormalMaxWalkSpeed;
	OwningCharacter->GetCharacterMovement()->Velocity = OwningCharacter->GetCharacterMovement()->Velocity.GetClampedToMaxSize(OwningCharacter->NormalMaxWalkSpeed * PostDashRemainingVelocityPercentage);

	SetIsPunching(false);

	GetWorld()->GetTimerManager().SetTimer(
		TH_PunchAgainHandle, this, &UPunchComponent_B::PunchAgain,
		PunchWaitingTime,
		false);
	VelocityBeforeDash = FVector::ZeroVector;
}

void UPunchComponent_B::PunchAgain()
{
	SetCanPunch(true);

	AGamePlayerController_B* PlayerController = OwningCharacter->GetController<AGamePlayerController_B>();
	if (PlayerController)
	{
		if (PlayerController->IsPunchChargeInputHeld())
			PlayerController->TryStartPunchCharge();
	}
}

void UPunchComponent_B::PunchHit(ACharacter_B* OtherPlayer)
{
	if (!OtherPlayer) { BError("%s No OtherPlayer found!", *GetNameSafe(this)); return; }
	if (!OtherPlayer->PunchComponent) { BError("No PunchComponent found for OtherPlayer %s!", *GetNameSafe(OtherPlayer)); return; }
	if (!OwningCharacter) { BError("No OwningCharacter found for PunchComponent %s!", *GetNameSafe(this)); return; }

	if (!OtherPlayer->bIsInvulnerable || !OtherPlayer->bHasShield)
	{
		OtherPlayer->PunchComponent->GetPunched(CalculatePunchStrength(), OwningCharacter);
		UGameplayStatics::ApplyDamage(OtherPlayer, CalculatePunchDamage(), OwningCharacter->GetController(), OwningCharacter, BP_DamageType);
		OwningCharacter->StunStrength = 1; // This ends the punch powerup after you hit a punch. If we want to end the effect after every punch we need to move this to PunchEnd
		OwningCharacter->GetMovementComponent()->Velocity *= PunchHitVelocityDamper;

		//If the character hit is a player increase the score
		if (OtherPlayer->IsA(APlayerCharacter_B::StaticClass()))
			OnHitPlayerPunch_D.Broadcast();
	}
	else
	{
		OwningCharacter->GetCharacterMovement()->Velocity *= -0.5;
	}
}

void UPunchComponent_B::PunchHit(UPrimitiveComponent* OtherComp)
{
	if (!OtherComp) { BError("%s No OtherComp found!", *GetNameSafe(this)); return; }
	if (!OwningCharacter) { BError("No OwningCharacter found for PunchComponent %s!", *GetNameSafe(this)); return; }
	if (OtherComp->IsSimulatingPhysics())
	{
		OwningCharacter->GetMovementComponent()->Velocity *= PunchHitVelocityDamper;
		OtherComp->AddImpulse(CalculatePunchStrength());
	}
}

void UPunchComponent_B::GetPunched(FVector InPunchStrength, ACharacter_B* PlayerThatPunched)
{
	if (!OwningCharacter) { BError("No OwningCharacter found for PunchComponent %s!", *GetNameSafe(this)); return; }

	if (OwningCharacter->ThrowComponent && OwningCharacter->ThrowComponent->IsDrinking())
		OwningCharacter->ThrowComponent->CancelDrinking();

	OnGetPunched_D.Broadcast(PlayerThatPunched);

	PunchEnd(); //In case the player got hit while punching, mostly to make sure punch collisions get turned off
	if (!OwningCharacter->IsInvulnerable())
	{

		OwningCharacter->RemoveShield();
		FVector PunchDirection = InPunchStrength.GetSafeNormal();

		switch (PlayerThatPunched->GetChargeLevel())
		{
		case EChargeLevel::EChargeLevel1:
			OwningCharacter->AddStun(PlayerThatPunched->StunStrength);
			OwningCharacter->GetCharacterMovement()->AddImpulse(PunchDirection * PlayerThatPunched->PunchComponent->Level1PunchPushStrength);
			break;
		case EChargeLevel::EChargeLevel2:
			OwningCharacter->AddStun(PlayerThatPunched->StunStrength * 2);
			OwningCharacter->GetCharacterMovement()->AddImpulse(PunchDirection * PlayerThatPunched->PunchComponent->Level2PunchPushStrength);
			break;
		case EChargeLevel::EChargeLevel3:
			OwningCharacter->CheckFall(InPunchStrength);
			return;
		default:
			BError("Character %s got hit with an invalid charge level from character %s", *GetNameSafe(OwningCharacter), *GetNameSafe(PlayerThatPunched));
			break;
		}

		if (OwningCharacter->StunAmount >= OwningCharacter->PunchesToStun)
		{
			OwningCharacter->CheckFall(InPunchStrength);
		}
	}
}

FVector UPunchComponent_B::CalculatePunchStrength()
{
	if (!OwningCharacter) { BError("No OwningCharacter found for PunchComponent %s!", *GetNameSafe(this)); return FVector(); }

	switch (OwningCharacter->GetChargeLevel())
	{
	case EChargeLevel::EChargeLevel1:
		return OwningCharacter->GetActorForwardVector() * Level1PunchStrength;
	case EChargeLevel::EChargeLevel2:
		return OwningCharacter->GetActorForwardVector() * Level2PunchStrength;
	case EChargeLevel::EChargeLevel3:
		return OwningCharacter->GetActorForwardVector() * Level3PunchStrength;
	default:
		return FVector::ZeroVector;
	}
}

int UPunchComponent_B::CalculatePunchDamage() const
{
	if (!OwningCharacter) { BError("No OwningCharactersss found for PunchComponent %s!", *GetNameSafe(this)); return 0; }

	switch (OwningCharacter->ChargeLevel)
	{
	case EChargeLevel::EChargeLevel1:
		return Level1ScoreValue;
	case EChargeLevel::EChargeLevel2:
		return Level2ScoreValue;
	case EChargeLevel::EChargeLevel3:
		return Level3ScoreValue;
	default:
		return Level1ScoreValue;
	}
}

bool UPunchComponent_B::GetIsPunching()
{
	return bIsPunching;
}

void UPunchComponent_B::SetIsPunching(bool Value)
{
	bIsPunching = Value;
}

bool UPunchComponent_B::GetCanPunch()
{
	return bCanPunch;
}

void UPunchComponent_B::SetCanPunch(bool Value)
{
	bCanPunch = Value;
}

bool UPunchComponent_B::GetCanDash()
{
	return bCanDash;
}

void UPunchComponent_B::SetCanDash(bool Value)
{
	bCanDash = Value;
}

bool UPunchComponent_B::GetIsDashing()
{
	return bIsDashing;
}

void UPunchComponent_B::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	CheckPunchHit(OtherActor, OtherComp);
}
