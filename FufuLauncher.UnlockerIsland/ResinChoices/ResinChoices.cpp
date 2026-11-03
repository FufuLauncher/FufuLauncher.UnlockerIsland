/*
Copyright (c) FufuLauncher Dev Team. All rights reserved.
Licensed under the AGPL-3.0 License.
*/
#include "ResinChoices.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "../Config/Config.h"
#include "../Core/SharedState.h"
#include "../Core/Utils.h"
#include "../MinHook/MinHook.h"
#include "../Patterns/Patterns.h"
#include "../Scanner/Scanner.h"
#include "../Visual/Visual.h"
#include "../il2cpp/Il2CppList.h"

namespace ResinChoices
{
    namespace
    {
        // Bit order is shared with Config::GetResinChoiceMask().
        constexpr std::array<uint32_t, 5> kItemIds{106, 220007, 107012, 107009, 201};
        constexpr uint32_t kInventoryChanged = 1, kChoicesChanged = 2, kWindowShown = 4;
        using RefreshList = void (WINAPI*)(void*, bool);
        using ContextAction = void (WINAPI*)(void*);
        using ShowContext = void (WINAPI*)(void*, bool);
        using InventoryChanged = void (WINAPI*)(void*, uint32_t);
        using GetItemCount = int64_t (WINAPI*)(void*, uint32_t);

        uint32_t g_ListOffset = 0, g_ViewOffset = 0, g_GameObjectOffset = 0, g_InventoryOffset = 0;
        void** g_ModuleTable = nullptr;
        RefreshList g_RefreshList = nullptr;
        ContextAction g_OriginalDisable = nullptr;
        ShowContext g_OriginalShow = nullptr;
        InventoryChanged g_OriginalInventoryChanged = nullptr;
        GetItemCount g_GetItemCount = nullptr;
        std::atomic<bool> g_Ready{false}, g_Posted{false};
        std::atomic<uint32_t> g_Reasons{0};
        std::atomic<DWORD> g_UiThread{0};
        UINT g_Message = 0;
        HHOOK g_MessageHook = nullptr;
        bool g_Refreshing = false, g_Deferred = false;

        struct Snapshot
        {
            void* inventory = nullptr;
            void* view = nullptr;
            uint32_t available = 0;
            uint32_t choices = 0;

            bool operator==(const Snapshot& other) const
            {
                return inventory == other.inventory && view == other.view &&
                    available == other.available && choices == other.choices;
            }
        };

        struct WindowState
        {
            void* context = nullptr;
            void* klass = nullptr;
            Snapshot applied;
            bool valid = false;
            bool failed = false;
        } g_Window;

        bool ReadPointer(void* object, uint32_t offset, void*& value)
        {
            if (!object) return false;
            auto** field = reinterpret_cast<void**>(static_cast<uint8_t*>(object) + offset);
            if (IsBadReadPtr(field, sizeof(*field))) return false;
            value = *field;
            return value != nullptr;
        }

        Il2CppList<uint64_t>* GetList(void* context)
        {
            void* value = nullptr;
            if (!g_ListOffset || !ReadPointer(context, g_ListOffset, value)) return nullptr;
            auto* list = static_cast<Il2CppList<uint64_t>*>(value);
            if (IsBadReadPtr(list, sizeof(*list)) || IsBadWritePtr(list, sizeof(*list))) return nullptr;
            const int count = list->Count();
            if (count < 0 || count > 32) return nullptr;
            auto* items = list->Items();
            if (!items || IsBadReadPtr(items, 0x20)) return nullptr;
            const auto capacity = items->Count();
            if (capacity < static_cast<size_t>(count) || capacity > 1000) return nullptr;
            auto* values = reinterpret_cast<uint8_t*>(items) + 0x20;
            if (count && (IsBadReadPtr(values, count * sizeof(uint64_t)) ||
                IsBadWritePtr(values, count * sizeof(uint64_t))))
                return nullptr;
            return list;
        }

        uint32_t ChoiceMask(uint64_t item)
        {
            for (size_t i = 0; i < kItemIds.size(); ++i)
                if (static_cast<uint32_t>(item) == kItemIds[i] ||
                    static_cast<uint32_t>(item >> 32) == kItemIds[i])
                    return 1u << i;
            return 0;
        }

        bool ApplyChoices(void* context)
        {
            auto* list = GetList(context);
            if (!list) return false;
            const uint32_t choices = Config::GetResinChoiceMask();
            uint32_t nativeMask = 0, filteredMask = 0;
            for (int i = list->Count() - 1; i >= 0; --i)
            {
                const uint32_t bit = ChoiceMask(list->Get(i));
                nativeMask |= bit;
                if (bit && !(choices & bit)) list->RemoveAt(i);
                else filteredMask |= bit;
            }
            if (Config::Get().debug_console)
                std::cout << "[Resin] choices=" << choices << " native=" << nativeMask
                    << " filtered=" << filteredMask << '\n';
            return true;
        }

        bool IsVisible(void* context, void*& view)
        {
            void* nativeView = nullptr;
            void* gameObject = nullptr;
            void* nativeObject = nullptr;
            auto getActive = reinterpret_cast<tGetActive>(p_GetActive.load());
            return getActive && ReadPointer(context, g_ViewOffset, view) &&
                ReadPointer(view, 0x10, nativeView) &&
                ReadPointer(context, g_GameObjectOffset, gameObject) &&
                ReadPointer(gameObject, 0x10, nativeObject) && getActive(gameObject);
        }

        bool TakeSnapshot(void* view, Snapshot& snapshot)
        {
            void* table = nullptr;
            if (!g_GetItemCount || !ReadPointer(g_ModuleTable, 0, table) ||
                !ReadPointer(table, g_InventoryOffset, snapshot.inventory))
                return false;
            snapshot.view = view;
            snapshot.choices = Config::GetResinChoiceMask();
            for (size_t i = 0; i < kItemIds.size(); ++i)
            {
                const int64_t count = g_GetItemCount(snapshot.inventory, kItemIds[i]);
                if (count < 0) return false;
                if (count > 0) snapshot.available |= 1u << i;
            }
            return true;
        }

        void PostRequest(uint32_t reason)
        {
            if (!g_Ready.load(std::memory_order_acquire)) return;
            g_Reasons.fetch_or(reason, std::memory_order_acq_rel);
            const DWORD thread = g_UiThread.load(std::memory_order_acquire);
            if (!thread || g_Posted.exchange(true)) return;
            if (!PostThreadMessageW(thread, g_Message, 0, 0))
            {
                g_Posted.store(false);
                OutputDebugStringA("[Resin] Could not post native UI refresh request.\n");
            }
        }

        bool TryRefresh(void* context)
        {
            __try
            {
                // Native generation passes through HookSetupList before controls
                // are rebound. Never add choices absent from the native list.
                g_RefreshList(context, false);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        }

        void ProcessRequest()
        {
            if (GetCurrentThreadId() != g_UiThread.load(std::memory_order_acquire)) return;
            g_Posted.store(false);
            if (g_Refreshing)
            {
                g_Deferred = true;
                return;
            }
            const uint32_t reasons = g_Reasons.exchange(0);
            if (!reasons || !g_Window.context || g_Window.failed) return;
            void* klass = nullptr;
            void* view = nullptr;
            if (!ReadPointer(g_Window.context, 0, klass) || klass != g_Window.klass ||
                !IsVisible(g_Window.context, view))
                return;
            Snapshot snapshot;
            if (!TakeSnapshot(view, snapshot))
            {
                g_Window.valid = false;
                return;
            }
            if (!(reasons & kWindowShown) && g_Window.valid && snapshot == g_Window.applied) return;

            // Events are delivered through this thread's message queue, after
            // the inventory transaction or background configuration reload ends.
            void* context = g_Window.context;
            g_Refreshing = true;
            const bool refreshed = TryRefresh(context);
            g_Refreshing = false;
            if (g_Window.context == context)
            {
                g_Window.failed = !refreshed;
                g_Window.valid = refreshed;
                if (refreshed) g_Window.applied = snapshot;
            }
            if (!refreshed)
                std::cout << "[Resin] Native refresh failed; waiting for window reopen.\n";
            else if (Config::Get().debug_console)
                std::cout << "[Resin] Event refresh; available=" << snapshot.available
                    << " choices=" << snapshot.choices << '\n';
            if (g_Deferred)
            {
                g_Deferred = false;
                PostRequest(0);
            }
        }

        LRESULT CALLBACK OnMessage(int code, WPARAM remove, LPARAM param)
        {
            if (code >= 0 && remove == PM_REMOVE && param)
            {
                const auto* message = reinterpret_cast<const MSG*>(param);
                if (message->message == g_Message)
                {
                    __try { ProcessRequest(); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { g_Window = {}; }
                }
            }
            return CallNextHookEx(g_MessageHook, code, remove, param);
        }

        bool EnsureDispatcher()
        {
            const DWORD current = GetCurrentThreadId();
            if (g_UiThread.load() != 0) return g_UiThread.load() == current;
            g_MessageHook = SetWindowsHookExW(WH_GETMESSAGE, OnMessage, nullptr, current);
            if (!g_MessageHook) return false;
            g_UiThread.store(current, std::memory_order_release);
            return true;
        }

        void TrackContext(void* context, bool shown)
        {
            if (!g_Ready.load() || g_Refreshing) return;
            void* view = nullptr;
            void* klass = nullptr;
            if (!ReadPointer(context, 0, klass) || !IsVisible(context, view) || !EnsureDispatcher()) return;
            if (g_Window.context != context || (g_Window.valid && g_Window.applied.view != view))
                g_Window = {context, klass};
            if (shown) g_Window.failed = false;
            PostRequest(shown || !g_Window.valid ? kWindowShown : kInventoryChanged);
        }

        void WINAPI HookSetupList(void* context)
        {
            auto original = reinterpret_cast<tSetupResinList>(o_SetupResinList.load());
            if (!original) return;
            original(context);
            __try
            {
                if (ApplyChoices(context)) TrackContext(context, false);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        void WINAPI HookContextShow(void* context, bool shown)
        {
            if (!shown && g_Window.context == context) g_Window = {};
            if (g_OriginalShow) g_OriginalShow(context, shown);
            if (!shown) return;
            __try { TrackContext(context, true); }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        void WINAPI HookContextDisable(void* context)
        {
            // Clear the reference before the native lifecycle releases the view.
            if (g_Window.context == context) g_Window = {};
            if (g_OriginalDisable) g_OriginalDisable(context);
        }

        void WINAPI HookInventoryChanged(void* inventory, uint32_t storeType)
        {
            if (g_OriginalInventoryChanged) g_OriginalInventoryChanged(inventory, storeType);
            // This native BagSlotUpdate publisher runs after the add/update/delete
            // batch commits. No timer or per-frame inventory query is installed.
            PostRequest(kInventoryChanged);
        }

        bool InitRefresh(void* setupCall, void* setup)
        {
            auto* refresh = static_cast<uint8_t*>(setupCall) - 0x46;
            if (Scanner::ScanRange(refresh, 0x4D, Patterns::RefreshResinList) != refresh) return false;
            auto* windowBase = static_cast<uint8_t*>(setup) - 0x2000;
            auto* setupView = static_cast<uint8_t*>(Scanner::ScanRange(windowBase, 0x4000, Patterns::ResinSetupView));
            auto* inventory = static_cast<uint8_t*>(Scanner::ScanRange(setup, 0x400, Patterns::ResinItemCount));
            void* disable = Scanner::ScanRange(windowBase, 0x4000, Patterns::ResinContextDisable);
            void* show = Scanner::ScanMainMod(Patterns::ResinContextShow);
            void* changed = Scanner::ScanMainMod(Patterns::ResinInventoryChanged);
            if (!setupView || !inventory || !show || !disable || !changed ||
                !IsValidCodePtr(p_GetActive.load()))
                return false;
            if (Scanner::ResolveRelative(setupView + 0x60, 1, 5) != refresh) return false;
            uint32_t viewOffset = 0, storedViewOffset = 0, inventoryOffset = 0;
            memcpy(&viewOffset, refresh + 0x2B, sizeof(viewOffset));
            memcpy(&storedViewOffset, setupView + 0x46, sizeof(storedViewOffset));
            memcpy(&inventoryOffset, inventory + 0x10, sizeof(inventoryOffset));
            if (viewOffset != storedViewOffset || viewOffset < 0x20 || viewOffset > 0x1000 || viewOffset % 8 ||
                inventoryOffset < 0x20 || inventoryOffset > 0x1000000 || inventoryOffset % 8)
                return false;
            void* count = Scanner::ResolveRelative(inventory + 0x22, 1, 5);
            const uint8_t prologue[]{0x56, 0x57, 0x48, 0x83, 0xEC, 0x28, 0x89, 0xD6, 0x48, 0x89, 0xCF};
            if (!count || IsBadReadPtr(count, sizeof(prologue)) || memcmp(count, prologue, sizeof(prologue))) return
                false;
            auto** moduleTable = reinterpret_cast<void**>(Scanner::ResolveRelative(inventory, 3, 7));
            if (!moduleTable || IsBadReadPtr(moduleTable, sizeof(*moduleTable))) return false;
            g_Message = RegisterWindowMessageW(L"FufuLauncher.UnlockerIsland.ResinChoices.Refresh");
            if (!g_Message) return false;
            if (MH_CreateHook(show, reinterpret_cast<void*>(HookContextShow),
                              reinterpret_cast<void**>(&g_OriginalShow)) != MH_OK)
                return false;
            if (MH_CreateHook(disable, reinterpret_cast<void*>(HookContextDisable),
                              reinterpret_cast<void**>(&g_OriginalDisable)) != MH_OK)
            {
                MH_RemoveHook(show);
                return false;
            }
            if (MH_CreateHook(changed, reinterpret_cast<void*>(HookInventoryChanged),
                              reinterpret_cast<void**>(&g_OriginalInventoryChanged)) != MH_OK)
            {
                MH_RemoveHook(disable);
                MH_RemoveHook(show);
                return false;
            }
            g_ViewOffset = viewOffset;
            g_GameObjectOffset = setupView[0x2A];
            g_InventoryOffset = inventoryOffset;
            g_ModuleTable = moduleTable;
            g_GetItemCount = reinterpret_cast<GetItemCount>(count);
            g_RefreshList = reinterpret_cast<RefreshList>(refresh);
            g_Ready.store(true, std::memory_order_release);
            LogOffset("RefreshResinList", refresh, setupCall);
            LogOffset("ResinContext.Show", show, show);
            LogOffset("ResinContext.OnDisable", disable, disable);
            LogOffset("ResinInventoryChanged", changed, changed);
            LogOffset("ResinItemCount", count, inventory + 0x22);
            return true;
        }
    }

    void ChoicesChanged() { PostRequest(kChoicesChanged); }

    void Init()
    {
        std::cout << "[SCAN] SetupResinList...\n";
        void* call = Scanner::ScanMainMod(Patterns::SetupResinList);
        auto* setup = static_cast<uint8_t*>(call ? Scanner::ResolveRelative(call, 1, 5) : nullptr);
        if (setup && !IsBadReadPtr(setup, 0x2E))
        {
            const uint8_t prologue[]{0x56, 0x57, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x89, 0xCB};
            const uint8_t listRead[]{0x48, 0x8B, 0x8B};
            const uint8_t listCheck[]{0x48, 0x85, 0xC9, 0x0F, 0x84};
            const uint8_t sizeReset[]{0xC7, 0x41, 0x18, 0, 0, 0, 0};
            uint32_t offset = 0;
            memcpy(&offset, setup + 0x1A, sizeof(offset));
            if (!memcmp(setup, prologue, sizeof(prologue)) &&
                !memcmp(setup + 0x17, listRead, sizeof(listRead)) &&
                !memcmp(setup + 0x1E, listCheck, sizeof(listCheck)) &&
                !memcmp(setup + 0x27, sizeReset, sizeof(sizeReset)) &&
                offset >= 0x20 && offset <= 0x1000 && offset % 8 == 0 &&
                MH_CreateHook(setup, reinterpret_cast<void*>(HookSetupList),
                              reinterpret_cast<void**>(&o_SetupResinList)) == MH_OK)
            {
                g_ListOffset = offset;
                LogOffset("SetupResinList", setup, call);
                std::cout << "   -> Hook Ready; list field: 0x" << std::hex << offset << std::dec << '\n';
                std::cout << (InitRefresh(call, setup)
                                  ? "   -> Resin availability events ready.\n"
                                  : "   -> [WARN] Resin event dependencies unsupported; native filtering retained.\n");
                return;
            }
        }
        std::cout << "   -> [WARN] Signature or list layout unsupported; choices unchanged.\n";
    }
}
