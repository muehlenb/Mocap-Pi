#pragma once

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

#include "src/util/gl/TextRenderer.h"

std::shared_ptr<Shader> TextRenderer::shader = nullptr;
std::shared_ptr<Texture2D> TextRenderer::fontTexture = nullptr;


void TextRenderer::render(std::string text, int screenWidth, int screenHeight, int positionX = 0, int positionY = 0, float fontSize = 20){
    if(shader == nullptr)
        shader = std::make_shared<Shader>("shader/textrenderer.vert", "shader/textrenderer.frag");

    if(fontTexture == nullptr)
        fontTexture = std::make_shared<Texture2D>("data/font/texture.png");

    std::vector<float> vertices;
    std::vector<float> uv;

    int xSize = 32;
    int ySize = 51;

    int x = 0;
    int y = 0;

    for(char& c : text) {
        if(c == '\n'){
            x = 0;
            y -= ySize;
            continue;
        }

        int charOfTexture = int(c) - 32;

        float charXSize = 32/512.f;
        float charYSize = 51/512.f;

        float charX = (charOfTexture % 16) * charXSize;
        float charY = (charOfTexture / 16) * charYSize;

        vertices.push_back(x); uv.push_back(charX);
        vertices.push_back(y); uv.push_back(charY + charYSize);

        vertices.push_back(x+xSize); uv.push_back(charX+charXSize);
        vertices.push_back(y+ySize); uv.push_back(charY);

        vertices.push_back(x); uv.push_back(charX);
        vertices.push_back(y+ySize); uv.push_back(charY);

        vertices.push_back(x+xSize); uv.push_back(charX+charXSize);
        vertices.push_back(y+ySize); uv.push_back(charY);

        vertices.push_back(x); uv.push_back(charX);
        vertices.push_back(y); uv.push_back(charY + charYSize);

        vertices.push_back(x+xSize); uv.push_back(charX + charXSize);
        vertices.push_back(y); uv.push_back(charY + charYSize);

        x += xSize;
    }

    if(vertices.size() == 0)
        return;

    // OpenGL Drawing:
    {
        unsigned int vao;
        unsigned int vbo[2];

        // Generate vertex array object:
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(2, vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() *  sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(GLfloat), &uv[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glActiveTexture(GL_TEXTURE0);
        fontTexture->bind();

        // Draw text:
        shader->bind();

        shader->setUniform("screenWidth", screenWidth);
        shader->setUniform("screenHeight", screenHeight);
        shader->setUniform("fontSize", (fontSize / 51.f));
        shader->setUniform("x", positionX - (x/9));
        shader->setUniform("y", positionY);


        shader->setUniform("fontTexture", 0);

        glDrawArrays(GL_TRIANGLES, 0, int(vertices.size() / 2));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Delete vertex array object:
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(2, vbo);
    }
}
