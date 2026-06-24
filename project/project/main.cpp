#include <memory>
#include "MyGame.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::unique_ptr<MyGame> game = std::make_unique<MyGame>();

    game->Initialize();

    while (game->IsRunning()) {
        game->Update();
        game->Draw();
    }

    game->Finalize();

    return 0;
}