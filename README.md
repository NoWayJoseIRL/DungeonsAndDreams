Lyra Sample Game
===

Content Information
----
When downloading Lyra Source code from git, the content folders are not included.

To make use of the Lyra source code, you will need to download the content from the Unreal Marketplace through the Epic Games Launcher.

For more information please visit the [Unreal Engine Lyra documentation](https://docs.unrealengine.com/5.0/en-US/lyra-sample-game-in-unreal-engine/)

Once installed, simply copy the content folder inside your solution directory and you should be good to go.


Additional Information
----
See the [Unreal Engine README](../../../README.md) at the root of the repository for [Licensing](../../../README.md#licensing) and [Contributing](../../../README.md#contributions) information.


NoWayJoseDev Notes

REMOVE  RUNTIME

# Make sure you start in your project directory,
#  For example:
cd C:\GitHub\UE5Root\DungeonsAndDreams # <-- REPLACE THIS WITH YOUR PROJECT DIRECTORY

# Plugin Code Name (NO SPACES)
$PluginName = "DADCore" # <-- REPLACE THIS WITH YOUR PLUGIN NAME

$RuntimeName = "DADCoreRuntime" # E.g. "DADGameRuntime", the name we want to replace

# cd into the plugin directory
cd "Plugins/GameFeatures/$PluginName"