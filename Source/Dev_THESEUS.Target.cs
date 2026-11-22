// Copyright 2026, Okchun-Yee.

using UnrealBuildTool;
using System.Collections.Generic;

public class Dev_THESEUSTarget : TargetRules
{
	public Dev_THESEUSTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;

		ExtraModuleNames.AddRange( new string[] { "Dev_THESEUS" } );
	}
}
