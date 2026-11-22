// Copyright 2026, Okchun-Yee.

#include "AI/BTService_GameAcquireTarget.h"
#include "AIController.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UBTService_GameAcquireTarget::UBTService_GameAcquireTarget()
{
    NodeName = TEXT("Game Acquire Target");

    IsSplineModeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_GameAcquireTarget, IsSplineModeKey));
    ChaseTargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_GameAcquireTarget, ChaseTargetKey), AActor::StaticClass());
    AcquireRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_GameAcquireTarget, AcquireRangeKey));
}

void UBTService_GameAcquireTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    if (BlackboardComp->GetValueAsBool(IsSplineModeKey.SelectedKeyName))
    {
        return;
    }

    if (IsValid(BlackboardComp->GetValueAsObject(ChaseTargetKey.SelectedKeyName)))
    {
        return;
    }

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return;
    }

    APawn* ControlledPawn = AIController->GetPawn();
    APlayerController* PlayerController = AIController->GetWorld() ? AIController->GetWorld()->GetFirstPlayerController() : nullptr;
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (!ControlledPawn || !PlayerPawn)
    {
        return;
    }

    const float AcquireRange = BlackboardComp->GetValueAsFloat(AcquireRangeKey.SelectedKeyName);
    if (AcquireRange <= 0.0f)
    {
        return;
    }

    const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
    if (Distance <= AcquireRange)
    {
        BlackboardComp->SetValueAsObject(ChaseTargetKey.SelectedKeyName, PlayerPawn);
    }
}
