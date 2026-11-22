// Copyright 2026, Okchun-Yee.


#include "Sequence/TickerPaperActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sequence/TickerPaperMotionComponent.h"

ATickerPaperActor::ATickerPaperActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    PaperMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PaperMesh"));
    PaperMesh->SetupAttachment(Root);

    PaperMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PaperMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
    PaperMesh->SetNotifyRigidBodyCollision(true);
    PaperMesh->SetSimulatePhysics(false);
    PaperMesh->SetEnableGravity(false);

    MotionComponent = CreateDefaultSubobject<UTickerPaperMotionComponent>(TEXT("TickerPaperMotionComponent"));

}

void ATickerPaperActor::InitializeFromRequest(
    const FTickerPrintRequest& Request,
    const FVector& SpawnOrigin,
    const FRotator& SpawnRotation,
    USceneComponent* InFollowSourceComponent,
    UMaterialInterface* InPaperMaterial)
{
    SetActorLocation(SpawnOrigin);
    SetActorRotation(SpawnRotation);

    if (MotionComponent && PaperMesh)
    {
        MotionComponent->InitPaper(PaperMesh, Request, SpawnOrigin, SpawnRotation.Vector(), SpawnRotation, InFollowSourceComponent);
    }

    if (PaperMesh && InPaperMaterial)
    {
        PaperMesh->SetMaterial(0, InPaperMaterial);
    }
}

