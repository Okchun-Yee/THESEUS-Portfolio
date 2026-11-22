// Copyright 2026, Okchun-Yee.

#include "Common/Trigger/LightFlickerModeComponent.h"

#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

ULightFlickerModeComponent::ULightFlickerModeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULightFlickerModeComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveTargetsIfNeeded();

    if (TargetLight)
    {
        CachedOriginalIntensity = TargetLight->Intensity;
        bCachedOriginalVisibility = TargetLight->IsVisible();
    }

    EnsureDynamicMaterialInstance();

    if (bAutoStart)
    {
        StartFlickerLoop();
    }
}

void ULightFlickerModeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAndRestore();
    Super::EndPlay(EndPlayReason);
}

bool ULightFlickerModeComponent::StartFlickerLoop()
{
    if (bStartCallsBlocked)
    {
        if (bEnableDebugLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[LightFlickerMode] StartFlickerLoop blocked. Owner=%s"), *GetNameSafe(GetOwner()));
        }
        return false;
    }

    if (bIsRunning)
    {
        if (bEnableDebugLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[LightFlickerMode] Start skipped: already running. Owner=%s"), *GetNameSafe(GetOwner()));
        }
        return false;
    }

    ResolveTargetsIfNeeded();

    if (!TargetLight)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LightFlickerMode] Start failed: TargetLight is null. Owner=%s"), *GetNameSafe(GetOwner()));
        return false;
    }

    bIsRunning = true;
    CurrentRunMode = ETheseusLightFlickerRunMode::Loop;
    BurstTargetBlinkCount = 0;
    BurstCompletedBlinkCount = 0;
    CurrentMode = InitialMode;

    ApplyCurrentMode();
    ScheduleNextToggle();
    return true;
}

bool ULightFlickerModeComponent::StartFlickerBurst(int32 BlinkCount, float IntervalSeconds)
{
    if (bStartCallsBlocked)
    {
        if (bEnableDebugLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[LightFlickerMode] StartFlickerBurst blocked. Owner=%s"), *GetNameSafe(GetOwner()));
        }
        return false;
    }

    if (bIsRunning)
    {
        if (bEnableDebugLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[LightFlickerMode] Burst start skipped: already running. Owner=%s"), *GetNameSafe(GetOwner()));
        }
        return false;
    }

    ResolveTargetsIfNeeded();

    if (!TargetLight)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LightFlickerMode] Burst start failed: TargetLight is null. Owner=%s"), *GetNameSafe(GetOwner()));
        return false;
    }

    bIsRunning = true;
    CurrentRunMode = ETheseusLightFlickerRunMode::Burst;
    BurstTargetBlinkCount = FMath::Max(1, BlinkCount);
    BurstCompletedBlinkCount = 1;
    BurstIntervalSeconds = FMath::Max(0.01f, IntervalSeconds);
    CurrentMode = ETheseusLightFlickerMode::FlickerOn;

    ApplyCurrentMode();
    ScheduleNextToggle();
    return true;
}

void ULightFlickerModeComponent::StopAndRestore()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ModeTimerHandle);
    }

    bIsRunning = false;
    CurrentRunMode = ETheseusLightFlickerRunMode::None;
    BurstTargetBlinkCount = 0;
    BurstCompletedBlinkCount = 0;

    if (TargetLight)
    {
        TargetLight->SetIntensity(CachedOriginalIntensity);
        TargetLight->SetVisibility(bCachedOriginalVisibility);
    }

    SetMaterialFlickerState(0.0f);
}

void ULightFlickerModeComponent::ResolveTargetsIfNeeded()
{
    AActor* Owner = GetOwner();
    if (!Owner || !bAutoResolveTargets)
    {
        return;
    }

    if (!TargetLight)
    {
        TargetLight = Owner->FindComponentByClass<ULightComponent>();
    }

    if (!TargetMesh)
    {
        TargetMesh = Owner->FindComponentByClass<UMeshComponent>();
    }
}

void ULightFlickerModeComponent::ApplyCurrentMode()
{
    if (!TargetLight)
    {
        return;
    }

    if (CurrentMode == ETheseusLightFlickerMode::StableOn)
    {
        TargetLight->SetIntensity(CachedOriginalIntensity);
        TargetLight->SetVisibility(true);
        SetMaterialFlickerState(0.0f);
    }
    else
    {
        const float FlickerIntensity = GetRandomFlickerIntensity();
        TargetLight->SetIntensity(FlickerIntensity);
        TargetLight->SetVisibility(true);
        SetMaterialFlickerState(1.0f);
    }

    if (bEnableDebugLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[LightFlickerMode] Apply mode: %s Intensity=%.2f"),
            CurrentMode == ETheseusLightFlickerMode::StableOn ? TEXT("StableOn") : TEXT("FlickerOn"),
            TargetLight->Intensity);
    }

    OnModeChanged.Broadcast(CurrentMode);
}

void ULightFlickerModeComponent::ScheduleNextToggle()
{
    if (!bIsRunning)
    {
        return;
    }

    const float DelaySeconds = (CurrentRunMode == ETheseusLightFlickerRunMode::Burst)
        ? BurstIntervalSeconds
        : GetRandomDurationForMode(CurrentMode);
    if (DelaySeconds <= 0.0f)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ModeTimerHandle);
        World->GetTimerManager().SetTimer(
            ModeTimerHandle,
            this,
            &ULightFlickerModeComponent::HandleToggleTimerElapsed,
            DelaySeconds,
            false);
    }

    if (bEnableDebugLog)
    {
        UE_LOG(LogTemp, Log, TEXT("[LightFlickerMode] Schedule next toggle: Delay=%.3fs CurrentMode=%s"),
            DelaySeconds,
            CurrentMode == ETheseusLightFlickerMode::StableOn ? TEXT("StableOn") : TEXT("FlickerOn"));
    }
}

void ULightFlickerModeComponent::HandleToggleTimerElapsed()
{
    if (!bIsRunning)
    {
        return;
    }

    if (CurrentRunMode == ETheseusLightFlickerRunMode::Burst)
    {
        if (CurrentMode == ETheseusLightFlickerMode::FlickerOn)
        {
            CurrentMode = ETheseusLightFlickerMode::StableOn;
            ApplyCurrentMode();

            if (BurstCompletedBlinkCount >= BurstTargetBlinkCount)
            {
                StopAndRestore();
                return;
            }
        }
        else
        {
            CurrentMode = ETheseusLightFlickerMode::FlickerOn;
            ++BurstCompletedBlinkCount;
            ApplyCurrentMode();
        }

        ScheduleNextToggle();
        return;
    }

    CurrentMode = (CurrentMode == ETheseusLightFlickerMode::StableOn)
        ? ETheseusLightFlickerMode::FlickerOn
        : ETheseusLightFlickerMode::StableOn;

    ApplyCurrentMode();
    ScheduleNextToggle();
}

float ULightFlickerModeComponent::GetRandomDurationForMode(ETheseusLightFlickerMode Mode) const
{
    float MinDuration = 0.05f;
    float MaxDuration = 0.10f;

    if (Mode == ETheseusLightFlickerMode::StableOn)
    {
        MinDuration = StableMinDuration;
        MaxDuration = StableMaxDuration;
    }
    else
    {
        MinDuration = FlickerMinDuration;
        MaxDuration = FlickerMaxDuration;
    }

    if (MinDuration > MaxDuration)
    {
        Swap(MinDuration, MaxDuration);
    }

    return FMath::FRandRange(FMath::Max(0.001f, MinDuration), FMath::Max(0.001f, MaxDuration));
}

float ULightFlickerModeComponent::GetRandomFlickerIntensity() const
{
    if (!bRandomizeFlickerIntensity)
    {
        return CachedOriginalIntensity * FMath::Max(0.0f, FlickerIntensityScale);
    }

    float MinIntensity = FlickerIntensityMin;
    float MaxIntensity = FlickerIntensityMax;
    if (MinIntensity > MaxIntensity)
    {
        Swap(MinIntensity, MaxIntensity);
    }

    return FMath::FRandRange(FMath::Max(0.0f, MinIntensity), FMath::Max(0.0f, MaxIntensity));
}

void ULightFlickerModeComponent::SetMaterialFlickerState(float Value)
{
    if (!bAffectMaterialParameter)
    {
        return;
    }

    EnsureDynamicMaterialInstance();

    if (CachedMID)
    {
        CachedMID->SetScalarParameterValue(MaterialFlickerParamName, Value);
    }
}

void ULightFlickerModeComponent::EnsureDynamicMaterialInstance()
{
    if (CachedMID || !TargetMesh || MaterialElementIndex < 0)
    {
        return;
    }

    const int32 NumMaterials = TargetMesh->GetNumMaterials();
    if (NumMaterials <= 0)
    {
        if (!bLoggedMissingMaterialSlots)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[LightFlickerMode] MID create skipped: TargetMesh has no material slots. Owner=%s Mesh=%s"),
                *GetNameSafe(GetOwner()),
                *GetNameSafe(TargetMesh));
            bLoggedMissingMaterialSlots = true;
        }
        return;
    }

    const int32 SafeMaterialIndex = FMath::Clamp(MaterialElementIndex, 0, NumMaterials - 1);
    if (SafeMaterialIndex != MaterialElementIndex && !bLoggedIndexFallback)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LightFlickerMode] MaterialElementIndex %d is out of range (NumMaterials=%d). Fallback index=%d. Owner=%s"),
            MaterialElementIndex,
            NumMaterials,
            SafeMaterialIndex,
            *GetNameSafe(GetOwner()));
        bLoggedIndexFallback = true;
    }

    CachedMID = TargetMesh->CreateDynamicMaterialInstance(SafeMaterialIndex);
}
