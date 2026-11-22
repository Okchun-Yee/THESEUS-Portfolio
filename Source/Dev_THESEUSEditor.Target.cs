// Copyright 2026, Okchun-Yee.

using UnrealBuildTool;
using System.Collections.Generic;

public class Dev_THESEUSEditorTarget : TargetRules
{
	public Dev_THESEUSEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

        ExtraModuleNames.AddRange( new string[] { "Dev_THESEUS" } );
	}
}
