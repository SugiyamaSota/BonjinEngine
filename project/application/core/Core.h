#pragma once
#include <cstdint>
#include<memory>

#include"skydome/Skydome.h"

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

    std::unique_ptr<Skydome> skydome_ = nullptr;
    std::unique_ptr<Model> skydomeModel_ = nullptr;

    std::unique_ptr <Bonjin::Sprite> fadeIOSprite_;

};