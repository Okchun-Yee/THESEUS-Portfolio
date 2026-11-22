// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "SplineSegmentSequencerComponent.generated.h"

class USplineComponent;
class USplineFollowComponent;

USTRUCT(BlueprintType)
struct FSplineSequenceSegment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SplineSequence")
    FName SegmentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SplineSequence")
    FComponentReference SplineComponentRef;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SplineSequence", meta=(ClampMin="-1.0"))
    float MoveSpeedOverride = -1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SplineSequence")
    bool bRequireTriggerToStart = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SplineSequence", meta=(EditCondition="bRequireTriggerToStart"))
    FName StartTriggerId = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSplineSequenceSegmentEvent, int32, SegmentIndex, FName, SegmentId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSplineSequenceWaitingForTrigger, int32, NextSegmentIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSplineSequenceCompleted);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USplineSegmentSequencerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USplineSegmentSequencerComponent();

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    bool StartFirstSegment();

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    bool RequestNextSegmentByIndex(int32 SegmentIndex);

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    bool RequestNextSegmentByTrigger(FName TriggerId);

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    void ResetSequence(bool bStopCurrentFollow = true);

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    bool IsWaitingForTrigger() const { return bWaitingForTrigger; }

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    bool IsSequenceCompleted() const { return bSequenceCompleted; }

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    int32 GetCurrentSegmentIndex() const { return CurrentSegmentIndex; }

    UFUNCTION(BlueprintCallable, Category="SplineSequence")
    int32 GetNextExpectedSegmentIndex() const { return NextExpectedSegmentIndex; }

    UPROPERTY(BlueprintAssignable, Category="SplineSequence")
    FOnSplineSequenceSegmentEvent OnSegmentStarted;

    UPROPERTY(BlueprintAssignable, Category="SplineSequence")
    FOnSplineSequenceSegmentEvent OnSegmentArrived;

    UPROPERTY(BlueprintAssignable, Category="SplineSequence")
    FOnSplineSequenceWaitingForTrigger OnWaitingForTrigger;

    UPROPERTY(BlueprintAssignable, Category="SplineSequence")
    FOnSplineSequenceCompleted OnSequenceCompleted;

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void HandleFollowFinished(bool bInterrupted);

    bool ValidateSegments() const;
    bool StartSegmentInternal(int32 SegmentIndex);
    bool IsTriggerAcceptedForSegment(int32 SegmentIndex, FName TriggerId) const;
    float ResolveMoveSpeed(const FSplineSequenceSegment& Segment) const;
    USplineFollowComponent* ResolveFollowComponent();
    USplineComponent* ResolveSplineComponent(const FSplineSequenceSegment& Segment) const;
    void QueuePendingRequest(int32 SegmentIndex, FName TriggerId);
    void TryConsumeQueuedRequest();
    void ClearQueuedRequest();
    void LogDebug(const FString& Message) const;
    void LogWarning(const FString& Message) const;

private:
    UPROPERTY(EditAnywhere, Category="SplineSequence|Setup")
    TArray<FSplineSequenceSegment> Segments;

    UPROPERTY(EditAnywhere, Category="SplineSequence|Setup", meta=(ClampMin="0"))
    int32 FirstSegmentIndex = 0;

    UPROPERTY(EditAnywhere, Category="SplineSequence|Setup")
    FComponentReference FollowComponentRef;

    UPROPERTY(EditAnywhere, Category="SplineSequence|Runtime", meta=(ClampMin="0.0"))
    float DefaultMoveSpeed = 300.0f;

    UPROPERTY(EditAnywhere, Category="SplineSequence|Runtime")
    bool bAutoStartFirstSegment = false;

    UPROPERTY(EditAnywhere, Category="SplineSequence|Runtime")
    bool bQueueEarlyTriggerRequests = true;

    UPROPERTY(EditAnywhere, Category="SplineSequence|Debug")
    bool bEnableDebugLog = false;

    UPROPERTY(VisibleAnywhere, Category="SplineSequence|State")
    int32 CurrentSegmentIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, Category="SplineSequence|State")
    int32 NextExpectedSegmentIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, Category="SplineSequence|State")
    bool bWaitingForTrigger = false;

    UPROPERTY(VisibleAnywhere, Category="SplineSequence|State")
    bool bSequenceCompleted = false;

    UPROPERTY(VisibleAnywhere, Category="SplineSequence|State")
    bool bHasQueuedRequest = false;

    UPROPERTY(VisibleAnywhere, Category="SplineSequence|State")
    int32 QueuedSegmentIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, Category="SplineSequence|State")
    FName QueuedTriggerId = NAME_None;

    UPROPERTY(Transient)
    TObjectPtr<USplineFollowComponent> CachedFollowComponent = nullptr;
};
