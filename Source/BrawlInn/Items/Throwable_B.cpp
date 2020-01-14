// Fill out your copyright notice in the Description page of Project Settings.


#include "Throwable_B.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

#include "BrawlInn.h"
#include "Player/PlayerCharacter_B.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AThrowable_B::AThrowable_B()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	PickupSphere->SetupAttachment(RootComponent);
	PickupSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
}

void AThrowable_B::BeginPlay()
{

	Super::BeginPlay();

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AThrowable_B::OnThrowOverlapBegin);

}

void AThrowable_B::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		BWarn("Spawning particles from %s system.", *GetNameSafe(ParticleSystem));
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ParticleSystem, GetActorLocation());
	}
}

void AThrowable_B::OnThrowOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsHeld() || OtherActor == OwningPlayer || OtherActor == this)
		return;

	Destroy();
	
	BScreen("%s is overlapping with %s", *GetNameSafe(this),*GetNameSafe(OtherActor));
}

AThrowable_B* AThrowable_B::PickedUp(APlayerCharacter_B* Owner)
{
	//DestructibleMesh->SetSimulatePhysics(false);
	//DestructibleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//DestructibleMesh->SetVisibility(false);

	//Mesh->SetVisibility(true);
	//Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OwningPlayer = Owner;
	return nullptr;
}

void AThrowable_B::Dropped()
{
//	DestructibleMesh->SetVisibility(true);
//	DestructibleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
////	DestructibleMesh->SetSimulatePhysics(true);
//
//	Mesh->SetVisibility(false);
//	
//	/// Setter p� kollisjon s� den kan finne gulvet.
	PickupSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
}

UMeshComponent* AThrowable_B::GetMesh()
{
	return nullptr;
}

bool AThrowable_B::IsHeld() const
{
	if (OwningPlayer)
		return true;
	return false;
}




