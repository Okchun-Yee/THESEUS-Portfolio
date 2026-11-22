// Copyright 2026, Okchun-Yee.

#include "Sequence/SequencePlayAtActorComponent.h"

#include "DefaultLevelSequenceInstanceData.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

USequencePlayAtActorComponent::USequencePlayAtActorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool USequencePlayAtActorComponent::PlayConfigured()
{
	return PlayInternal(AnchorMode, ExplicitAnchorActor);
}

bool USequencePlayAtActorComponent::PlayAtOwner()
{
	return PlayInternal(ESequencePlayAnchorMode::Owner, nullptr);
}

bool USequencePlayAtActorComponent::PlayAtPlayer()
{
	return PlayInternal(ESequencePlayAnchorMode::PlayerPawn, nullptr);
}

bool USequencePlayAtActorComponent::PlayAtActor(AActor* AnchorActor)
{
	return PlayInternal(ESequencePlayAnchorMode::ExplicitActor, AnchorActor);
}

void USequencePlayAtActorComponent::StopPlayback()
{
	if (ULevelSequencePlayer* SequencePlayer = GetSequencePlayer())
	{
		SequencePlayer->Stop();
	}
}

ULevelSequencePlayer* USequencePlayAtActorComponent::GetSequencePlayer() const
{
	return IsValid(TargetLevelSequenceActor) ? TargetLevelSequenceActor->GetSequencePlayer() : nullptr;
}

void USequencePlayAtActorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPlayback();
	Super::EndPlay(EndPlayReason);
}

AActor* USequencePlayAtActorComponent::ResolveAnchorActor(ESequencePlayAnchorMode InAnchorMode, AActor* InExplicitAnchorActor) const
{
	switch (InAnchorMode)
	{
	case ESequencePlayAnchorMode::Owner:
		return GetOwner();

	case ESequencePlayAnchorMode::PlayerPawn:
		return UGameplayStatics::GetPlayerPawn(this, 0);

	case ESequencePlayAnchorMode::ExplicitActor:
		return InExplicitAnchorActor;

	default:
		return nullptr;
	}
}

FTransform USequencePlayAtActorComponent::BuildPlaybackTransform(const AActor* AnchorActor) const
{
	if (!IsValid(AnchorActor))
	{
		return FTransform::Identity;
	}

	const FTransform AnchorTransform = AnchorActor->GetActorTransform();
	const FQuat BaseRotation = bUseAnchorRotation ? AnchorTransform.GetRotation() : FQuat::Identity;
	const FVector ResolvedOffset = bApplyLocationOffsetInAnchorSpace
		? BaseRotation.RotateVector(LocationOffset)
		: LocationOffset;

	FTransform PlaybackTransform = FTransform::Identity;
	PlaybackTransform.SetLocation(AnchorTransform.GetLocation() + ResolvedOffset);
	PlaybackTransform.SetRotation((BaseRotation * RotationOffset.Quaternion()).GetNormalized());
	PlaybackTransform.SetScale3D(FVector::OneVector);
	return PlaybackTransform;
}

bool USequencePlayAtActorComponent::ApplyBindingByTag(ALevelSequenceActor* InTargetSequenceActor, AActor* InAnchorActor) const
{
	if (!IsValid(InTargetSequenceActor) || !IsValid(InAnchorActor))
	{
		return false;
	}

	if (BindingTag.IsNone())
	{
		LogPlayMessage(TEXT("ApplyBindingByTag failed: BindingTag is None."), true);
		return false;
	}

	const TArray<FMovieSceneObjectBindingID>& MatchingBindings = InTargetSequenceActor->FindNamedBindings(BindingTag);
	if (MatchingBindings.Num() <= 0)
	{
		LogPlayMessage(FString::Printf(TEXT("ApplyBindingByTag failed: tag not found (%s)."), *BindingTag.ToString()), true);
		return false;
	}

	TArray<AActor*> BindingActors;
	BindingActors.Add(InAnchorActor);
	InTargetSequenceActor->SetBindingByTag(BindingTag, BindingActors, false);
	return true;
}

bool USequencePlayAtActorComponent::ApplyTransformOrigin(ALevelSequenceActor* InTargetSequenceActor, const FTransform& InTransform) const
{
	if (!IsValid(InTargetSequenceActor))
	{
		return false;
	}

	InTargetSequenceActor->bOverrideInstanceData = true;

	UDefaultLevelSequenceInstanceData* InstanceData = Cast<UDefaultLevelSequenceInstanceData>(InTargetSequenceActor->DefaultInstanceData);
	if (!InstanceData)
	{
		if (IsValid(InTargetSequenceActor->DefaultInstanceData))
		{
			LogPlayMessage(TEXT("ApplyTransformOrigin failed: DefaultInstanceData is not UDefaultLevelSequenceInstanceData."), true);
			return false;
		}

		InstanceData = NewObject<UDefaultLevelSequenceInstanceData>(InTargetSequenceActor, NAME_None, RF_Transactional);
		InTargetSequenceActor->DefaultInstanceData = InstanceData;
	}

	InstanceData->TransformOriginActor = nullptr;
	InstanceData->TransformOrigin = InTransform;
	return true;
}

bool USequencePlayAtActorComponent::PlayInternal(ESequencePlayAnchorMode InAnchorMode, AActor* InExplicitAnchorActor)
{
	if (!IsValid(TargetLevelSequenceActor))
	{
		LogPlayMessage(TEXT("Play failed: TargetLevelSequenceActor is null."), true);
		return false;
	}

	AActor* AnchorActor = ResolveAnchorActor(InAnchorMode, InExplicitAnchorActor);
	if (!IsValid(AnchorActor))
	{
		LogPlayMessage(TEXT("Play failed: AnchorActor is null."), true);
		return false;
	}

	if (bRebindByTag && !ApplyBindingByTag(TargetLevelSequenceActor, AnchorActor))
	{
		return false;
	}

	if (bUseTransformOrigin && !ApplyTransformOrigin(TargetLevelSequenceActor, BuildPlaybackTransform(AnchorActor)))
	{
		return false;
	}

	ULevelSequencePlayer* SequencePlayer = TargetLevelSequenceActor->GetSequencePlayer();
	if (!IsValid(SequencePlayer))
	{
		LogPlayMessage(TEXT("Play failed: SequencePlayer is null."), true);
		return false;
	}

	if (bStopBeforePlay && SequencePlayer->IsPlaying())
	{
		SequencePlayer->Stop();
	}

	SequencePlayer->Play();

	LogPlayMessage(FString::Printf(
		TEXT("Play success: Sequence=%s Anchor=%s Mode=%d"),
		*GetNameSafe(TargetLevelSequenceActor),
		*GetNameSafe(AnchorActor),
		static_cast<int32>(InAnchorMode)));

	return true;
}

void USequencePlayAtActorComponent::LogPlayMessage(const FString& Message, bool bWarning) const
{
	if (!bEnableSequencePlayLog && !bWarning)
	{
		return;
	}

	if (bWarning)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SequencePlayAtActor] %s"), *Message);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[SequencePlayAtActor] %s"), *Message);
}
