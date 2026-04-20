#include "Core.h"
#include "BonjinEngine.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    D3DResourceLeakChecker leakChecker;

    

    Core core;
    core.Initialize();
    core.Run();
    core.Finalize();

    return 0;
}