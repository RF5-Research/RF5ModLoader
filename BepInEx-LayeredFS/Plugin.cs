using BepInEx;
using BepInEx.IL2CPP;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace LayeredFS
{
    [BepInPlugin(PluginInfo.PLUGIN_GUID, PluginInfo.PLUGIN_NAME, PluginInfo.PLUGIN_VERSION)]
    public class Plugin : BasePlugin
    {
        public override void Load()
        {
            // Plugin startup logic
            Log.LogInfo($"Plugin {PluginInfo.PLUGIN_GUID} is loaded!");
            //Task.Run(() => Initialize());
            Initialize();
        }
        
        [DllImport("LayeredFS")]
        public static extern void Initialize();
    }
}
