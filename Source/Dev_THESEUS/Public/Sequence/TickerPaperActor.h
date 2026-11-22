// Copyright 2026, Okchun-Yee.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sequence/TickerPaperTypes.h"
#include "TickerPaperActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTickerPaperMotionComponent;
class UMaterialInterface;

UCLASS()
class DEV_THESEUS_API ATickerPaperActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATickerPaperActor();

    void InitializeFromRequest(
        const FTickerPrintRequest& Request,
        const FVector& SpawnOrigin,
        const FRotator& SpawnRotation,
        USceneComponent* InFollowSourceComponent = nullptr,
        UMaterialInterface* InPaperMaterial = nullptr);

protected:
    UPROPERTY(VisibleAnywhere, Category="Ticker")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ticker")
    TObjectPtr<UStaticMeshComponent> PaperMesh;

    UPROPERTY(VisibleAnywhere, Category="Ticker")
    TObjectPtr<UTickerPaperMotionComponent> MotionComponent;


};
