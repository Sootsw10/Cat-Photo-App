using UnrealBuildTool;

public class TheShelfEditorTarget : TargetRules
{
	public TheShelfEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TheShelf");
	}
}
