#pragma once
#include <cstdint>

class Core {
public:
    Core();
    ~Core();
    ///--- 初期化 ---
    void Initialize();
    ///--- メインループ ---
    void Run();
    ///--- 終了処理 ---
    void Finalize();

private:

};