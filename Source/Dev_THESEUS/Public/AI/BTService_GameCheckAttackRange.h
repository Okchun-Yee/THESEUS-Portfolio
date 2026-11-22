// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_GameCheckAttackRange.generated.h"

UCLASS()
class DEV_THESEUS_API UBTService_GameCheckAttackRange : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_GameCheckAttackRange();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
    bool IsFacingTarget(const APawn* ControlledPawn, const AActor* Target) const;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector ChaseTargetKey;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector AttackRangeKey;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector IsInAttackRangeKey;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector RequireFacingToFireKey;

    UPROPERTY(EditAnywhere, Category="Attack")
    bool bDefaultRequireFacingTarget = true;

    UPROPERTY(EditAnywhere, Category="Attack", meta=(ClampMin="1.0", ClampMax="180.0"))
    float FacingHalfAngleDegrees = 70.0f;

    UPROPERTY(EditAnywhere, Category="Debug")
    bool bEnableAttackRangeDebugLog = false;

    UPROPERTY(EditAnywhere, Category="Debug", meta=(ClampMin="0.0"))
    float DebugLogIntervalSeconds = 0.5f;

    float LastDebugLogWorldTime = -100000.0f;
};
