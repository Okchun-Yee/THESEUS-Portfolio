// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Sequence/TickerPaperTypes.h"
#include "TickerPaperManager.generated.h"

class UTheseusEventBusSubsystem;
class ATickerPaperActor;
class APlayerController;
class APawn;
class USceneComponent;
class UMaterialInterface;
class UAnimMontage;

UCLASS()
class DEV_THESEUS_API ATickerPaperManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATickerPaperManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void HandleTickerPrintRequested(const FTickerPrintRequest& Request);

private:
    void ApplyTickerInputLock(bool bLock);
    void HandlePrintMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    bool TryGetNamedSceneComponentSpawnTransform(
        const FTickerPrintRequest& Request,
        FVector& OutLocation,
        FVector& OutForward,
        FRotator& OutRotation,
        USceneComponent*& OutSourceComponent,
        FString& OutSourceInfo) const;

    bool TryGetSocketSpawnTransform(
        const FTickerPrintRequest& Request,
        FVector& OutLocation,
        FVector& OutForward,
        FRotator& OutRotation,
        FString& OutSocketInfo) const;

    FVector ResolveSpawnLocation(const FTickerPrintRequest& Request) const;
    FVector ResolveSpawnForward(const FTickerPrintRequest& Request) const;

private:
    TWeakObjectPtr<UTheseusEventBusSubsystem> CachedBus;
    TWeakObjectPtr<APlayerController> InputLockedPlayerController;
    TWeakObjectPtr<APawn> InputLockedPawn;
    FTimerHandle MontagePlayTimerHandle;
    bool bTickerInputLocked = false;
    bool bTickerPawnInputDisabled = false;

    UPROPERTY(EditAnywhere, Category="Ticker")
    TSubclassOf<ATickerPaperActor> TickerPaperClass;

    UPROPERTY(EditAnywhere, Category="Ticker")
    FName MuzzleSocketName = TEXT("Muzzle");

    UPROPERTY(EditAnywhere, Category="Ticker")
    TMap<FGameplayTag, TObjectPtr<UMaterialInterface>> PaperMaterialMap;

    UPROPERTY(EditAnywhere, Category="Ticker|Montage")
    TObjectPtr<UAnimMontage> PrintMontage;

    UPROPERTY(EditAnywhere, Category="Ticker|Montage", meta=(ClampMin="0.0"))
    float MontagePlayDelay = 1.0f;

    UPROPERTY(EditAnywhere, Category="Ticker|Montage")
    FName ArmsMeshComponentName = TEXT("Skeletal Mesh Arms");

    UPROPERTY(EditAnywhere, Category="Ticker")
    FName PreferredSpawnComponentName = TEXT("PaperExit Socket");
};
