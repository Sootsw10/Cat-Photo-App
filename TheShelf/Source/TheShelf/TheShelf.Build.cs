using UnrealBuildTool;

public class TheShelf : ModuleRules
{
	public TheShelf(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"AIModule", "Json", "JsonUtilities", "Slate", "SlateCore"
		});
	}
}
