#include <iostream>

#include <hppv/App.hpp>
#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

#include "Video.hpp"

class Scene: public hppv::Prototype
{
public:
    Scene(const char* const filename):
        hppv::Prototype({}),
        video_(filename)
    {}

private:
    Video video_;

    bool decode_ = true;
    bool updateTexture_ = true;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        if(decode_)
        {
            video_.decodeFrame();
        }

        if(updateTexture_)
        {
            video_.updateTexture();
        }

        renderer.shader(hppv::Render::Tex);
        renderer.flipTextureY(true);

        hppv::Sprite sprite({0, 0, video_.texture.getSize()});
        sprite.texRect = {0.f, 0.f, video_.texture.getSize()};

        renderer.projection(hppv::expandToMatchAspectRatio(sprite.toSpace(), properties_.size));
        renderer.texture(video_.texture);
        renderer.cache(sprite);

        ImGui::Begin(prototype_.imguiWindowName);
        {
            ImGui::Checkbox("decode", &decode_);
            ImGui::Checkbox("upload", &updateTexture_);
        }
        ImGui::End();
    }
};

int main(const int argc, const char* const * const argv)
{
    if(argc != 2)
    {
        std::cout << "usage: ./video filename" << std::endl;
        return 1;
    }

    hppv::App app;
    hppv::App::InitParams p;
    p.window.title = argv[1];
    if(!app.initialize(p)) return 1;

    Video::init();

    app.pushScene<Scene>(argv[1]);
    app.run();
    return 0;
}
