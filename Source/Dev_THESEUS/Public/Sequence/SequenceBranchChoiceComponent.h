// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sequence/TheseusBranchChoiceTypes.h"
#include "SequenceBranchChoiceComponent.generated.h"

class AActor;
class APawn;

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API USequenceBranchChoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USequenceBranchChoiceComponent();

	UFUNCTION(BlueprintCallable, Category="Sequence|Branch")
	void InitializeChoiceState();

	UFUNCTION(BlueprintCallable, Category="Sequence|Branch")
	void ResetChoiceState();

	UFUNCTION(BlueprintCallable, Category="Sequence|Branch")
	bool ChooseSide(ETheseusBranchSide InSide, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category="Sequence|Branch")
	bool ChooseLeft(AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category="Sequence|Branch")
	bool ChooseRight(AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category="Sequence|Branch")
	bool IsChoiceLocked() const { return bChoiceLocked; }

	UFUNCTION(BlueprintPure, Category="Sequence|Branch")
	ETheseusBranchSide GetSelectedSide() const { return SelectedSide; }

	UFUNCTION(BlueprintPure, Category="Sequence|Branch")
	TArray<APawn*> GetLastSpawnedUnits() const;

protected:
	virtual void BeginPlay() override;

private:
	const FTheseusBranchSideRuntimeData* GetSideData(ETheseusBranchSide InSide) const;
	const FTheseusBranchSideRuntimeData* GetOppositeSideData(ETheseusBranchSide InSide) const;
	AActor* ResolvePlayerActor(AActor* InstigatorActor);

	void SetChoiceTriggerActorsEnabled(bool bEnabled);
	void SetSideWallRemoved(const FTheseusBranchSideRuntimeData& InData, bool bRemoved);
	bool ActivateEncounterSide(const FTheseusBranchSideRuntimeData& InData, AActor* TargetActor);

	void SetManagedActorEnabled(AActor* InActor, bool bEnabled, bool bEnableCollision) const;
	void SetManagedActorArrayEnabled(const TArray<TObjectPtr<AActor>>& InActors, bool bEnabled, bool bEnableCollision) const;
	void LogChoiceMessage(const FString& Message, bool bWarning = false) const;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	FTheseusBranchSideRuntimeData LeftSideData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	FTheseusBranchSideRuntimeData RightSideData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<AActor>> ChoiceTriggerActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	TObjectPtr<AActor> PlayerActorOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	bool bAutoInitializeOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	bool bAutoResolvePlayerOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch|Debug", meta=(AllowPrivateAccess="true"))
	bool bDebugLog = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence|Branch|Observe", meta=(AllowPrivateAccess="true"))
	bool bActivateEyesOnEncounterSpawn = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	bool bChoiceLocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	ETheseusBranchSide SelectedSide = ETheseusBranchSide::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	TObjectPtr<AActor> CachedPlayerActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sequence|Branch", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<APawn>> LastSpawnedUnits;
};
