#include <stdbool.h>
#include <stdint.h>
#include <immintrin.h>
#include <math.h>
#include <string.h>

/**
 * Data structures representing game engine objects and cheat context.
 */

typedef struct {
    uint64_t entity_list_ptr; // Offset 48
    uint64_t list_capacity;    // Offset 64
    uint64_t list_count;       // Offset 72
    uint8_t  padding[84];
    uint8_t  status_flag;      // Offset 164
} CheatContext;

typedef struct {
    int32_t x, y, width, height;
} ViewportInfo;

// Entity property structure (hypothetical based on DayZ/Enfusion engine)
typedef struct {
    uint64_t vtable;
    uint32_t network_id;
    uint32_t type_id; // Used to identify if entity is a Player, Zombie, or Item
} EntityData;

// Internal Game Engine Function Prototypes
typedef double (__fastcall *ProjectPoint_t)(uint64_t ctx, double scale, double y, double z, double p1, double p2, double p3, double p4);
typedef bool (__fastcall *GetEntityInfo_t)(uint64_t world, uint64_t ctx, uint64_t entity, void* out, uint64_t f1, uint64_t f2);

/**
 * Refactored sub_748C0: ValidatePointInGameViewport
 */
bool ValidatePointInGameViewport(
    double worldX, double worldY, double worldZ,
    double p4, double p5, double p6,
    __m128 m1, __m128 m2,
    uint64_t unused_ctx, uint64_t unused_handle,
    uint64_t world_ptr, uint64_t viewport_ptr)
{
    ViewportInfo view = {0};
    double dpiScale = 1.0;

    // Project 3D -> 2D
    ProjectPoint_t projectFunc = (ProjectPoint_t)0x66F917;
    double screenX = projectFunc(viewport_ptr, dpiScale, worldY, worldZ, p4, 0.0, 0.0, dpiScale);
    double screenY = projectFunc(0, dpiScale, worldY, worldZ, p4, 0.0, 0.0, dpiScale);

    int finalX = (int)fmin(fmax(screenX, -2147483648.0), 2147483647.0);
    int finalY = (int)fmin(fmax(screenY, -2147483648.0), 2147483647.0);

    // Bounds checking
    if (finalX >= view.x && finalX <= (view.x + view.width)) {
        if (finalY >= view.y && finalY <= (view.y + view.height)) {
            return true;
        }
    }
    return false;
}

/**
 * Refactored sub_545D0: PlayerOnlyESPUpdate
 * Updated to filter only for human players.
 */
bool PlayerOnlyESPUpdate(
    double p1, double p2, double p3,
    double p4, double p5, double p6,
    __m128 m1, __m128 m2,
    uint64_t world_ptr, uint64_t unused_a10,
    uint64_t entity_list_base, char flags,
    uint64_t context_ptr, uint64_t action_id)
{
    CheatContext* ctx = (CheatContext*)context_ptr;
    if (ctx->status_flag == 3) return false;

    uint64_t* entityPointers = (uint64_t*)ctx->entity_list_ptr;
    uint64_t count = ctx->list_count;

    for (uint64_t i = 0; i < count; ++i) {
        uint64_t currentEntityPtr = entityPointers[i];
        if (currentEntityPtr == 0 || currentEntityPtr == world_ptr) continue;

        // Get basic entity info from the game engine
        GetEntityInfo_t getProperties = (GetEntityInfo_t)0x7FFFB4492770;
        EntityData entData;

        if (getProperties(world_ptr, context_ptr, currentEntityPtr, &entData, 15, 15)) {

            /**
             * PLAYER-ONLY FILTER
             * In DayZ, 'DayZPlayer' usually has a specific type ID or VTable signature.
             * Logic based on sub_545D0 loop structure.
             */
            bool isPlayer = (entData.type_id == 0x1); // 0x1 assumed for human players

            if (isPlayer) {
                // Update ESP rendering only for human players
                void (*renderESP)(uint64_t, uint64_t, uint64_t, void*) = (void*)0x7FFFB448AC10;
                renderESP(world_ptr, context_ptr, 0, &entData);
            }
        }
    }

    return true;
}
