using BepInEx;
using BepInEx.IL2CPP;
using System.Runtime.InteropServices;

namespace RF5ModLoader
{
    [BepInPlugin(PluginInfo.PLUGIN_GUID, PluginInfo.PLUGIN_NAME, PluginInfo.PLUGIN_VERSION)]
    public class Plugin : BasePlugin
    {
        public override void Load()
        {
            // Plugin startup logic
            Log.LogInfo($"Plugin {PluginInfo.PLUGIN_GUID} is loaded!");
            Initialize();
        }
        
        [DllImport("RF5ModLoader")]
        public static extern void Initialize();
    }
}
