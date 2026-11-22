// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_GameAcquireTarget.generated.h"

UCLASS()
class DEV_THESEUS_API UBTService_GameAcquireTarget : public UBTService_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTService_GameAcquireTarget();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector IsSplineModeKey;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector ChaseTargetKey;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector AcquireRangeKey;
};
