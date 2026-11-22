// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SequencePlayAtActorComponent.generated.h"

class AActor;
class ALevelSequenceActor;
class ULevelSequencePlayer;

UENUM(BlueprintType)
enum class ESequencePlayAnchorMode : uint8
{
	Owner,
	PlayerPawn,
	ExplicitActor
};

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USequencePlayAtActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USequencePlayAtActorComponent();

	UFUNCTION(BlueprintCallable, Category="Sequence|PlayAtActor")
	bool PlayConfigured();

	UFUNCTION(BlueprintCallable, Category="Sequence|PlayAtActor")
	bool PlayAtOwner();

	UFUNCTION(BlueprintCallable, Category="Sequence|PlayAtActor")
	bool PlayAtPlayer();

	UFUNCTION(BlueprintCallable, Category="Sequence|PlayAtActor")
	bool PlayAtActor(AActor* AnchorActor);

	UFUNCTION(BlueprintCallable, Category="Sequence|PlayAtActor")
	void StopPlayback();

	UFUNCTION(BlueprintPure, Category="Sequence|PlayAtActor")
	ULevelSequencePlayer* GetSequencePlayer() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AActor* ResolveAnchorActor(ESequencePlayAnchorMode InAnchorMode, AActor* InExplicitAnchorActor) const;
	FTransform BuildPlaybackTransform(const AActor* AnchorActor) const;
	bool ApplyBindingByTag(ALevelSequenceActor* InTargetSequenceActor, AActor* InAnchorActor) const;
	bool ApplyTransformOrigin(ALevelSequenceActor* InTargetSequenceActor, const FTransform& InTransform) const;
	bool PlayInternal(ESequencePlayAnchorMode InAnchorMode, AActor* InExplicitAnchorActor);
	void LogPlayMessage(const FString& Message, bool bWarning = false) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ALevelSequenceActor> TargetLevelSequenceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true"))
	ESequencePlayAnchorMode AnchorMode = ESequencePlayAnchorMode::Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true"))
	TObjectPtr<AActor> ExplicitAnchorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true"))
	bool bRebindByTag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true", EditCondition="bRebindByTag"))
	FName BindingTag = TEXT("Player");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true"))
	bool bUseTransformOrigin = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true", EditCondition="bUseTransformOrigin"))
	bool bUseAnchorRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true", EditCondition="bUseTransformOrigin"))
	bool bApplyLocationOffsetInAnchorSpace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true", EditCondition="bUseTransformOrigin"))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true", EditCondition="bUseTransformOrigin"))
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor", meta=(AllowPrivateAccess="true"))
	bool bStopBeforePlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|PlayAtActor|Debug", meta=(AllowPrivateAccess="true"))
	bool bEnableSequencePlayLog = true;
};
