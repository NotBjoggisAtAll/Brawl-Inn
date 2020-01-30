// Fill out your copyright notice in the Description page of Project Settings.


#include "BT_PickupItem_B.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "AI/NavigationSystemBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include "Items/Item_B.h"
#include "Items/Throwable_B.h"
#include "System/DamageTypes/Stool_DamageType_B.h"
#include "Characters/AI/AIController_B.h"
#include "Characters/AI/AICharacter_B.h"
#include "BrawlInn.h"

UBT_PickupItem_B::UBT_PickupItem_B()
{
	bNotifyTick = true;
}

EBTNodeResult::Type UBT_PickupItem_B::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	OwningAI = Cast<AAIController_B>(OwnerComp.GetAIOwner());
	if (!OwningAI)
	{
		BError("Can't find AI controller");
		return EBTNodeResult::Failed;
	}

	Item = Cast<AItem_B>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(ItemToPickup.SelectedKeyName));

	return EBTNodeResult::InProgress;
}

void UBT_PickupItem_B::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	if (Item)
	{

		switch (OwningAI->MoveToActor(Item, 200.f))
		{
		case EPathFollowingRequestResult::AlreadyAtGoal:
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			break;
		case EPathFollowingRequestResult::RequestSuccessful:
			BScreen("Moving to Item");
			break;
		default:
			break;
		}
	}

}
