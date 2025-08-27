/*
    HUD overlay: tiny instanced-quad text/box renderer for on-screen stats.
*/

#pragma once

#include "../utils/ShaderLoader.h"

#include <glad/glad.h>
#include <glm.hpp>
#include <vector>
#include <cstdint>
#include <initializer_list>
#include <cstdio>

//--HUD-OVERLAY--
class HUD
{
public:
    HUD() : shader(ShaderLoader::fromFiles("shaders/hud.vert", "shaders/hud.frag"))
    {
        const float quad[12] = { 0,0, 1,1, 1,0,   0,0, 0,1, 1,1 };

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &instVBO);
        glBindBuffer(GL_ARRAY_BUFFER, instVBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); //Instance data comes per frame.

        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(HUDRect), (void*)offsetof(HUDRect, x));
        glEnableVertexAttribArray(1);
        glVertexAttribDivisor(1, 1); //Per-instance.

        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(HUDRect), (void*)offsetof(HUDRect, r));
        glEnableVertexAttribArray(2);
        glVertexAttribDivisor(2, 1);

        glBindVertexArray(0);
    }

    ~HUD()
    {
        if (instVBO) glDeleteBuffers(1, &instVBO);
        if (quadVBO) glDeleteBuffers(1, &quadVBO);
        if (vao) glDeleteVertexArrays(1, &vao);
    }

    HUD(const HUD&) = delete;
    HUD& operator=(const HUD&) = delete;

    //Draw overlay with two lines.
    void draw(int screenW, int screenH, const char* line1, const char* line2)
    {
        draw(screenW, screenH, line1, line2, nullptr);
    }

    void draw(int screenW, int screenH, const char* line1, const char* line2, const char* line3)
    {
        std::vector<HUDRect> rects;
        rects.reserve(2048); //Enough for a few dozen characters.

        //--HUD-MEASURE--
        const int scale = 2;            //Text size.
        const int charSpacing = 2;      //Extra pixels between letters.
        const int pad = 6;              //Box padding.
        const int lineGap = 10;         //Gap between lines.
		const int x0 = 8;               //X position.
		const int y0 = 8;               //Y position.    

        auto measure = [&](const char* s)->int
        {
            if (!s) return 0;

            int total = 0, count = 0;

            for (const char* p = s; *p; ++p)
            {
                total += glyph(*p).advance * scale; //Advance is in 5x7 cells.
                ++count;
            }

            if (count > 1) total += (count - 1) * charSpacing;

            return total;
        };

        const int w1 = measure(line1);
        const int w2 = measure(line2);
        const int w3 = measure(line3);
        const int bw = (std::max)((std::max)(w1, w2), w3) + pad * 2;

        int lines = 0; 
        if (line1 && *line1) ++lines; 
        if (line2 && *line2) ++lines; 
        if (line3 && *line3) ++lines;

        const int bh = (lines ? (7 * scale * lines + lineGap * (lines - 1)) : 0) + pad * 2;
        //--HUD-MEASURE-END--

        //--HUD-BUILD-RECTS--
        //Background.
        rects.push_back(HUDRect{ float(x0 - 2), float(y0 - 2), float(bw + 4), float(bh + 4), 0,0,0, 0.35f });
        rects.push_back(HUDRect{ float(x0), float(y0), float(bw), float(bh), 0,0,0, 0.35f });

        //Text lines.
        int ty = y0 + pad;
        if (line1 && *line1) { appendText(rects, x0 + pad, ty, scale, charSpacing, line1, 1, 1, 1, 1); ty += 7 * scale + lineGap; }
        if (line2 && *line2) { appendText(rects, x0 + pad, ty, scale, charSpacing, line2, 0.85f, 0.92f, 1.0f, 1); ty += 7 * scale + lineGap; }
        if (line3 && *line3) { appendText(rects, x0 + pad, ty, scale, charSpacing, line3, 0.85f, 1.0f, 0.85f, 1); }
        //--HUD-BUILD-RECTS-END--

        //--HUD-UPLOAD+DRAW--
        shader.use();
        shader.setVec2("uScreen", glm::vec2((float)screenW, (float)screenH));

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, instVBO);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(rects.size() * sizeof(HUDRect)), rects.data(), GL_STREAM_DRAW);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLboolean wasDepth = glIsEnabled(GL_DEPTH_TEST);
        GLboolean wasCull = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)rects.size()); //6 verts, one quad, instanced per rect.

        if (wasDepth) glEnable(GL_DEPTH_TEST);
        if (wasCull)  glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
        //--HUD-UPLOAD+DRAW-END--
    }

private:
    struct HUDRect { float x, y, w, h; float r, g, b, a; };

    struct Glyph { uint8_t row[7]; uint8_t advance; }; //5x7 bitmap + advance.

    static inline const Glyph& glyph(char c)
    {
        static Glyph G[128]{};
        static bool init = false;
        if (!init)
        {
            auto set = [&](char ch, std::initializer_list<uint8_t> rows, uint8_t adv = 6)
            { 
                Glyph g{}; int k = 0;

                for (uint8_t r : rows) 
                {
                    g.row[k++] = r;
                }

                g.advance = adv; 
                G[(int)ch] = g;
            };

            //Space and punctuation.
            set(' ', { 0,0,0,0,0,0,0 }, 4);
            set('.', { 0,0,0,0,0,0b00110,0b00110 }, 3);
            set(':', { 0,0b00100,0b00100,0,0b00100,0b00100,0 }, 3);

            //Digits.
            set('0', { 0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110 });
            set('1', { 0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110 });
            set('2', { 0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111 });
            set('3', { 0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110 });
            set('4', { 0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010 });
            set('5', { 0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110 });
            set('6', { 0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110 });
            set('7', { 0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000 });
            set('8', { 0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110 });
            set('9', { 0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100 });

            //Letters used.
            set('F', { 0b11111,0b10000,0b11100,0b10000,0b10000,0b10000,0b10000 });
            set('P', { 0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000 });
            set('S', { 0b01111,0b10000,0b10000,0b01110,0b00001,0b00001,0b11110 });
            set('V', { 0b10001,0b10001,0b10001,0b10001,0b01010,0b01010,0b00100 });
            set('I', { 0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b11111 });

            init = true;
        }

        return G[(int)c];
    }

    static inline void appendText(std::vector<HUDRect>& out, int x, int y, int scale, int charSpacing,
        const char* text, float r, float g, float b, float a)
    {
        const int cell = scale;
        const int gap = 1;
        int penX = x;

        while (*text)
        {
            const Glyph& gph = glyph(*text++);

            for (int row = 0; row < 7; ++row)
            {
                uint8_t bits = gph.row[row];

                for (int col = 0; col < 5; ++col)
                {
                    if (bits & (1u << (4 - col)))
                    {
                        HUDRect q{};
                        q.x = float(penX + col * (cell + gap));
                        q.y = float(y + row * (cell + gap));
                        q.w = float(cell);
                        q.h = float(cell);
                        q.r = r; q.g = g; q.b = b; q.a = a;
                        out.push_back(q);
                    }
                }
            }

            penX += gph.advance * cell + charSpacing;
        }
    }

private:
    ShaderLoader shader;
    GLuint vao = 0;
    GLuint quadVBO = 0;
    GLuint instVBO = 0;
};
//--HUD-OVERLAY-END--