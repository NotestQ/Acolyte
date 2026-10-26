#pragma once
#include <Acolyte/exports.h>

namespace sdk::Lobby {
    // Set on the game thread, changes apply the next time we host a lobby
    ACOLYTE_API int GetMaxPlayers();
    ACOLYTE_API void SetMaxPlayers(int players);
    ACOLYTE_API int GetSupportedPlayers();
    ACOLYTE_API int GetPlayerLimit();
    ACOLYTE_API bool GetDuplicatePalettes();
    ACOLYTE_API void SetDuplicatePalettes(bool enabled);
}
