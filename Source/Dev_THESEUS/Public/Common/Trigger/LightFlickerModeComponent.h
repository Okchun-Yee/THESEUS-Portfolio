// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "LightFlickerModeComponent.generated.h"

class ULightComponent;
class UMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class ETheseusLightFlickerMode : uint8
{
    StableOn UMETA(DisplayName="Stable On"),
    FlickerOn UMETA(DisplayName="Flicker On")
};

UENUM(BlueprintType)
enum class ETheseusLightFlickerRunMode : uint8
{
    None UMETA(DisplayName="None"),
    Loop UMETA(DisplayName="Loop"),
    Burst UMETA(DisplayName="Burst")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTheseusLightFlickerModeChanged, ETheseusLightFlickerMode, NewMode);

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API ULightFlickerModeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULightFlickerModeComponent();

    UFUNCTION(BlueprintCallable, Category="Light|Flicker")
    bool StartFlickerLoop();

    UFUNCTION(BlueprintCallable, Category="Light|Flicker")
    bool StartFlickerBurst(int32 BlinkCount = 4, float IntervalSeconds = 0.06f);

    UFUNCTION(BlueprintCallable, Category="Light|Flicker")
    void StopAndRestore();

    UFUNCTION(BlueprintCallable, Category="Light|Flicker")
    bool IsRunning() const { return bIsRunning; }

    UFUNCTION(BlueprintCallable, Category="Light|Flicker")
    ETheseusLightFlickerMode GetCurrentMode() const { return CurrentMode; }

    UFUNCTION(BlueprintCallable, Category="Light|Flicker")
    void SetStartCallsBlocked(bool bBlocked) { bStartCallsBlocked = bBlocked; }

    UFUNCTION(BlueprintCallable, Category="Light|Flicker")
    bool IsStartCallsBlocked() const { return bStartCallsBlocked; }

    UPROPERTY(BlueprintAssignable, Category="Light|Flicker")
    FOnTheseusLightFlickerModeChanged OnModeChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ResolveTargetsIfNeeded();
    void ApplyCurrentMode();
    void ScheduleNextToggle();
    void HandleToggleTimerElapsed();
    void EnsureDynamicMaterialInstance();
    float GetRandomDurationForMode(ETheseusLightFlickerMode Mode) const;
    float GetRandomFlickerIntensity() const;
    void SetMaterialFlickerState(float Value);

private:
    UPROPERTY(EditAnywhere, Category="Light|Flicker|Target")
    TObjectPtr<ULightComponent> TargetLight = nullptr;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Target")
    TObjectPtr<UMeshComponent> TargetMesh = nullptr;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Target", meta=(ClampMin="0"))
    int32 MaterialElementIndex = 0;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Setup")
    bool bAutoResolveTargets = true;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Setup")
    bool bAutoStart = false;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Setup")
    ETheseusLightFlickerMode InitialMode = ETheseusLightFlickerMode::StableOn;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Duration", meta=(ClampMin="0.001"))
    float StableMinDuration = 0.8f;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Duration", meta=(ClampMin="0.001"))
    float StableMaxDuration = 2.2f;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Duration", meta=(ClampMin="0.001"))
    float FlickerMinDuration = 0.05f;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Duration", meta=(ClampMin="0.001"))
    float FlickerMaxDuration = 0.22f;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Intensity")
    bool bRandomizeFlickerIntensity = true;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Intensity", meta=(ClampMin="0.0"))
    float FlickerIntensityMin = 14.0f;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Intensity", meta=(ClampMin="0.0"))
    float FlickerIntensityMax = 23.0f;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Intensity", meta=(ClampMin="0.0"))
    float FlickerIntensityScale = 1.0f;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Material")
    bool bAffectMaterialParameter = true;

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Material")
    FName MaterialFlickerParamName = TEXT("FlickerState");

    UPROPERTY(EditAnywhere, Category="Light|Flicker|Debug")
    bool bEnableDebugLog = false;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    bool bIsRunning = false;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    bool bStartCallsBlocked = false;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    ETheseusLightFlickerRunMode CurrentRunMode = ETheseusLightFlickerRunMode::None;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    ETheseusLightFlickerMode CurrentMode = ETheseusLightFlickerMode::StableOn;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    int32 BurstTargetBlinkCount = 0;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    int32 BurstCompletedBlinkCount = 0;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    float BurstIntervalSeconds = 0.06f;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    float CachedOriginalIntensity = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="Light|Flicker|Runtime")
    bool bCachedOriginalVisibility = true;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CachedMID = nullptr;

    UPROPERTY(Transient)
    bool bLoggedMissingMaterialSlots = false;

    UPROPERTY(Transient)
    bool bLoggedIndexFallback = false;

    FTimerHandle ModeTimerHandle;
};
