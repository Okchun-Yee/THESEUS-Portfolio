// Copyright 2026, Okchun-Yee.

#include "Sequence/TheseusEventBusSubsystem.h"

DEFINE_LOG_CATEGORY(LogTheseusEventBus);

void UTheseusEventBusSubsystem::PublishTickerPrintRequest(const FTickerPrintRequest& Request)
{
    if (Request.Message.IsEmpty())
    {
        UE_LOG(LogTheseusEventBus, Warning, TEXT("PublishTickerPrintRequest skipped: empty message."));
        return;
    }

    UE_LOG(
        LogTheseusEventBus,
        Log,
        TEXT("PublishTickerPrintRequest: Message=\"%s\", ReasonTag=\"%s\", EjectDistance=%.2f, LifeTime=%.2f, Instigator=%s"),
        *Request.Message.ToString(),
        *Request.ReasonTag.ToString(),
        Request.EjectDistance,
        Request.LifeTime,
        Request.InstigatorActor ? *Request.InstigatorActor->GetName() : TEXT("None"));

    OnTickerPrintRequested.Broadcast(Request);
}
