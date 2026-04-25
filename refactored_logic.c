#include <stdbool.h>
#include <stdint.h>
#include <immintrin.h>
#include <math.h>
#include <string.h>

/**
 * Data structures representing game engine objects and cheat context.
 * Derived from memory offsets and access patterns in the decompiled code.
 */

typedef struct {
    uint64_t entity_list_ptr; // Offset 48
    uint64_t list_capacity;    // Offset 64 (part of slice/vec)
    uint64_t list_count;       // Offset 72
    uint8_t  padding[84];      // Gap
    uint8_t  status_flag;      // Offset 164
} CheatContext;

typedef struct {
    int32_t x;                // Offset 4
    int32_t y;                // Offset 8
    int32_t width;            // Offset 12
    int32_t height;           // Offset 16
} ViewportInfo;

// Internal Game Engine Function Prototypes (represented by fixed memory addresses in original)
typedef double (__fastcall *ProjectPoint_t)(uint64_t ctx, double scale, double y, double z, double p1, double p2, double p3, double p4);
typedef bool (__fastcall *GetEntityInfo_t)(uint64_t world, uint64_t ctx, uint64_t entity, void* out, uint64_t f1, uint64_t f2);

/**
 * Refactored sub_748C0: ValidatePointInGameViewport
 * This function handles WorldToScreen projection and ensures the point is within
 * the visible game window, accounting for DPI scaling.
 */
bool ValidatePointInGameViewport(
    double worldX, double worldY, double worldZ,
    double p4, double p5, double p6,
    __m128 m1, __m128 m2,
    uint64_t unused_ctx, uint64_t unused_handle,
    uint64_t world_ptr, uint64_t viewport_ptr)
{
    ViewportInfo view;
    // Original calls MEMORY[0x7FFFB44786E0] to populate viewport info
    // We simulate the structure population based on the original's v39 usage.
    memset(&view, 0, sizeof(view));

    // DPI Scaling Factor Logic
    double dpiScale = 1.0;
    typedef int (__fastcall *GetDpi_t)(uint64_t, uint64_t, uint64_t, uint64_t, int*, void*, double, double, double, double, double, double, double, double);
    GetDpi_t getDpiFunc = (GetDpi_t)0x850230; // Address from original unk_850230

    if (getDpiFunc) {
        int dpiValue = 0;
        if (getDpiFunc(viewport_ptr, world_ptr, 0, world_ptr, &dpiValue, &view, worldX, worldY, worldZ, p4, 0.0, 0.0, 0.0, 1.0) >= 0) {
            dpiScale = (double)dpiValue / 96.0;
        }
    }

    // Projection Logic (sub_66F917)
    ProjectPoint_t projectFunc = (ProjectPoint_t)0x66F917;

    // Calculate projected screen X coordinate
    double screenX = projectFunc(viewport_ptr, dpiScale, worldY, worldZ, p4, 0.0, 0.0, dpiScale);
    // Calculate projected screen Y coordinate
    double screenY = projectFunc(0, dpiScale, worldY, worldZ, p4, 0.0, 0.0, dpiScale);

    // Clamp coordinates to integer space
    int finalX = (int)fmin(fmax(screenX, -2147483648.0), 2147483647.0);
    int finalY = (int)fmin(fmax(screenY, -2147483648.0), 2147483647.0);

    // Bounds checking against the viewport rectangle
    // Offset mapping from original v15, v19, v18, v20
    if (finalX >= view.x && finalX <= (view.x + view.width)) {
        if (finalY >= view.y && finalY <= (view.y + view.height)) {
            return true;
        }
    }

    return false;
}

/**
 * Refactored sub_545D0: MainEntityProcessingLoop
 * High-performance entity iterator using SIMD instructions to quickly filter
 * valid pointers. Responsible for updating ESP/Aimbot state for all players/items.
 */
bool MainEntityProcessingLoop(
    double p1, double p2, double p3,
    double p4, double p5, double p6,
    __m128 m1, __m128 m2,
    uint64_t world_ptr, uint64_t unused_a10,
    uint64_t entity_list_base, char flags,
    uint64_t context_ptr, uint64_t action_id)
{
    CheatContext* ctx = (CheatContext*)context_ptr;

    // Check if the hack system is enabled (Status 3 often means disabled/initializing)
    if (ctx->status_flag == 3) return false;

    // The original code uses __readgsqword(0x58u) to access Thread Local Storage (TLS)
    // for synchronization or counting. We increment the 'session' counter.
    uint64_t tls_base = __readgsqword(0x58);
    uint64_t* frame_counter = (uint64_t*)(*(uint64_t*)tls_base + 240);
    *frame_counter += 1;

    // Entity List Iteration
    uint64_t* entityPointers = (uint64_t*)ctx->entity_list_ptr;
    uint64_t count = ctx->list_count;

    if (count == 0) return true;

    // Re-implementation of the SIMD bitmask iteration logic
    // This allows the cheat to process batches of 16 pointers at once.
    const __m128i* simdList = (const __m128i*)entityPointers;

    for (uint64_t i = 0; i < count; ++i) {
        // Load the entity list pointer using SIMD mask to skip nulls (simplified)
        // Original logic uses _mm_movemask_epi8 and tzcnt to find non-zero pointers.
        uint64_t currentEntity = entityPointers[i];

        if (currentEntity == 0 || currentEntity == world_ptr) continue;

        // Call Game Engine functions to validate entity (e.g., IsAlive, IsPlayer)
        // 0x7FFFB4492770 is the likely address for 'GetEntityProperties'
        GetEntityInfo_t getProperties = (GetEntityInfo_t)0x7FFFB4492770;

        uint64_t outputData[8];
        if (getProperties(world_ptr, context_ptr, currentEntity, outputData, 15, 15)) {
            // If entity is valid, call further engine functions to update its screen-space state
            // Addresses 0x7FFFB448AC10 and 0x7FFFB4488570 correspond to drawing/state updates.
            void (*updateRenderer)(uint64_t, uint64_t, uint64_t, void*) = (void*)0x7FFFB448AC10;
            updateRenderer(world_ptr, context_ptr, 0, outputData);
        }
    }

    return true;
}
