#pragma once
#include <cstdint>

class Core {
public:
    Core();
    ~Core();

    void Initialize();
    void Run();
    void Finalize();

private:
    const int32_t kClientWidth = 1280;
    const int32_t kClientHeight = 720;
};