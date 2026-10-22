#pragma once

#include "src/util/gl/Shader.h"
#include "src/util/gl/Texture2D.h"


class TextRenderer
{
public:
    static void render(std::string text, int screenWidth, int screenHeight, int x, int y, float size);

private:
    static std::shared_ptr<Shader> shader;
    static std::shared_ptr<Texture2D> fontTexture;

};
