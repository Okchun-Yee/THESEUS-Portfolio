// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Sequence/TickerPaperTypes.h"
#include "TickerPaperMotionComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;

UCLASS(ClassGroup=(Theseus), meta=(BlueprintSpawnableComponent))
class DEV_THESEUS_API UTickerPaperMotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTickerPaperMotionComponent();

    void InitPaper(
        UPrimitiveComponent* InPaperPrimitive,
        const FTickerPrintRequest& InRequest,
        const FVector& InStart,
        const FVector& InForward,
        const FRotator& InStartRotation,
        USceneComponent* InFollowSourceComponent = nullptr);

    void StartFlutter();
    void StopFlutter();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UFUNCTION()
    void HandlePaperHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        FVector NormalImpulse,
        const FHitResult& Hit);

    void EnablePhysicsPhase();
    void DestroyOwner();

private:
    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> PaperPrimitive = nullptr;

    FTickerPrintRequest CachedRequest;
    FVector StartLocation = FVector::ZeroVector;
    FVector ForwardDir = FVector::ForwardVector;
    FRotator StartRotation = FRotator::ZeroRotator;
    TWeakObjectPtr<USceneComponent> FollowSourceComponent;
    float EjectElapsed = 0.0f;

    UPROPERTY(EditAnywhere, Category="Ticker|Motion")
    float EjectDuration = 0.18f;

    UPROPERTY(EditAnywhere, Category="Ticker|Motion")
    float FlutterForce = 25.0f;

    UPROPERTY(EditAnywhere, Category="Ticker|Motion")
    float FlutterTorque = 6.0f;

    UPROPERTY(EditAnywhere, Category="Ticker|Motion")
    float FlutterFrequency = 10.0f;

    UPROPERTY(EditAnywhere, Category="Ticker|Motion")
    float HitIgnoreTimeAfterPhysics = 0.35f;

    bool bPhysicsEnabled = false;
    bool bFluttering = false;
    float PhysicsEnabledAtTime = -1.0f;

    FTimerHandle LifeTimerHandle;
    FTimerHandle EjectFailSafeTimerHandle;
};
