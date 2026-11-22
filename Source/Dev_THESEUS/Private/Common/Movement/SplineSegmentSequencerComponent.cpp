// Copyright 2026, Okchun-Yee.

#include "Common/Movement/SplineSegmentSequencerComponent.h"
#include "Common/Movement/SplineFollowComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"

USplineSegmentSequencerComponent::USplineSegmentSequencerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USplineSegmentSequencerComponent::BeginPlay()
{
    Super::BeginPlay();

    USplineFollowComponent* FollowComponent = ResolveFollowComponent();
    if (!FollowComponent)
    {
        LogWarning(TEXT("BeginPlay failed: FollowComponent is null."));
        return;
    }

    FollowComponent->OnFollowFinished.AddDynamic(this, &USplineSegmentSequencerComponent::HandleFollowFinished);

    if (!ValidateSegments())
    {
        LogWarning(TEXT("BeginPlay: segment validation failed. Sequencer will stay idle."));
        return;
    }

    if (bAutoStartFirstSegment)
    {
        StartFirstSegment();
    }
}

bool USplineSegmentSequencerComponent::StartFirstSegment()
{
    if (!ValidateSegments())
    {
        return false;
    }

    if (CurrentSegmentIndex != INDEX_NONE || bSequenceCompleted)
    {
        LogDebug(TEXT("StartFirstSegment skipped: sequence already started or completed."));
        return false;
    }

    return StartSegmentInternal(FirstSegmentIndex);
}

bool USplineSegmentSequencerComponent::RequestNextSegmentByIndex(int32 SegmentIndex)
{
    if (!ValidateSegments())
    {
        return false;
    }

    if (bSequenceCompleted)
    {
        LogDebug(TEXT("RequestNextSegmentByIndex skipped: sequence already completed."));
        return false;
    }

    if (USplineFollowComponent* FollowComponent = ResolveFollowComponent(); FollowComponent && FollowComponent->IsFollowing())
    {
        const bool bCanQueue = bQueueEarlyTriggerRequests && SegmentIndex == CurrentSegmentIndex + 1;
        if (bCanQueue)
        {
            QueuePendingRequest(SegmentIndex, NAME_None);
            LogDebug(FString::Printf(TEXT("Queued early segment request while moving. SegmentIndex=%d"), SegmentIndex));
            return true;
        }

        LogDebug(FString::Printf(TEXT("RequestNextSegmentByIndex rejected while moving. SegmentIndex=%d"), SegmentIndex));
        return false;
    }

    if (!bWaitingForTrigger)
    {
        LogDebug(FString::Printf(TEXT("RequestNextSegmentByIndex rejected: not waiting for trigger. SegmentIndex=%d"), SegmentIndex));
        return false;
    }

    if (SegmentIndex != NextExpectedSegmentIndex)
    {
        LogWarning(FString::Printf(TEXT("RequestNextSegmentByIndex rejected: expected %d but got %d."), NextExpectedSegmentIndex, SegmentIndex));
        return false;
    }

    return StartSegmentInternal(SegmentIndex);
}

bool USplineSegmentSequencerComponent::RequestNextSegmentByTrigger(FName TriggerId)
{
    if (!ValidateSegments())
    {
        return false;
    }

    if (bSequenceCompleted)
    {
        LogDebug(TEXT("RequestNextSegmentByTrigger skipped: sequence already completed."));
        return false;
    }

    const int32 CandidateIndex = (CurrentSegmentIndex == INDEX_NONE) ? FirstSegmentIndex : (CurrentSegmentIndex + 1);

    if (USplineFollowComponent* FollowComponent = ResolveFollowComponent(); FollowComponent && FollowComponent->IsFollowing())
    {
        const bool bCanQueue = bQueueEarlyTriggerRequests && CandidateIndex == CurrentSegmentIndex + 1;
        if (bCanQueue)
        {
            QueuePendingRequest(CandidateIndex, TriggerId);
            LogDebug(FString::Printf(TEXT("Queued early trigger request while moving. TriggerId=%s Next=%d"), *TriggerId.ToString(), CandidateIndex));
            return true;
        }

        LogDebug(FString::Printf(TEXT("RequestNextSegmentByTrigger rejected while moving. TriggerId=%s"), *TriggerId.ToString()));
        return false;
    }

    if (!bWaitingForTrigger)
    {
        LogDebug(FString::Printf(TEXT("RequestNextSegmentByTrigger rejected: not waiting. TriggerId=%s"), *TriggerId.ToString()));
        return false;
    }

    if (NextExpectedSegmentIndex == INDEX_NONE || !Segments.IsValidIndex(NextExpectedSegmentIndex))
    {
        LogWarning(TEXT("RequestNextSegmentByTrigger rejected: invalid next expected segment index."));
        return false;
    }

    if (!IsTriggerAcceptedForSegment(NextExpectedSegmentIndex, TriggerId))
    {
        const FName ExpectedTrigger = Segments[NextExpectedSegmentIndex].StartTriggerId;
        LogWarning(FString::Printf(TEXT("RequestNextSegmentByTrigger rejected: trigger mismatch. Expected=%s Got=%s"),
            *ExpectedTrigger.ToString(),
            *TriggerId.ToString()));
        return false;
    }

    return StartSegmentInternal(NextExpectedSegmentIndex);
}

void USplineSegmentSequencerComponent::ResetSequence(bool bStopCurrentFollow)
{
    if (USplineFollowComponent* FollowComponent = ResolveFollowComponent(); bStopCurrentFollow && FollowComponent && FollowComponent->IsFollowing())
    {
        FollowComponent->StopFollow();
    }

    CurrentSegmentIndex = INDEX_NONE;
    NextExpectedSegmentIndex = INDEX_NONE;
    bWaitingForTrigger = false;
    bSequenceCompleted = false;
    ClearQueuedRequest();

    LogDebug(TEXT("ResetSequence completed."));
}

void USplineSegmentSequencerComponent::HandleFollowFinished(bool bInterrupted)
{
    if (CurrentSegmentIndex == INDEX_NONE || !Segments.IsValidIndex(CurrentSegmentIndex))
    {
        LogWarning(TEXT("HandleFollowFinished: current segment is invalid."));
        return;
    }

    const FSplineSequenceSegment& FinishedSegment = Segments[CurrentSegmentIndex];
    OnSegmentArrived.Broadcast(CurrentSegmentIndex, FinishedSegment.SegmentId);

    if (bInterrupted)
    {
        LogDebug(FString::Printf(TEXT("Segment interrupted. Index=%d Id=%s"), CurrentSegmentIndex, *FinishedSegment.SegmentId.ToString()));
        return;
    }

    const int32 NextIndex = CurrentSegmentIndex + 1;
    if (!Segments.IsValidIndex(NextIndex))
    {
        bWaitingForTrigger = false;
        NextExpectedSegmentIndex = INDEX_NONE;
        bSequenceCompleted = true;
        OnSequenceCompleted.Broadcast();
        LogDebug(TEXT("Sequence completed."));
        return;
    }

    bWaitingForTrigger = true;
    NextExpectedSegmentIndex = NextIndex;
    OnWaitingForTrigger.Broadcast(NextExpectedSegmentIndex);

    const FSplineSequenceSegment& NextSegment = Segments[NextExpectedSegmentIndex];
    LogDebug(FString::Printf(TEXT("Waiting for next trigger. NextIndex=%d NextId=%s Trigger=%s"),
        NextExpectedSegmentIndex,
        *NextSegment.SegmentId.ToString(),
        *NextSegment.StartTriggerId.ToString()));

    TryConsumeQueuedRequest();
}

bool USplineSegmentSequencerComponent::ValidateSegments() const
{
    if (!Segments.IsEmpty() && !Segments.IsValidIndex(FirstSegmentIndex))
    {
        LogWarning(FString::Printf(TEXT("ValidateSegments failed: FirstSegmentIndex(%d) is out of range."), FirstSegmentIndex));
        return false;
    }

    TSet<FName> SegmentIdSet;
    for (int32 Index = 0; Index < Segments.Num(); ++Index)
    {
        const FSplineSequenceSegment& Segment = Segments[Index];
        if (!ResolveSplineComponent(Segment))
        {
            LogWarning(FString::Printf(TEXT("ValidateSegments warning: Segment[%d] spline component reference is invalid."), Index));
            return false;
        }

        if (Segment.SegmentId != NAME_None)
        {
            if (SegmentIdSet.Contains(Segment.SegmentId))
            {
                LogWarning(FString::Printf(TEXT("ValidateSegments warning: duplicated SegmentId '%s'."), *Segment.SegmentId.ToString()));
                return false;
            }
            SegmentIdSet.Add(Segment.SegmentId);
        }

        if (Index != FirstSegmentIndex && Segment.bRequireTriggerToStart && Segment.StartTriggerId == NAME_None)
        {
            LogWarning(FString::Printf(TEXT("ValidateSegments warning: Segment[%d] requires trigger but StartTriggerId is None."), Index));
        }
    }

    return true;
}

bool USplineSegmentSequencerComponent::StartSegmentInternal(int32 SegmentIndex)
{
    if (!Segments.IsValidIndex(SegmentIndex))
    {
        LogWarning(FString::Printf(TEXT("StartSegmentInternal failed: invalid index %d."), SegmentIndex));
        return false;
    }

    USplineFollowComponent* FollowComponent = ResolveFollowComponent();
    if (!FollowComponent)
    {
        LogWarning(TEXT("StartSegmentInternal failed: FollowComponent is null."));
        return false;
    }

    if (FollowComponent->IsFollowing())
    {
        LogWarning(FString::Printf(TEXT("StartSegmentInternal failed: already following. RequestedIndex=%d"), SegmentIndex));
        return false;
    }

    const FSplineSequenceSegment& Segment = Segments[SegmentIndex];
    USplineComponent* SplineRef = ResolveSplineComponent(Segment);
    if (!SplineRef)
    {
        LogWarning(FString::Printf(TEXT("StartSegmentInternal failed: spline component is null for segment %d."), SegmentIndex));
        return false;
    }

    const float Speed = ResolveMoveSpeed(Segment);
    if (Speed <= 0.0f)
    {
        LogWarning(FString::Printf(TEXT("StartSegmentInternal failed: invalid move speed %.2f for segment %d."), Speed, SegmentIndex));
        return false;
    }

    if (!FollowComponent->StartFollow(SplineRef, Speed))
    {
        LogWarning(FString::Printf(TEXT("StartSegmentInternal failed: StartFollow returned false. Index=%d"), SegmentIndex));
        return false;
    }

    CurrentSegmentIndex = SegmentIndex;
    bWaitingForTrigger = false;
    bSequenceCompleted = false;
    NextExpectedSegmentIndex = INDEX_NONE;
    ClearQueuedRequest();

    OnSegmentStarted.Broadcast(CurrentSegmentIndex, Segment.SegmentId);
    LogDebug(FString::Printf(TEXT("Segment started. Index=%d Id=%s Speed=%.1f"),
        CurrentSegmentIndex,
        *Segment.SegmentId.ToString(),
        Speed));

    return true;
}

USplineFollowComponent* USplineSegmentSequencerComponent::ResolveFollowComponent()
{
    if (CachedFollowComponent)
    {
        return CachedFollowComponent;
    }

    if (AActor* Owner = GetOwner())
    {
        if (UActorComponent* Picked = FollowComponentRef.GetComponent(Owner))
        {
            CachedFollowComponent = Cast<USplineFollowComponent>(Picked);
        }

        if (!CachedFollowComponent)
        {
            CachedFollowComponent = Owner->FindComponentByClass<USplineFollowComponent>();
        }
    }

    return CachedFollowComponent;
}

USplineComponent* USplineSegmentSequencerComponent::ResolveSplineComponent(const FSplineSequenceSegment& Segment) const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    UActorComponent* Component = Segment.SplineComponentRef.GetComponent(Owner);
    if (!Component)
    {
        return nullptr;
    }

    return Cast<USplineComponent>(Component);
}

bool USplineSegmentSequencerComponent::IsTriggerAcceptedForSegment(int32 SegmentIndex, FName TriggerId) const
{
    if (!Segments.IsValidIndex(SegmentIndex))
    {
        return false;
    }

    const FSplineSequenceSegment& Segment = Segments[SegmentIndex];
    if (!Segment.bRequireTriggerToStart)
    {
        return true;
    }

    if (Segment.StartTriggerId == NAME_None)
    {
        return TriggerId != NAME_None;
    }

    return Segment.StartTriggerId == TriggerId;
}

float USplineSegmentSequencerComponent::ResolveMoveSpeed(const FSplineSequenceSegment& Segment) const
{
    if (Segment.MoveSpeedOverride > 0.0f)
    {
        return Segment.MoveSpeedOverride;
    }

    return DefaultMoveSpeed;
}

void USplineSegmentSequencerComponent::QueuePendingRequest(int32 SegmentIndex, FName TriggerId)
{
    bHasQueuedRequest = true;
    QueuedSegmentIndex = SegmentIndex;
    QueuedTriggerId = TriggerId;
}

void USplineSegmentSequencerComponent::TryConsumeQueuedRequest()
{
    if (!bHasQueuedRequest || !bWaitingForTrigger)
    {
        return;
    }

    const int32 PendingIndex = QueuedSegmentIndex;
    const FName PendingTrigger = QueuedTriggerId;
    ClearQueuedRequest();

    if (PendingIndex != NextExpectedSegmentIndex)
    {
        LogWarning(FString::Printf(TEXT("Queued request dropped: expected %d but queued %d."), NextExpectedSegmentIndex, PendingIndex));
        return;
    }

    if (!IsTriggerAcceptedForSegment(PendingIndex, PendingTrigger))
    {
        const FName ExpectedTrigger = Segments[PendingIndex].StartTriggerId;
        LogWarning(FString::Printf(TEXT("Queued request dropped: trigger mismatch. Expected=%s Got=%s"),
            *ExpectedTrigger.ToString(),
            *PendingTrigger.ToString()));
        return;
    }

    LogDebug(FString::Printf(TEXT("Consuming queued request. Index=%d Trigger=%s"), PendingIndex, *PendingTrigger.ToString()));
    StartSegmentInternal(PendingIndex);
}

void USplineSegmentSequencerComponent::ClearQueuedRequest()
{
    bHasQueuedRequest = false;
    QueuedSegmentIndex = INDEX_NONE;
    QueuedTriggerId = NAME_None;
}

void USplineSegmentSequencerComponent::LogDebug(const FString& Message) const
{
    if (!bEnableDebugLog)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SplineSeq] %s"), *Message);
}

void USplineSegmentSequencerComponent::LogWarning(const FString& Message) const
{
    UE_LOG(LogTemp, Warning, TEXT("[SplineSeq] %s"), *Message);
}
