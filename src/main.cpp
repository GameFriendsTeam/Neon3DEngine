#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <chrono>

#include <vector>

#ifdef _WIN32
#include <windows.h>

float getCpuLoad() {
    static ULONGLONG lastIdle = 0, lastKernel = 0, lastUser = 0;
    static bool first = true;
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return 0.0f;

    ULONGLONG idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    ULONGLONG kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    ULONGLONG user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    if (first) {
        lastIdle = idle;
        lastKernel = kernel;
        lastUser = user;
        first = false;
        return 0.0f;
    }

    ULONGLONG idleDiff = idle - lastIdle;
    ULONGLONG kernelDiff = kernel - lastKernel;
    ULONGLONG userDiff = user - lastUser;
    ULONGLONG total = kernelDiff + userDiff;

    lastIdle = idle;
    lastKernel = kernel;
    lastUser = user;

    if (total == 0) return 0.0f;
    float cpuLoad = 100.0f * (1.0f - (float)idleDiff / (float)total);
    if (cpuLoad < 0.0f) cpuLoad = 0.0f;
    if (cpuLoad > 100.0f) cpuLoad = 100.0f;
    return cpuLoad;
}
#else
float getCpuLoad() { return 0.0f; }
#endif


// --- ASCII bitmap-шрифт 8x8, 128 символов (0..127) ---
static const unsigned char font8x8_basic[128][8] = {
 { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},   // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};

// Вершинный и фрагментный шейдеры для текста
const char* textVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
out vec2 uv;
uniform vec2 screen;
void main() {
    gl_Position = vec4((inPos / screen) * 2.0 - 1.0, 0, 1);
    gl_Position.y = -gl_Position.y;
    uv = inUV;
}
)";
const char* textFragmentShader = R"(
#version 330 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D fontTex;
uniform vec3 color;
void main() {
    float a = texture(fontTex, uv).r;
    fragColor = vec4(color, a);
}
)";

const char* graphVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 inPos;
uniform vec2 screen;
void main() {
    vec2 p = inPos;
    p.x = (p.x / screen.x) * 2.0 - 1.0;
    p.y = (p.y / screen.y) * 2.0 - 1.0;
    gl_Position = vec4(p, 0, 1);
}
)";
const char* graphFragmentShader = R"(
#version 330 core
out vec4 fragColor;
uniform vec3 color;
void main() {
    fragColor = vec4(color, 1.0);
}
)";


// Создание текстуры шрифта
GLuint createFontTexture() {
    unsigned char atlas[128 * 64] = { 0 };
    for (int ch = 0; ch < 128; ++ch) {
        int cx = ch % 16, cy = ch / 16;
        for (int y = 0; y < 8; ++y) {
            unsigned char row = font8x8_basic[ch][y];
            for (int x = 0; x < 8; ++x) {
                int px = cx * 8 + x;
                int py = cy * 8 + y;
                atlas[py * 128 + px] = (row & (1 << x)) ? 255 : 0;
            }
        }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 128, 64, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return tex;
}

// Рендер строки текста (ASCII 32..127)
void drawTextGL(GLuint prog, GLuint fontTex, int w, int h, float x, float y, const char* text, float scale, float r, float g, float b) {
    std::vector<float> vbo;
    float sx = 8.0f * scale, sy = 8.0f * scale;
    size_t len = strlen(text);
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = text[i];
        if (c < 32 || c > 127) continue;
        int ch = c;
        int cx = ch % 16, cy = ch / 16;
        float u0 = cx / 16.0f, v0 = cy / 8.0f;
        float u1 = (cx + 1) / 16.0f, v1 = (cy + 1) / 8.0f;
        float xpos = x + i * sx;
        float ypos = y;
        float quad[] = {
            xpos,     ypos,     u0, v0,
            xpos + sx,  ypos,     u1, v0,
            xpos + sx,  ypos + sy,  u1, v1,
            xpos,     ypos + sy,  u0, v1
        };
        vbo.insert(vbo.end(), quad, quad + 16);
    }
    std::vector<unsigned> ibo;
    for (size_t i = 0; i < len; ++i) {
        unsigned idx = i * 4;
        ibo.push_back(idx + 0); ibo.push_back(idx + 1); ibo.push_back(idx + 2);
        ibo.push_back(idx + 2); ibo.push_back(idx + 3); ibo.push_back(idx + 0);
    }
    if (vbo.empty() || ibo.empty()) return;
    GLuint vao, vboId, iboId;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboId);
    glGenBuffers(1, &iboId);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, vbo.size() * sizeof(float), vbo.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0); // pos
    glEnableVertexAttribArray(1); // uv
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo.size() * sizeof(unsigned), ibo.data(), GL_STREAM_DRAW);

    glUseProgram(prog);
    glUniform2f(glGetUniformLocation(prog, "screen"), (float)w, (float)h);
    glUniform3f(glGetUniformLocation(prog, "color"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glUniform1i(glGetUniformLocation(prog, "fontTex"), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLES, (GLsizei)ibo.size(), GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vboId);
    glDeleteBuffers(1, &iboId);
    glDeleteVertexArrays(1, &vao);
}

void drawGraphGL(GLuint prog, int w, int h, const std::vector<float>& values, float minVal, float maxVal, float x, float y, float width, float height, float r, float g, float b) {
    if (values.empty()) return;
    std::vector<float> vbo;
    size_t N = values.size();
    for (size_t i = 0; i < N; ++i) {
        float px = x + (float)i / (N - 1) * width;
        float norm = (values[i] - minVal) / (maxVal - minVal);
        float py = y + (1.0f - norm) * height;
        vbo.push_back(px);
        vbo.push_back(py);
    }
    GLuint vao, vboId;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboId);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, vbo.size() * sizeof(float), vbo.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glUseProgram(prog);
    glUniform2f(glGetUniformLocation(prog, "screen"), (float)w, (float)h);
    glUniform3f(glGetUniformLocation(prog, "color"), r, g, b);

    glLineWidth(3.0f); // толщина линии графика
    glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)N);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vboId);
    glDeleteVertexArrays(1, &vao);
}



// Конфиг-файл
const char* g_ConfigPath = "config.cfg";

// Вершинный шейдер для полноэкранного треугольника
const char* fullscreenVertexShader = R"(
#version 330 core
out vec2 vUV;
void main() {
    const vec2 pos[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1,3));
    vUV = (pos[gl_VertexID].xy + 1.0) * 0.5;
    gl_Position = vec4(pos[gl_VertexID], 0, 1);
}
)";

// Загрузка текста из файла
std::string loadTextFile(const char* path) {
    std::ifstream file(path);
    if (!file) {
        printf("Не удалось открыть файл: %s\n", path);
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Сохранение текста в файл
void saveTextFile(const char* path, const std::string& text) {
    std::ofstream file(path);
    if (!file) {
        printf("Не удалось записать файл: %s\n", path);
        return;
    }
    file << text;
}

// Загрузка пути к шейдеру из config.cfg
std::string loadShaderPathFromConfig() {
    std::ifstream file(g_ConfigPath);
    if (!file) return "";
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("shader_path=") == 0) {
            return line.substr(12);
        }
    }
    return "";
}

// Сохранение пути к шейдеру в config.cfg
void saveShaderPathToConfig(const std::string& path) {
    std::ofstream file(g_ConfigPath);
    if (!file) return;
    file << "shader_path=" << path << std::endl;
}

// Автоматическое преобразование ShaderToy-шейдера в GLSL 330 core (только в памяти)
std::string fixShaderToyShader(const std::string& src, bool& wasFixed) {
    std::string out;
    wasFixed = false;
    std::regex mainImageRegex(R"(void\s+mainImage\s*\(\s*out\s+vec4\s+\w+\s*,\s*in\s*vec2\s+\w+\s*\))");
    if (std::regex_search(src, mainImageRegex)) {
        wasFixed = true;
        out += "#version 330 core\n";
        out += "out vec4 fragColor;\n";
        out += "in vec2 vUV;\n";
        out += "uniform vec2 iResolution;\n";
        out += "uniform float iTime;\n";
        out += "uniform vec4 iMouse;\n";
        out += "uniform int iFrame;\n\n";
        std::string body = std::regex_replace(src, std::regex(R"(#version[^\n]*\n)"), "");
        body = std::regex_replace(body, std::regex(R"(uniform\s+[^\n]*\n)"), "");
        body = std::regex_replace(body, mainImageRegex, "void main()");
        body = std::regex_replace(body, std::regex(R"(\bfragCoord\b)"), "vUV * iResolution.xy");
        body = std::regex_replace(body, std::regex(R"(void\s+main\s*\(\s*out\s+vec4\s+\w+\s*,\s*in\s*vec2\s+\w+\s*\))"), "void main()");
        body = std::regex_replace(body, std::regex(R"(\n{3,})"), "\n\n");
        out += body;
    }
    else {
        out = src;
    }
    return out;
}

// Компиляция и линковка шейдера
GLuint createShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[2048];
        glGetShaderInfoLog(shader, 2048, nullptr, log);
        printf("Shader error: %s\n", log);
    }
    return shader;
}
GLuint createProgram(const char* vs, const char* fs) {
    GLuint v = createShader(GL_VERTEX_SHADER, vs);
    GLuint f = createShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// Меню выбора и загрузки шейдера
std::string selectShaderPath() {
    std::string lastPath = loadShaderPathFromConfig();
    std::string path;
    printf("=== GLSL Shader Loader ===\n");
    if (!lastPath.empty()) {
        printf("Последний путь к шейдеру: %s\n", lastPath.c_str());
        printf("Использовать его? (y/n): ");
        std::string ans;
        std::getline(std::cin, ans);
        if (ans == "y" || ans == "Y" || ans.empty()) {
            path = lastPath;
        }
    }
    if (path.empty()) {
        printf("Введите путь к файлу с фрагментным шейдером (.txt): ");
        std::getline(std::cin, path);
    }
    // Проверяем файл
    std::ifstream test(path);
    while (!test) {
        printf("Файл не найден. Введите путь ещё раз: ");
        std::getline(std::cin, path);
        test.open(path);
    }
    saveShaderPathToConfig(path);
    return path;
}

int main() {
	setlocale(LC_ALL, "Russian");
    // Меню выбора шейдера
    std::string shaderPath = selectShaderPath();
    std::string origShaderText = loadTextFile(shaderPath.c_str());
    if (origShaderText.empty()) return -1;
    bool wasFixed = false;
    std::string fixedShaderText = fixShaderToyShader(origShaderText, wasFixed);
    if (wasFixed) {
        printf("ShaderToy-стиль обнаружен: шейдер автоматически преобразован для OpenGL.\n");
    }

    printf("Шейдер успешно загружен.\n");
    printf("Запустить рендер? (y/n): ");
    std::string ans;
    std::getline(std::cin, ans);
    if (!(ans == "y" || ans == "Y" || ans.empty())) {
        printf("Выход.\n");
        return 0;
    }

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "GLSL Shader Loader", monitor, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("GLAD init failed\n");
        return -1;
    }

    GLuint prog = createProgram(fullscreenVertexShader, fixedShaderText.c_str());
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // --- Инициализация текстового рендера ---
    GLuint fontTex = createFontTexture();
    GLuint textProg = createProgram(textVertexShader, textFragmentShader);

    GLuint graphProg = createProgram(graphVertexShader, graphFragmentShader);

    const size_t graphSize = 256;


    double startTime = glfwGetTime();
    int frame = 0;
    double fps = 0.0;
    double frameTime = 0.0;
    double lastFpsTime = glfwGetTime();
    int frameCount = 0;

    // Имя файла для debug-инфо
    std::string shaderFileName = shaderPath;
    size_t pos = shaderFileName.find_last_of("/\\");
    if (pos != std::string::npos)
        shaderFileName = shaderFileName.substr(pos + 1);

    std::vector<float> frameTimes(graphSize, 0.0f);
    std::vector<float> fpsHistory(graphSize, 0.0f);
    std::vector<float> cpuLoadHistory(graphSize, 0.0f);


    while (!glfwWindowShouldClose(window)) {
        frameTimes.erase(frameTimes.begin());
        frameTimes.push_back((float)frameTime * 1000.0f); // ms

        fpsHistory.erase(fpsHistory.begin());
        fpsHistory.push_back((float)fps);

        cpuLoadHistory.erase(cpuLoadHistory.begin());
        cpuLoadHistory.push_back(getCpuLoad());


        auto t0 = std::chrono::high_resolution_clock::now();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        GLint locRes = glGetUniformLocation(prog, "iResolution");
        if (locRes >= 0) glUniform2f(locRes, (float)w, (float)h);
        GLint locTime = glGetUniformLocation(prog, "iTime");
        if (locTime >= 0) glUniform1f(locTime, (float)(glfwGetTime() - startTime));
        GLint locFrame = glGetUniformLocation(prog, "iFrame");
        if (locFrame >= 0) glUniform1i(locFrame, frame);

        double mx = 0, my = 0;
        int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        glfwGetCursorPos(window, &mx, &my);
        GLint locMouse = glGetUniformLocation(prog, "iMouse");
        if (locMouse >= 0)
            glUniform4f(locMouse, (float)mx, (float)(h - my), state == GLFW_PRESS ? 1.0f : 0.0f, 0.0f);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --- Debug info ---
        auto t1 = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<double>(t1 - t0).count();
        frameCount++;
        double now = glfwGetTime();
        if (now - lastFpsTime > 0.5) {
            fps = frameCount / (now - lastFpsTime);
            frameCount = 0;
            lastFpsTime = now;
        }

        // Обновляем историю
        frameTimes.erase(frameTimes.begin());
        frameTimes.push_back((float)frameTime * 1000.0f); // ms

        // Сохраняем состояние OpenGL
        GLint prevProgram = 0, prevVAO = 0, prevTex = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);

        // Параметры области графика
        float graphW = (float)w * 0.7f;
        float graphH = 140.0f;
        float graphX = (float)w * 0.15f;
        float graphY = 40.0f;

        // Время кадра (жёлтый)
        drawGraphGL(graphProg, w, h, frameTimes, 0.0f, 40.0f, graphX, graphY, graphW, graphH, 1.0f, 0.8f, 0.2f);
        // FPS (зелёный)
        drawGraphGL(graphProg, w, h, fpsHistory, 0.0f, 120.0f, graphX, graphY, graphW, graphH, 0.2f, 1.0f, 0.2f);
        // CPU Load (синий)
        drawGraphGL(graphProg, w, h, cpuLoadHistory, 0.0f, 100.0f, graphX, graphY, graphW, graphH, 0.2f, 0.5f, 1.0f);

        // Цветная легенда с динамическим смещением
        auto textWidth = [](const char* text, float scale) {
            return (int)(strlen(text) * 8 * scale);
            };
        float legendY = graphY + graphH + 650; // всегда чуть выше графика, вне зависимости от разрешения
        float legendX = graphX;
        float scale = 2.0f;

        // Формируем строки с текущими значениями
        char txt1[64], txt2[64], txt3[64];
        snprintf(txt1, sizeof(txt1), "FrameTime (ms): %.2f", frameTime * 1000.0);
        snprintf(txt2, sizeof(txt2), "FPS: %.1f", fps);
        snprintf(txt3, sizeof(txt3), "CPU load: %.1f%%", cpuLoadHistory.back());

        // Рисуем каждую метку с цветом и значением
        drawTextGL(textProg, fontTex, w, h, legendX, legendY, txt1, scale, 1.0f, 0.8f, 0.2f);
        legendX += textWidth(txt1, scale) + 32;
        drawTextGL(textProg, fontTex, w, h, legendX, legendY, txt2, scale, 0.2f, 1.0f, 0.2f);
        legendX += textWidth(txt2, scale) + 32;
        drawTextGL(textProg, fontTex, w, h, legendX, legendY, txt3, scale, 0.2f, 0.5f, 1.0f);



        // Восстанавливаем состояние
        glUseProgram(prevProgram);
        glBindVertexArray(prevVAO);
        glBindTexture(GL_TEXTURE_2D, prevTex);



        char info[256];
        snprintf(info, sizeof(info),
            "FPS: %.1f | Frame: %d | Time: %.2f ms | %dx%d | %s",
            fps, frame, frameTime * 1000.0, w, h, shaderFileName.c_str());

        prevProgram = 0, prevVAO = 0, prevTex = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
        drawTextGL(textProg, fontTex, w, h, 10, 10, info, 1.5f, 1.0f, 1.0f, 0.0f);
        glUseProgram(prevProgram);
        glBindVertexArray(prevVAO);
        glBindTexture(GL_TEXTURE_2D, prevTex);


        glfwSwapBuffers(window);
        glfwPollEvents();
        frame++;
    }
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(textProg);
    glDeleteTextures(1, &fontTex);
    glDeleteProgram(prog);
    glDeleteProgram(graphProg);
    glfwTerminate();
    return 0;
}
