#include <filesystem>
#include <fstream>
#include "hooks.h"
#include "game.h"
#include "nvse_api.h"

namespace
{
  using Callback = void(__cdecl *)();
  std::ofstream logFile;
  constexpr uintptr_t kOriginalFastTravelClosePipBoyCall = 0x79872E, kOriginalFastTravelDoneCall = 0x93BF2B, kOriginalLoadHudCall = 0x457F37, kOriginalLoadHudTimeoutCall = 0x457F8B;

  void RefreshSceneAndMapAfterFastTravel()
  {
    // check if pipboy world map is open (in case we did fast travel via some other method)
    auto *mapMenu = Game::GetMapMenu();
    auto *uiManager = Game::GetInterfaceManager();
    if (!mapMenu || !uiManager || !Game::IsPipBoyOpen(uiManager) || !Game::IsWorldMapTabActive(mapMenu))
      return;
    Game::RefreshSkyLighting();
    Game::SetupActiveMapTab(mapMenu);
    Game::RequestPipBoyBackgroundCapture();
  }

  void __cdecl RestoreHudAfterLoading(uint32_t state)
  {
    // check if pipboy world map is open (in case we did fast travel via some other method)
    auto *uiManager = Game::GetInterfaceManager();
    if (uiManager && Game::IsPipBoyOpen(uiManager))
      state = Game::kPipBoyHudState;
    Game::UpdateHudVisibilityState(state);
  }

  // Replace closing with its normal travel continuation. Retains thiscall ret 4.
  void __fastcall SkipPipBoyCloseForFastTravel(void *, void *, Callback callback)
  {
    if (callback)
      callback();
  }

  // Run after the fast-travel call, then resume its original cleanup jump.
  __declspec(naked) void HandleFastTravelDone()
  {
    __asm {
      pushfd
      pushad
      call RefreshSceneAndMapAfterFastTravel
      popad
      popfd
      push 0x93C171
      ret
    }
  }

  void InstallHooks(uintptr_t originalFastTravelClosePipBoyCall = kOriginalFastTravelClosePipBoyCall, uintptr_t originalFastTravelDoneCall = kOriginalFastTravelDoneCall,
                    uintptr_t originalLoadHudCall = kOriginalLoadHudCall, uintptr_t originalLoadHudTimeoutCall = kOriginalLoadHudTimeoutCall)
  {
    WriteRelCall(originalLoadHudCall, &RestoreHudAfterLoading);
    WriteRelCall(originalLoadHudTimeoutCall, &RestoreHudAfterLoading);
    WriteRelJump(originalFastTravelDoneCall, &HandleFastTravelDone);
    WriteRelCall(originalFastTravelClosePipBoyCall, &SkipPipBoyCloseForFastTravel);
  }

  void HandleNvseMessage(Messaging::Message *message)
  {
    if (message->type == 18)
      InstallHooks(); // DeferredInit
  }
}

extern "C" bool __cdecl NVSEPlugin_Query(const NVSEInterface *nvse, PluginInfo *info)
{
  info->infoVersion = 1;
  info->name = "Keep Pip-Boy Open During Fast Travel";
  info->version = 16;
  return nvse && !nvse->isEditor && !nvse->isNogore &&
         nvse->runtimeVersion == 0x040020D0 && nvse->nvseVersion >= 6;
}
extern "C" bool __cdecl NVSEPlugin_Load(const NVSEInterface *nvse)
{
  auto *nvseMessaging = static_cast<Messaging *>(nvse->QueryInterface(2));
  return nvseMessaging && nvseMessaging->RegisterListener(nvse->GetPluginHandle(), "NVSE", HandleNvseMessage);
}
