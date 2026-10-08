#pragma once
#include <cstddef>
#include <cstdint>

// Native Fallout: New Vegas 1.4.0.525 functions and fields.
namespace Game
{
  constexpr uint32_t kPipBoyHudState = 3;

  template <class T>
  const T &GetObjectField(const void *object, size_t offset)
  {
    return *reinterpret_cast<const T *>(static_cast<const uint8_t *>(object) + offset);
  }

  inline void *GetMapMenu()
  {
    return *reinterpret_cast<void **>(0x11DA368);
  }

  inline void *GetInterfaceManager()
  {
    return reinterpret_cast<void *(__cdecl *)()>(0x4B7210)();
  }

  inline bool IsPipBoyOpen(const void *ui)
  {
    return GetObjectField<uint32_t>(ui, 0x4BC) == 3;
  }

  inline bool IsWorldMapTabActive(const void *mapMenu)
  {
    return GetObjectField<uint8_t>(mapMenu, 0x80) == 0x21;
  }

  inline void RefreshSkyLighting()
  {
    // Native sky update; refresh destination lighting without advancing time.
    auto *sky = *reinterpret_cast<void **>(0x11CCB78);
    reinterpret_cast<void(__thiscall *)(void *, float)>(0x63AC70)(sky, 0.0f);
  }

  inline void SetupActiveMapTab(void *mapMenu)
  {
    // Native tab setup; restores the active image and calls SetupWorldMap.
    reinterpret_cast<void(__thiscall *)(void *)>(0x79AF30)(mapMenu);
  }

  inline void RequestPipBoyBackgroundCapture()
  {
    // Native Pip-Boy opening flag; recapture the world background on the next render.
    *reinterpret_cast<uint8_t *>(0x11D890A) = 1;
  }

  inline void UpdateHudVisibilityState(uint32_t state)
  {
    reinterpret_cast<void(__cdecl *)(uint32_t)>(0x771700)(state);
  }
}
