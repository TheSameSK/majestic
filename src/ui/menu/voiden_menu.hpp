#pragma once

struct ID3D11Device;

// New VOIDEN-style test menu (phase 1: Aimbot / Visuals).
// The legacy menu (src/ui/menu/menu.cpp) stays fully intact; the call site
// in src/game.hpp decides which menu is rendered at runtime.
namespace voiden_menu {
    void initialize( ID3D11Device* device );
    void shutdown( );
    void render( bool menu_open );
}
