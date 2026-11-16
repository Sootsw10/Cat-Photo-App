using UnrealBuildTool;

public class TheShelfTarget : TargetRules
{
	public TheShelfTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TheShelf");
	}
}
