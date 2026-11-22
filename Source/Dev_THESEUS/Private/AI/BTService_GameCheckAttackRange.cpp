// Copyright 2026, Okchun-Yee.

#include "AI/BTService_GameCheckAttackRange.h"
#include "AIController.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

UBTService_GameCheckAttackRange::UBTService_GameCheckAttackRange()
{
    NodeName = TEXT("Game Check Attack Range");
    ChaseTargetKey.SelectedKeyName = TEXT("Chase_Target");
    AttackRangeKey.SelectedKeyName = TEXT("AttackRange");
    IsInAttackRangeKey.SelectedKeyName = TEXT("IsInAttackRange");
    RequireFacingToFireKey.SelectedKeyName = TEXT("RequireFacingToFire");

    ChaseTargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_GameCheckAttackRange, ChaseTargetKey), AActor::StaticClass());
    AttackRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_GameCheckAttackRange, AttackRangeKey));
    IsInAttackRangeKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_GameCheckAttackRange, IsInAttackRangeKey));
    RequireFacingToFireKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_GameCheckAttackRange, RequireFacingToFireKey));
}

void UBTService_GameCheckAttackRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
    if (!BlackboardComp || !ControlledPawn)
    {
        return;
    }

    AActor* ChaseTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(ChaseTargetKey.SelectedKeyName));
    if (!ChaseTarget)
    {
        BlackboardComp->SetValueAsBool(IsInAttackRangeKey.SelectedKeyName, false);
        return;
    }

    const float AttackRange = BlackboardComp->GetValueAsFloat(AttackRangeKey.SelectedKeyName);
    if (AttackRange <= 0.0f)
    {
        BlackboardComp->SetValueAsBool(IsInAttackRangeKey.SelectedKeyName, false);
        return;
    }

    bool bRequireFacing = bDefaultRequireFacingTarget;
    bool bRequireFacingResolved = false;
    if (RequireFacingToFireKey.SelectedKeyName != NAME_None)
    {
        const FBlackboard::FKey KeyId = BlackboardComp->GetKeyID(RequireFacingToFireKey.SelectedKeyName);
        if (BlackboardComp->IsValidKey(KeyId))
        {
            bRequireFacing = BlackboardComp->GetValueAsBool(RequireFacingToFireKey.SelectedKeyName);
            bRequireFacingResolved = true;
        }
    }

    if (!bRequireFacingResolved)
    {
        if (const FBoolProperty* RequireFacingProp = FindFProperty<FBoolProperty>(ControlledPawn->GetClass(), TEXT("RequireFacingToFire")))
        {
            bRequireFacing = RequireFacingProp->GetPropertyValue_InContainer(ControlledPawn);
            bRequireFacingResolved = true;
        }
        else if (const FBoolProperty* RequireFacingAltProp = FindFProperty<FBoolProperty>(ControlledPawn->GetClass(), TEXT("bRequireFacingToFire")))
        {
            bRequireFacing = RequireFacingAltProp->GetPropertyValue_InContainer(ControlledPawn);
            bRequireFacingResolved = true;
        }
    }

    const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), ChaseTarget->GetActorLocation());
    const bool bInRange = Distance <= AttackRange;
    const bool bCanAttackByFacing = !bRequireFacing || IsFacingTarget(ControlledPawn, ChaseTarget);
    const bool bFinalInAttackRange = bInRange && bCanAttackByFacing;
    BlackboardComp->SetValueAsBool(IsInAttackRangeKey.SelectedKeyName, bFinalInAttackRange);

    if (bEnableAttackRangeDebugLog)
    {
        const UWorld* World = ControlledPawn->GetWorld();
        const float Now = World ? World->GetTimeSeconds() : 0.0f;
        if (!World || (Now - LastDebugLogWorldTime) >= FMath::Max(0.0f, DebugLogIntervalSeconds))
        {
            LastDebugLogWorldTime = Now;
            UE_LOG(
                LogTemp,
                Log,
                TEXT("[CheckAttackRange] Pawn=%s Target=%s Dist=%.1f AttackRange=%.1f RequireFacing=%s InRange=%s FacingPass=%s Final=%s"),
                *GetNameSafe(ControlledPawn),
                *GetNameSafe(ChaseTarget),
                Distance,
                AttackRange,
                bRequireFacing ? TEXT("true") : TEXT("false"),
                bInRange ? TEXT("true") : TEXT("false"),
                bCanAttackByFacing ? TEXT("true") : TEXT("false"),
                bFinalInAttackRange ? TEXT("true") : TEXT("false"));
        }
    }
}

bool UBTService_GameCheckAttackRange::IsFacingTarget(const APawn* ControlledPawn, const AActor* Target) const
{
    if (!ControlledPawn || !Target)
    {
        return false;
    }

    const FVector ToControlledPawn = (ControlledPawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
    FVector TargetForward2D = Target->GetActorForwardVector().GetSafeNormal2D();

    // In FPS, camera/control rotation is a better "look direction" than actor mesh forward.
    if (const APawn* TargetPawn = Cast<APawn>(Target))
    {
        if (const AController* TargetController = TargetPawn->GetController())
        {
            const FRotator ControlRot = TargetController->GetControlRotation();
            TargetForward2D = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X).GetSafeNormal2D();
        }
    }

    if (ToControlledPawn.IsNearlyZero() || TargetForward2D.IsNearlyZero())
    {
        return false;
    }

    const float FacingDotThreshold = FMath::Cos(FMath::DegreesToRadians(FacingHalfAngleDegrees));
    const float FacingDot = FVector::DotProduct(TargetForward2D, ToControlledPawn);
    return FacingDot >= FacingDotThreshold;
}
