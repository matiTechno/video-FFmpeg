#include <future>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Texture.hpp>

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

        video1.open("../hf_jackpot.mp4");
        video2.open("../dragon.mov");
    }

private:
    Video video1, video2;
    hppv::Shader shader_;
    bool decode_ = true;
    bool update_ = true;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.setShader(shader_);
        renderer.setProjection(hppv::Space(properties_.pos, properties_.size));

        if(decode_)
        {
            auto future = std::async(std::launch::async, [this]{video1.decodeNextFrame();});
            video2.decodeNextFrame();
            future.get();
        }

        if(update_)
        {
            video1.uploadTexture();
            video2.uploadTexture();
        }

        renderer.setTexture(video1.texture_);

        hppv::Sprite sprite;
        sprite.pos = properties_.pos;
        sprite.size = properties_.size;
        sprite.texRect = {0.f, 0.f, video1.texture_.getSize()};

        renderer.cache(sprite);

        renderer.setTexture(video2.texture_);
        sprite.texRect = {0.f, 0.f, video2.texture_.getSize()};

        renderer.cache(sprite);


        ImGui::Begin("options");
        {
            ImGui::Checkbox("decode", &decode_);
            ImGui::Checkbox("update", &update_);
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
