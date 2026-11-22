// Copyright 2026, Okchun-Yee.

#include "Sequence/FM1024ProgressSubsystem.h"

DEFINE_LOG_CATEGORY(LogFM1024Progress);

void UFM1024ProgressSubsystem::SetFM1024Acquired(bool bAcquired)
{
	if (bFM1024Acquired && bAcquired)
	{
		UE_LOG(LogFM1024Progress, Verbose, TEXT("SetFM1024Acquired ignored: already true."));
		return;
	}

	bFM1024Acquired = bAcquired;

	UE_LOG(
		LogFM1024Progress,
		Log,
		TEXT("SetFM1024Acquired: %s"),
		bFM1024Acquired ? TEXT("true") : TEXT("false"));
}

bool UFM1024ProgressSubsystem::IsFM1024Acquired() const
{
	return bFM1024Acquired;
}

void UFM1024ProgressSubsystem::ResetFM1024Progress()
{
	bFM1024Acquired = false;

	UE_LOG(LogFM1024Progress, Log, TEXT("ResetFM1024Progress: false"));
}
