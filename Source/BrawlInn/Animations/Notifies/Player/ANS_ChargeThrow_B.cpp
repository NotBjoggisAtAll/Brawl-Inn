// Fill out your copyright notice in the Description page of Project Settings.


#include "ANS_ChargeThrow_B.h"
#include "Components/SkeletalMeshComponent.h"

#include "Characters/Player/PlayerCharacter_B.h"
#include "Components/ThrowComponent_B.h"
#include "Animation/AnimSequence.h"
#include "BrawlInn.h"

void UANS_ChargeThrow_B::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	AnimLength = TotalDuration/ Animation->RateScale;
	CurrentTime = 0.f;
	Player = Cast<APlayerCharacter_B>(MeshComp->GetOwner());
}

void UANS_ChargeThrow_B::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	if (IsValid(Player) && IsValid(Player->ThrowComponent))
	{
		CurrentTime += FrameDeltaTime;
		Player->ThrowComponent->ImpulseTimer = (CurrentTime/ AnimLength);
	}
}

void UANS_ChargeThrow_B::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{

}
