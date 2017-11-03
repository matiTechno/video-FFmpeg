#include <future>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>

#include "Video.hpp"

static const char* flipped = R"(

#vertex

#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 texCoords;
layout(location = 3) in mat4 matrix;

uniform mat4 projection;

out vec4 vColor;
out vec2 vTexCoords;
out vec2 vPosition;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;
    vec2 nVertex = vertex.zw;
    nVertex.y = (nVertex.y - 1) * -1.0;
    vTexCoords = nVertex.xy * texCoords.zw + texCoords.xy;
    vPosition = vertex.xy;
}

#fragment

#version 330

out vec4 color;

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D sampler;
uniform int type;

void main()
{
    color = texture(sampler, vTexCoords) * vColor;
}
)";

class Scene: public hppv::PrototypeScene
{
public:
    Scene():
        hppv::PrototypeScene(hppv::Space(0.f, 0.f, 0.f, 0.f), 1.f, false),
        shader_({flipped}, "")
    {
        Video::init();

        videoBackground_.open("../hf_jackpot.mp4");
        videoDragon_.open("../dragon.mov");
    }

private:
    Video videoBackground_;
    Video videoDragon_;
    hppv::Shader shader_;
    bool decode_ = true;
    bool update_ = true;
    bool future_ = true;
    glm::vec4 color_ = {0.5f, 0.5f, 0.5f, 1.f};

    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.setShader(shader_);
        renderer.setProjection(hppv::Space(properties_.pos, properties_.size));

        if(decode_)
        {
            if(future_)
            {
                auto future = std::async(std::launch::async, [this]{videoBackground_.decodeNextFrame();});
                videoDragon_.decodeNextFrame();
                future.get();
            }
            else
            {
                videoBackground_.decodeNextFrame();
                videoDragon_.decodeNextFrame();
            }
        }

        if(update_)
        {
            videoBackground_.uploadTexture();
            videoDragon_.uploadTexture();
        }

        renderer.setTexture(videoBackground_.texture_);

        hppv::Sprite sprite;
        sprite.color = color_;
        sprite.pos = properties_.pos;
        sprite.size = properties_.size;
        sprite.texRect = {0.f, 0.f, videoBackground_.texture_.getSize()};

        renderer.cache(sprite);

        renderer.setTexture(videoDragon_.texture_);
        sprite.texRect = {0.f, 0.f, videoDragon_.texture_.getSize()};

        renderer.cache(sprite);

        ImGui::Begin("options");
        {
            ImGui::Checkbox("decode", &decode_);
            ImGui::Checkbox("upload", &update_);
            ImGui::Checkbox("future", &future_);
            ImGui::Spacing();
            ImGui::ColorPicker4("sprite color", &color_.x);
        }
        ImGui::End();
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<Scene>();
    app.run();
    return 0;
}
