#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define closesocket close
#endif
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <regex>
#include <fstream>

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <mmsystem.h>
#include <stdlib.h>

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

bool isBlockInFrustum(const glm::mat4& viewProjMatrix, const glm::vec3& blockPos, float blockSize) {
    // Центр блока
    glm::vec4 center(blockPos.x + blockSize / 2.0f, blockPos.y + blockSize / 2.0f, blockPos.z + blockSize / 2.0f, 1.0f);

    // Добавляем небольшой запас (margin) к границам фруструма
    const float margin = 0.45f; // Запас в clip space

    glm::vec4 clipSpacePos = viewProjMatrix * center;

    // Проверяем, находится ли центр блока в пределах видимости с учётом запаса
    return (clipSpacePos.x >= -clipSpacePos.w - margin && clipSpacePos.x <= clipSpacePos.w + margin &&
        clipSpacePos.y >= -clipSpacePos.w - margin && clipSpacePos.y <= clipSpacePos.w + margin &&
        clipSpacePos.z >= -clipSpacePos.w - margin && clipSpacePos.z <= clipSpacePos.w + margin);
}
struct FrustumPlane {
    glm::vec4 plane; // (a, b, c, d): ax + by + cz + d = 0
};

void extractFrustumPlanes(const glm::mat4& m, FrustumPlane planes[6]) {
    // Левая
    planes[0].plane = glm::vec4(
        m[0][3] + m[0][0],
        m[1][3] + m[1][0],
        m[2][3] + m[2][0],
        m[3][3] + m[3][0]);
    // Правая
    planes[1].plane = glm::vec4(
        m[0][3] - m[0][0],
        m[1][3] - m[1][0],
        m[2][3] - m[2][0],
        m[3][3] - m[3][0]);
    // Нижняя
    planes[2].plane = glm::vec4(
        m[0][3] + m[0][1],
        m[1][3] + m[1][1],
        m[2][3] + m[2][1],
        m[3][3] + m[3][1]);
    // Верхняя
    planes[3].plane = glm::vec4(
        m[0][3] - m[0][1],
        m[1][3] - m[1][1],
        m[2][3] - m[2][1],
        m[3][3] - m[3][1]);
    // Ближняя
    planes[4].plane = glm::vec4(
        m[0][3] + m[0][2],
        m[1][3] + m[1][2],
        m[2][3] + m[2][2],
        m[3][3] + m[3][2]);
    // Дальняя
    planes[5].plane = glm::vec4(
        m[0][3] - m[0][2],
        m[1][3] - m[1][2],
        m[2][3] - m[2][2],
        m[3][3] - m[3][2]);
    // Нормализация
    for (int i = 0; i < 6; ++i) {
        float len = glm::length(glm::vec3(planes[i].plane));
        planes[i].plane /= len;
    }
}

bool isAABBInFrustum(const glm::mat4& viewProjMatrix, const glm::vec3& min, const glm::vec3& max) {
    FrustumPlane planes[6];
    extractFrustumPlanes(viewProjMatrix, planes);
    for (int i = 0; i < 6; ++i) {
        // Для каждой плоскости ищем "самую дальнюю" точку AABB по направлению нормали
        glm::vec3 p = min;
        if (planes[i].plane.x >= 0) p.x = max.x;
        if (planes[i].plane.y >= 0) p.y = max.y;
        if (planes[i].plane.z >= 0) p.z = max.z;
        // Если вся AABB по одну сторону от плоскости — вне фруструма
        if (planes[i].plane.x * p.x + planes[i].plane.y * p.y + planes[i].plane.z * p.z + planes[i].plane.w < 0)
            return false;
    }
    return true;
}


bool isFaceInFrustum(const glm::mat4& viewProjMatrix, const glm::vec3& blockPos, const glm::vec3& faceOffset, float blockSize) {
    glm::vec3 min = blockPos;
    glm::vec3 max = blockPos + glm::vec3(blockSize);

    // Для каждой грани определяем её AABB (это плоскость толщиной 0, но для надёжности можно добавить небольшой offset)
    glm::vec3 fmin = min, fmax = max;
    const float eps = 0.001f;
    if (faceOffset.x != 0) {
        fmin.x = fmax.x = (faceOffset.x > 0) ? max.x : min.x;
        fmin.x -= eps; fmax.x += eps;
    }
    else if (faceOffset.y != 0) {
        fmin.y = fmax.y = (faceOffset.y > 0) ? max.y : min.y;
        fmin.y -= eps; fmax.y += eps;
    }
    else if (faceOffset.z != 0) {
        fmin.z = fmax.z = (faceOffset.z > 0) ? max.z : min.z;
        fmin.z -= eps; fmax.z += eps;
    }
    return isAABBInFrustum(viewProjMatrix, fmin, fmax);
}


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

// --- ���� � ��������� ---
#define TEXTURE_GRASS "textures/grass.bmp"
#define TEXTURE_DIRT  "textures/dirt.bmp"
#define TEXTURE_STONE "textures/stone.bmp"

#pragma pack(push, 1)
struct BlockChange {
    int x, y, z;
    int type; // BlockType
};
#pragma pack(pop)


// --- ������ ����� ---
const float BLOCK_SIZE = 0.5f;

// --- ���������� ���������� ---
int g_winWidth = 1280, g_winHeight = 720;
glm::mat4 g_projection;
float lastX = g_winWidth / 2.0f, lastY = g_winHeight / 2.0f;
bool firstMouse = true;
float yaw = -90.0f, pitch = 0.0f;
glm::vec3 cameraPos(16.0f * BLOCK_SIZE, 10.0f * BLOCK_SIZE, 16.0f * BLOCK_SIZE);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
float velocityY = 0.0f;
bool onGround = false;
bool showDebug = false;
const float playerHeight = BLOCK_SIZE * 1.8f;
const float playerRadius = BLOCK_SIZE * 0.3f;

GLFWmonitor* g_monitor = nullptr;
const GLFWvidmode* g_mode = nullptr;
bool g_fullscreen = true;

// --- ��������� ---
enum BlockType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3 };
BlockType inventory[] = { GRASS, DIRT};
int inventorySize = sizeof(inventory) / sizeof(BlockType);
int selectedBlock = 3;

// --- ��������� ���� ---
const int WORLD_X = 32, WORLD_Y = 16, WORLD_Z = 32;
struct Block {
    BlockType type;
};
Block world[WORLD_X][WORLD_Y][WORLD_Z];

// --- ����������� ---
constexpr int MAX_PLAYERS = 4;
glm::vec3 playerColors[MAX_PLAYERS] = {
    glm::vec3(1.0f, 0.2f, 0.2f), // ������� (����� 0)
    glm::vec3(0.2f, 0.6f, 1.0f), // ����� (����� 1)
    glm::vec3(0.2f, 1.0f, 0.3f), // ������ (����� 2)
    glm::vec3(1.0f, 1.0f, 0.2f)  // Ƹ���� (����� 3)
};
struct PlayerNetState {
    float x, y, z;
    bool active;
};
std::mutex netMutex;
PlayerNetState players[MAX_PLAYERS]; // [0] - ������, [1..] - �������
int myPlayerId = 0; // id �������� ������
std::atomic<bool> networkRunning{ false };
SOCKET sock = 0;
std::thread networkThread;
bool isServer = false;

// --- ������� ������� ---
void initSockets() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}
void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}
void serverThreadFunc(unsigned short port) {
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    bind(listenSock, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock, MAX_PLAYERS - 1);

    // ������� listenSock �������������
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(listenSock, FIONBIO, &mode);
#else
    int flags = fcntl(listenSock, F_GETFL, 0);
    fcntl(listenSock, F_SETFL, flags | O_NONBLOCK);
#endif

    std::vector<SOCKET> clients;
    printf("�������� �������� �� ����� %d...\n", port);
    networkRunning = true;
    while (networkRunning) {
        // ��������� ����� ��������
        SOCKET cs = accept(listenSock, nullptr, nullptr);
        if (cs != -1 && clients.size() < MAX_PLAYERS - 1) {
            // ������� cs �������������
#ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(cs, FIONBIO, &mode);
#else
            int flags = fcntl(cs, F_GETFL, 0);
            fcntl(cs, F_SETFL, flags | O_NONBLOCK);
#endif
            clients.push_back(cs);
            printf("������ �����������! id=%d\n", (int)clients.size());
        }
        // �������� ������� �� ��������
        for (size_t i = 0; i < clients.size(); ++i) {
            float pos[3];
            int recvd = recv(clients[i], (char*)pos, sizeof(pos), 0);
            if (recvd == sizeof(pos)) {
                std::lock_guard<std::mutex> lock(netMutex);
                players[i + 1].x = pos[0];
                players[i + 1].y = pos[1];
                players[i + 1].z = pos[2];
                players[i + 1].active = true;
            }
        }

        // �������� ��������� ������ �� ��������
        for (size_t i = 0; i < clients.size(); ++i) {
            BlockChange change;
            while (true) {
                int recvd = recv(clients[i], (char*)&change, sizeof(change), 0);
                if (recvd != sizeof(change)) {
#ifdef _WIN32
                    if (recvd == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
                        break;
#endif
                    break;
                }
                if (change.x >= 0 && change.x < WORLD_X &&
                    change.y >= 0 && change.y < WORLD_Y &&
                    change.z >= 0 && change.z < WORLD_Z) {
                    world[change.x][change.y][change.z].type = (BlockType)change.type;
                    for (size_t j = 0; j < clients.size(); ++j) {
                        send(clients[j], (char*)&change, sizeof(change), 0);
                    }
                }
            }
        }


        // ���������� ���� �������� ��������� ���� �������
        for (size_t i = 0; i < clients.size(); ++i) {
            std::lock_guard<std::mutex> lock(netMutex);
            send(clients[i], (char*)players, sizeof(players), 0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    for (auto cs : clients) closesocket(cs);
    closesocket(listenSock);
}

void clientThreadFunc(const char* ip, unsigned short port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    printf("����������� � %s:%d...\n", ip, port);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("������ �����������!\n");
        return;
    }
    // ������� sock �������������
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
    printf("���������� � �������!\n");
    networkRunning = true;
    while (networkRunning) {
        // ���������� ���� �������
        float pos[3];
        {
            std::lock_guard<std::mutex> lock(netMutex);
            pos[0] = players[0].x;
            pos[1] = players[0].y;
            pos[2] = players[0].z;
        }
        send(sock, (char*)pos, sizeof(pos), 0);
        // �������� ��������� ���� �������
        int recvd = recv(sock, (char*)players, sizeof(players), 0);
        if (recvd == sizeof(players)) {
            // ok
        }
        // �������� ��������� ������
        BlockChange change;
        while (true) {
            int recvd = recv(sock, (char*)&change, sizeof(change), 0);
            if (recvd != sizeof(change)) {
#ifdef _WIN32
                if (recvd == -1 && WSAGetLastError() == WSAEWOULDBLOCK)
                    break;
#endif
                break;
            }
            if (change.x >= 0 && change.x < WORLD_X &&
                change.y >= 0 && change.y < WORLD_Y &&
                change.z >= 0 && change.z < WORLD_Z) {
                world[change.x][change.y][change.z].type = (BlockType)change.type;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    closesocket(sock);
}


// --- Callback ��� ��������� ������� ���� ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    g_winWidth = width;
    g_winHeight = height;
    glViewport(0, 0, width, height);
    g_projection = glm::perspective(glm::radians(70.0f), (float)width / (float)height, 0.1f, 200.0f);
}

// --- ��������� ���� ---
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.12f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// --- ��������� �������� ���� ��� ��������� ---
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset > 0) selectedBlock = (selectedBlock + 1) % inventorySize;
    if (yoffset < 0) selectedBlock = (selectedBlock - 1 + inventorySize) % inventorySize;
}

// --- ��������� ������ 1/2/3 ��� ������ ����� ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) selectedBlock = 0;
        if (key == GLFW_KEY_2 && inventorySize > 1) selectedBlock = 1;
        if (key == GLFW_KEY_3 && inventorySize > 2) selectedBlock = 2;
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }
        if (key == GLFW_KEY_F11) {
            g_fullscreen = !g_fullscreen;
            if (g_fullscreen) {
                glfwSetWindowMonitor(window, g_monitor, 0, 0, g_mode->width, g_mode->height, g_mode->refreshRate);
            }
            else {
                int xpos = (g_mode->width - 1280) / 2;
                int ypos = (g_mode->height - 720) / 2;
                glfwSetWindowMonitor(window, nullptr, xpos, ypos, 1280, 720, 0);
            }
        }
        if (key == GLFW_KEY_F3) {
            showDebug = !showDebug;
        }
    }
}

// --- �������� BMP-�������� ---
GLuint loadBMP(const char* imagepath) {
    unsigned char header[54];
    unsigned int dataPos, width, height, imageSize;
    unsigned char* data;

    FILE* file = fopen(imagepath, "rb");
    if (!file) {
        std::cerr << "�� ������� ������� BMP: " << imagepath << std::endl;
        return 0;
    }
    if (fread(header, 1, 54, file) != 54) {
        std::cerr << "�������� BMP header: " << imagepath << std::endl;
        fclose(file);
        return 0;
    }
    if (header[0] != 'B' || header[1] != 'M') {
        std::cerr << "���� �� BMP: " << imagepath << std::endl;
        fclose(file);
        return 0;
    }
    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    width = *(int*)&(header[0x12]);
    height = *(int*)&(header[0x16]);
    if (imageSize == 0) imageSize = width * height * 3;
    if (dataPos == 0) dataPos = 54;
    data = new unsigned char[imageSize];
    fseek(file, dataPos, SEEK_SET);
    fread(data, 1, imageSize, file);
    fclose(file);

    // BMP ������ ����� � BGR, OpenGL ������� RGB
    for (unsigned int i = 0; i < imageSize; i += 3) {
        std::swap(data[i], data[i + 2]);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    delete[] data;
    return textureID;
}

// --- ������� ---
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in float aAO;
out vec2 vUV;
out vec3 vNormal;
out vec3 vWorldPos;
out float vAO;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    vUV = aUV;
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vWorldPos = vec3(model * vec4(aPos, 1.0));
    vAO = aAO;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";
const char* fragmentShaderSource = R"(
#version 330 core
in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;
in float vAO;
out vec4 FragColor;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform int blockType;
uniform sampler2D texGrass;
uniform sampler2D texDirt;
uniform sampler2D texStone;
uniform bool isPlayer;
uniform vec3 playerColor;
void main() {
    float ambient = 0.25 + 0.25 * vAO;
    vec3 norm = normalize(vNormal);
    float diff = max(dot(norm, normalize(-lightDir)), 0.0);
    vec3 viewDir = normalize(viewPos - vWorldPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * 0.0025;
    float lighting = ambient + diff * 0.2 + spec;
    vec4 texColor;
    if (isPlayer)
        texColor = vec4(playerColor, 1.0);
    else if (blockType == 1)
        texColor = texture(texGrass, vUV);
    else if (blockType == 2)
        texColor = texture(texDirt, vUV);
    else if (blockType == 3)
        texColor = texture(texStone, vUV);
    else
        texColor = vec4(1,0,1,1);
    FragColor = vec4(texColor.rgb * lighting * lightColor*0.75, texColor.a);
}
)";

// --- ������ ��� ������� � ������ (2D) ---
const char* quadVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
out vec2 vUV;
uniform mat4 proj;
void main() {
    vUV = aUV;
    gl_Position = proj * vec4(aPos, 0.0, 1.0);
}
)";
const char* quadFragmentShader = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D tex;
uniform float alpha;
void main() {
    vec4 c = texture(tex, vUV);
    FragColor = vec4(c.rgb, c.a * alpha);
}
)";

// --- ������ ��� ����� (������� � ����� �������) ---
const char* lineVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 proj;
void main() {
    gl_Position = proj * vec4(aPos, 0.0, 1.0);
}
)";
const char* lineFragmentShader = R"(
#version 330 core
uniform vec3 color;
out vec4 FragColor;
void main() {
    FragColor = vec4(color, 1.0);
}
)";

// --- ��� (36 ������, � uv, �������� � AO ��� ������ �����) ---
float cubeVertices[] = {
    // pos      // uv   // normal    // AO
    // �����
    0,0,1, 0,0, 0,0,1, 0,
    1,0,1, 1,0, 0,0,1, 0,
    1,1,1, 1,1, 0,0,1, 0,
    0,0,1, 0,0, 0,0,1, 0,
    1,1,1, 1,1, 0,0,1, 0,
    0,1,1, 0,1, 0,0,1, 0,
    // ���
    0,0,0, 1,0, 0,0,-1, 0,
    1,0,0, 0,0, 0,0,-1, 0,
    1,1,0, 0,1, 0,0,-1, 0,
    0,0,0, 1,0, 0,0,-1, 0,
    1,1,0, 0,1, 0,0,-1, 0,
    0,1,0, 1,1, 0,0,-1, 0,
    // ����
    0,0,0, 0,0, -1,0,0, 0,
    0,0,1, 1,0, -1,0,0, 0,
    0,1,1, 1,1, -1,0,0, 0,
    0,0,0, 0,0, -1,0,0, 0,
    0,1,1, 1,1, -1,0,0, 0,
    0,1,0, 0,1, -1,0,0, 0,
    // �����
    1,0,0, 1,0, 1,0,0, 0,
    1,0,1, 0,0, 1,0,0, 0,
    1,1,1, 0,1, 1,0,0, 0,
    1,0,0, 1,0, 1,0,0, 0,
    1,1,1, 0,1, 1,0,0, 0,
    1,1,0, 1,1, 1,0,0, 0,
    // ����
    0,1,0, 0,1, 0,1,0, 0,
    1,1,0, 1,1, 0,1,0, 0,
    1,1,1, 1,0, 0,1,0, 0,
    0,1,0, 0,1, 0,1,0, 0,
    1,1,1, 1,0, 0,1,0, 0,
    0,1,1, 0,0, 0,1,0, 0,
    // ���
    0,0,0, 0,0, 0,-1,0, 0,
    1,0,0, 1,0, 0,-1,0, 0,
    1,0,1, 1,1, 0,-1,0, 0,
    0,0,0, 0,0, 0,-1,0, 0,
    1,0,1, 1,1, 0,-1,0, 0,
    0,0,1, 0,1, 0,-1,0, 0,
};

// --- ���� ��� �������/������ ---
float quadVertices[] = {
    // pos   // uv
    0,0, 0,0,
    1,0, 1,0,
    1,1, 1,1,
    0,0, 0,0,
    1,1, 1,1,
    0,1, 0,1
};

// --- ������� ��� �������� (NDC)
float crosshairVertices[] = {
    0.0f, -0.02f,
    0.0f,  0.02f,
    -0.02f, 0.0f,
     0.02f, 0.0f
};

// --- ������� ��� ����� ������� (4 �����, 5 ��� ���������)
float hotbarFrameVertices[5 * 2];

// --- ������ 2D ---
float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float lerp(float a, float b, float t) { return a + t * (b - a); }
float grad(int hash, float x, float y) {
    int h = hash & 3;
    float u = h < 2 ? x : y;
    float v = h < 2 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}
int p[512];
void initPerlin() {
    int permutation[256] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,
        247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,
        74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
        65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,
        52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
        119,248,152, 2,44,154,163,70,221,153,101,155,167, 43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,
        246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214, 31,181,199,106,157,184,
        84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
    for (int i = 0; i < 256; i++) p[256 + i] = p[i] = permutation[i];
}
float perlin2d(float x, float y) {
    int X = int(floor(x)) & 255;
    int Y = int(floor(y)) & 255;
    x -= floor(x); y -= floor(y);
    float u = fade(x), v = fade(y);
    int A = p[X] + Y, AA = p[A], AB = p[A + 1];
    int B = p[X + 1] + Y, BA = p[B], BB = p[B + 1];
    return lerp(
        lerp(grad(p[AA], x, y), grad(p[BA], x - 1, y), u),
        lerp(grad(p[AB], x, y - 1), grad(p[BB], x - 1, y - 1), u),
        v
    );
}

// --- ��������� ���� � �������� ---
void generateWorld() {
    initPerlin();
    for (int x = 0; x < WORLD_X; ++x)
        for (int z = 0; z < WORLD_Z; ++z) {
            float fx = x * 0.15f, fz = z * 0.15f;
            float n = perlin2d(fx, fz) * 0.5f + 0.5f;
            int h = 6 + int(n * 7); // ������ �� 6 �� 13
            for (int y = 0; y < WORLD_Y; ++y) {
                if (y > h) world[x][y][z].type = AIR;
                else if (y == h) world[x][y][z].type = GRASS;
                else if (y > h - 3) world[x][y][z].type = DIRT;
                else world[x][y][z].type = STONE;
            }
        }
}

// --- ���������� ������� ---
GLuint createShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
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

// --- �������� �������� � ������� (AABB, � ������ ������� � ������) ---
bool checkCollision(glm::vec3 pos) {
    for (float y = 0.0f; y < playerHeight; y += BLOCK_SIZE / 2.0f) {
        glm::vec3 p = pos + glm::vec3(0, y, 0);
        for (int dx = -1; dx <= 1; ++dx)
            for (int dz = -1; dz <= 1; ++dz) {
                glm::vec3 check = p + glm::vec3(dx * playerRadius, 0, dz * playerRadius);
                int ix = int(floor(check.x / BLOCK_SIZE));
                int iy = int(floor(check.y / BLOCK_SIZE));
                int iz = int(floor(check.z / BLOCK_SIZE));
                if (ix < 0 || ix >= WORLD_X || iy < 0 || iy >= WORLD_Y || iz < 0 || iz >= WORLD_Z) continue;
                if (world[ix][iy][iz].type != AIR) return true;
            }
    }
    return false;
}

// --- Raycast ��� ������ ����� ---
struct RaycastResult {
    bool hit;
    int x, y, z;
    glm::ivec3 faceNormal;
};
RaycastResult raycast(glm::vec3 origin, glm::vec3 dir, float maxDist = 6.0f) {
    glm::vec3 pos = origin;
    for (float t = 0.0f; t < maxDist; t += 0.05f) {
        pos = origin + dir * t;
        int x = int(floor(pos.x / BLOCK_SIZE));
        int y = int(floor(pos.y / BLOCK_SIZE));
        int z = int(floor(pos.z / BLOCK_SIZE));
        if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y || z < 0 || z >= WORLD_Z) continue;
        if (world[x][y][z].type != AIR) {
            glm::vec3 local = (pos - glm::vec3((x + 0.5f) * BLOCK_SIZE, (y + 0.5f) * BLOCK_SIZE, (z + 0.5f) * BLOCK_SIZE)) / BLOCK_SIZE;
            glm::ivec3 normal(0);
            float maxc = std::max({ fabs(local.x), fabs(local.y), fabs(local.z) });
            if (fabs(local.x) == maxc) normal.x = (local.x > 0) ? 1 : -1;
            else if (fabs(local.y) == maxc) normal.y = (local.y > 0) ? 1 : -1;
            else if (fabs(local.z) == maxc) normal.z = (local.z > 0) ? 1 : -1;
            return { true, x, y, z, normal };
        }
    }
    return { false, 0, 0, 0, glm::ivec3(0) };
}

// --- Ambient Occlusion ��� ����� (���� ������ ���� ���� � ������) ---
float getBlockAO(int x, int y, int z) {
    if (y + 1 >= WORLD_Y) return 1.0f;
    if (world[x][y + 1][z].type != AIR) return 0.3f;
    return 1.0f;
}

// --- GLFW callbacks ��� ������� ������ ---
bool leftPressed = false, rightPressed = false;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) leftPressed = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_RIGHT) rightPressed = (action == GLFW_PRESS);
}

// --- ������ �������� � ������ ---
void drawCrosshair(GLuint crossVAO, GLuint lineProgram) {
    glUseProgram(lineProgram);
    glm::mat4 proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3f(glGetUniformLocation(lineProgram, "color"), 0.0f, 0.0f, 0.0f);
    glBindVertexArray(crossVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
}

// --- ������ ������� (3 �����, ��������� �������) ---
void drawHotbar(GLuint quadVAO, GLuint quadProgram, GLuint lineProgram, GLuint texGrass, GLuint texDirt, GLuint texStone) {
    float slotSize = 64.0f;
    float margin = 12.0f;
    float y = g_winHeight - slotSize - margin;
    float x0 = (g_winWidth - (slotSize * 3 + margin * 2)) / 2.0f;
    GLuint texArr[3] = { texGrass, texDirt, texStone };

    // --- ������ ---
    glUseProgram(quadProgram);
    glm::mat4 proj = glm::ortho(0.0f, float(g_winWidth), float(g_winHeight), 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(quadProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
    glBindVertexArray(quadVAO);
    for (int i = 0; i < 3; ++i) {
        float x = x0 + i * (slotSize + margin);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texArr[i]);
        glUniform1i(glGetUniformLocation(quadProgram, "tex"), 0);
        glUniform1f(glGetUniformLocation(quadProgram, "alpha"), 1.0f);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(slotSize, slotSize, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(quadProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj * model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
    glUseProgram(0);

    // --- ����� ---
    glUseProgram(lineProgram);
    glUniformMatrix4fv(glGetUniformLocation(lineProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
    for (int i = 0; i < 3; ++i) {
        float x = x0 + i * (slotSize + margin);
        // 4 ���� + ����������
        hotbarFrameVertices[0] = x;               hotbarFrameVertices[1] = y;
        hotbarFrameVertices[2] = x + slotSize;    hotbarFrameVertices[3] = y;
        hotbarFrameVertices[4] = x + slotSize;    hotbarFrameVertices[5] = y + slotSize;
        hotbarFrameVertices[6] = x;               hotbarFrameVertices[7] = y + slotSize;
        hotbarFrameVertices[8] = x;               hotbarFrameVertices[9] = y;
        GLuint frameVAO, frameVBO;
        glGenVertexArrays(1, &frameVAO);
        glGenBuffers(1, &frameVBO);
        glBindVertexArray(frameVAO);
        glBindBuffer(GL_ARRAY_BUFFER, frameVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(hotbarFrameVertices), hotbarFrameVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        if (i == selectedBlock)
            glUniform3f(glGetUniformLocation(lineProgram, "color"), 1.0f, 1.0f, 0.0f);
        else
            glUniform3f(glGetUniformLocation(lineProgram, "color"), 0.2f, 0.2f, 0.2f);
        glLineWidth(i == selectedBlock ? 4.0f : 2.0f);
        glDrawArrays(GL_LINE_STRIP, 0, 5);
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &frameVAO);
        glDeleteBuffers(1, &frameVBO);
    }
    glUseProgram(0);
}
/*void drawDebugText(float x, float y, const char* text, float r, float g, float b, float scale = 1.0f) {
    char buffer[99999]; // ~500 chars
    int num_quads;
    num_quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer));
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, g_winWidth, g_winHeight, 0, -1, 1);
    glColor3f(r, g, b);
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}*/
int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");
    // --- �����������: ����� ������ ---
    initSockets();
    printf("выберите режим (1 - сервер, 2 - клиент, 3 - хост): ");
    int mode = 0;
    std::cin >> mode;
    unsigned short port;
    char ip[64] = "127.0.0.1";
    if (mode == 1) {
        // ������ ������ (����������)
        isServer = true;
        myPlayerId = -1; // ������ �� ��������� ��� �����
        printf("Введите порт: ");
        std::cin >> port;
        for (int i = 0; i < MAX_PLAYERS; ++i) players[i].active = false;
        networkThread = std::thread(serverThreadFunc, port);
    }
    else if (mode == 2) {
        // ������ ������
        isServer = false;
        myPlayerId = 0;
        printf("Введите IP сервера: ");
        std::cin >> ip;
        printf("Введите порт: ");
        std::cin >> port;
        for (int i = 0; i < MAX_PLAYERS; ++i) players[i].active = false;
        players[0].active = true;
        networkThread = std::thread(clientThreadFunc, ip, port);
    }
    else if (mode == 3) {
        // ����: ������ + ������
        isServer = true;
        myPlayerId = 0;
        printf("Введие порт: ");
        std::cin >> port;
        for (int i = 0; i < MAX_PLAYERS; ++i) players[i].active = false;
        players[0].active = true;
        // ������� ������
        networkThread = std::thread(serverThreadFunc, port);
        // ���� ������� ����� ���������� (����� connect ����� �� ������)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // ����� ������ (localhost)
        std::thread(clientThreadFunc, "127.0.0.1", port).detach();
    }

    // --- OpenGL/GLFW ---
    GLFWwindow* window;
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    g_monitor = glfwGetPrimaryMonitor();
    g_mode = glfwGetVideoMode(g_monitor);
    g_winWidth = g_mode->width;
    g_winHeight = g_mode->height;
    window = glfwCreateWindow(g_winWidth, g_winHeight, "Mini Minecraft Parody", g_monitor, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // --- ��������� ���� ---
    generateWorld();

    // --- �������� ������� ---
    GLuint texGrass = loadBMP(TEXTURE_GRASS);
    GLuint texDirt = loadBMP(TEXTURE_DIRT);
    GLuint texStone = loadBMP(TEXTURE_STONE);

    // --- ��� VAO/VBO ---
    GLuint cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    // --- ���� ��� ������� ---
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- VAO/VBO ��� �������� ---
    GLuint crossVAO, crossVBO;
    glGenVertexArrays(1, &crossVAO);
    glGenBuffers(1, &crossVBO);
    glBindVertexArray(crossVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    GLuint fontTex = createFontTexture();

    // --- ��������� ---
    GLuint program = createProgram(vertexShaderSource, fragmentShaderSource);
    GLuint quadProgram = createProgram(quadVertexShader, quadFragmentShader);
    GLuint lineProgram = createProgram(lineVertexShader, lineFragmentShader);
    GLuint textProg = createProgram(textVertexShader, textFragmentShader);

    glEnable(GL_DEPTH_TEST);

    g_projection = glm::perspective(glm::radians(70.0f), (float)g_winWidth / (float)g_winHeight, 0.1f, 200.0f);

    double lastTime = glfwGetTime();
    double lastFPSTime = glfwGetTime();
    int frameCount = 0;
    double fps = 0.0;
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        frameCount++;
        if (now - lastFPSTime >= 1.0) {
            fps = frameCount / (now - lastFPSTime);
            lastFPSTime = now;
            frameCount = 0;
        }
        now = glfwGetTime();
        float deltaTime = float(now - lastTime);
        lastTime = now;

        // --- ��������� ���� ������� ��� �������� �� ���� ---
        {
            std::lock_guard<std::mutex> lock(netMutex);
            players[myPlayerId].x = cameraPos.x;
            players[myPlayerId].y = cameraPos.y;
            players[myPlayerId].z = cameraPos.z;
            players[myPlayerId].active = true;
        }

        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- ���������� ---
        float speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 8.0f : 4.0f;
        glm::vec3 move(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= glm::normalize(glm::cross(cameraFront, cameraUp));
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += glm::normalize(glm::cross(cameraFront, cameraUp));
        move.y = 0.0f;
        if (glm::length(move) > 0.0f) move = glm::normalize(move);

        // --- ���������� � ������ ---
        velocityY -= 18.0f * deltaTime;
        if (onGround && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            velocityY = 7.0f;
            onGround = false;
        }

        // --- ����������� � ������ �������� (��������� �� ����) ---
        glm::vec3 nextPos = cameraPos;
        // XZ
        glm::vec3 tryXZ = cameraPos + move * speed * deltaTime;
        tryXZ.y = cameraPos.y;
        if (!checkCollision(tryXZ)) {
            nextPos.x = tryXZ.x;
            nextPos.z = tryXZ.z;
        }
        // Y
        float tryY = cameraPos.y + velocityY * deltaTime;
        glm::vec3 tryPosY = nextPos;
        tryPosY.y = tryY;
        if (!checkCollision(tryPosY)) {
            nextPos.y = tryY;
            onGround = false;
        }
        else {
            if (velocityY < 0) onGround = true;
            velocityY = 0.0f;
        }
        cameraPos = nextPos;

        if (myPlayerId >= 0) {
            std::lock_guard<std::mutex> lock(netMutex);
            players[myPlayerId].x = cameraPos.x;
            players[myPlayerId].y = cameraPos.y;
            players[myPlayerId].z = cameraPos.z;
            players[myPlayerId].active = true;
        }

        // --- Raycast ��� ������ ����� ---
        RaycastResult pick = raycast(cameraPos + glm::vec3(0, playerHeight * 0.5f, 0), cameraFront);

        // --- ������/������� ����� ---
        static bool lastLeft = false, lastRight = false;
        if (pick.hit) {
            if (leftPressed && !lastLeft) {
                world[pick.x][pick.y][pick.z].type = AIR;
                PlaySound(TEXT("boing_x.wav"), NULL, SND_FILENAME | SND_ASYNC);
                // ��������� �� ������
                BlockChange change{ pick.x, pick.y, pick.z, AIR };
                send(sock, (char*)&change, sizeof(change), 0);
            }
            if (rightPressed && !lastRight) {
                int nx = pick.x + pick.faceNormal.x;
                int ny = pick.y + pick.faceNormal.y;
                int nz = pick.z + pick.faceNormal.z;
                if (nx >= 0 && nx < WORLD_X && ny >= 0 && ny < WORLD_Y && nz >= 0 && nz < WORLD_Z) {
                    if (world[nx][ny][nz].type == AIR) {
                        world[nx][ny][nz].type = inventory[selectedBlock];
                        // ��������� �� ������
                        BlockChange change{ nx, ny, nz, inventory[selectedBlock] };
                        send(sock, (char*)&change, sizeof(change), 0);
                    }
                }
            }

        }
        lastLeft = leftPressed;
        lastRight = rightPressed;

        // --- View/Projection ---
        glm::mat4 view = glm::lookAt(cameraPos + glm::vec3(0, playerHeight * 0.5f, 0), cameraPos + glm::vec3(0, playerHeight * 0.5f, 0) + cameraFront, cameraUp);

        // --- ������ ���� ������ ---
        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(g_projection));
        glUniform3f(glGetUniformLocation(program, "lightDir"), 0.5f, 1.0f, 0.3f);
        glUniform3f(glGetUniformLocation(program, "lightColor"), 1.0f, 0.95f, 0.85f);
        glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(cameraPos + glm::vec3(0, playerHeight * 0.5f, 0)));
        glUniform1i(glGetUniformLocation(program, "isPlayer"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        glUniform1i(glGetUniformLocation(program, "texGrass"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texDirt);
        glUniform1i(glGetUniformLocation(program, "texDirt"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texStone);
        glUniform1i(glGetUniformLocation(program, "texStone"), 2);

        glBindVertexArray(cubeVAO);

        glm::mat4 viewProjMatrix = g_projection * view;
        // --- Сборка всех видимых граней в один буфер ---
        std::vector<float> allFaces;
        std::vector<int> allTypes;
        std::vector<glm::mat4> allModels;

        for (int x = 0; x < WORLD_X; ++x) {
            for (int y = 0; y < WORLD_Y; ++y) {
                for (int z = 0; z < WORLD_Z; ++z) {
                    BlockType t = world[x][y][z].type;
                    if (t == AIR) continue;

                    glm::vec3 blockPos(x * BLOCK_SIZE, y * BLOCK_SIZE, z * BLOCK_SIZE);

                    if (glm::distance(cameraPos, blockPos) > 25.0f) continue;
                    if (!isBlockInFrustum(viewProjMatrix, blockPos, BLOCK_SIZE)) continue;

                    glm::vec3 faceOffsets[6] = {
                        {0.0f, 0.0f, 1.0f},
                        {0.0f, 0.0f, -1.0f},
                        {-1.0f, 0.0f, 0.0f},
                        {1.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f},
                        {0.0f, -1.0f, 0.0f}
                    };

                    for (int i = 0; i < 6; ++i) {
                        glm::vec3 offset = faceOffsets[i];
                        int nx = x + (int)offset.x;
                        int ny = y + (int)offset.y;
                        int nz = z + (int)offset.z;
                        if (!isFaceInFrustum(viewProjMatrix, blockPos, offset, BLOCK_SIZE)) continue;
                        if (nx >= 0 && nx < WORLD_X && ny >= 0 && ny < WORLD_Y && nz >= 0 && nz < WORLD_Z) {
                            if (world[nx][ny][nz].type != AIR) continue;
                        }

                        float ao = getBlockAO(x, y, z);
                        float coloredFace[6 * 9];
                        for (int j = 0; j < 6; ++j) {
                            int vertexIndex = i * 6 + j;
                            coloredFace[j * 9 + 0] = cubeVertices[vertexIndex * 9 + 0] * BLOCK_SIZE;
                            coloredFace[j * 9 + 1] = cubeVertices[vertexIndex * 9 + 1] * BLOCK_SIZE;
                            coloredFace[j * 9 + 2] = cubeVertices[vertexIndex * 9 + 2] * BLOCK_SIZE;
                            coloredFace[j * 9 + 3] = cubeVertices[vertexIndex * 9 + 3];
                            coloredFace[j * 9 + 4] = cubeVertices[vertexIndex * 9 + 4];
                            coloredFace[j * 9 + 5] = cubeVertices[vertexIndex * 9 + 5];
                            coloredFace[j * 9 + 6] = cubeVertices[vertexIndex * 9 + 6];
                            coloredFace[j * 9 + 7] = cubeVertices[vertexIndex * 9 + 7];
                            coloredFace[j * 9 + 8] = ao;
                        }
                        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(coloredFace), coloredFace);

                        glUniform1i(glGetUniformLocation(program, "blockType"), t);
                        glm::mat4 model = glm::translate(glm::mat4(1.0f), blockPos);
                        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }
                }
            }
        }


        // --- Отправка всех граней одним вызовом ---
        if (!allFaces.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, allFaces.size() * sizeof(float), allFaces.data(), GL_DYNAMIC_DRAW);
            for (size_t i = 0; i < allTypes.size(); ++i) {
                glUniform1i(glGetUniformLocation(program, "blockType"), allTypes[i]);
                glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(allModels[i]));
                glDrawArrays(GL_TRIANGLES, i * 6, 6);
            }
        }


        // --- ������ ������ ������� ��� �������������� ---
        glUseProgram(program);
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            std::lock_guard<std::mutex> lock(netMutex);
            if (!players[i].active) continue;
            // �� ������ ���� ������ �� ��� ����� ��������������� (�� ������� � �� ��������)
            if (i == myPlayerId) continue;
            if (glm::distance(glm::vec3(players[i].x, players[i].y, players[i].z), cameraPos) < BLOCK_SIZE)
                continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(players[i].x, players[i].y, players[i].z));
            model = glm::scale(model, glm::vec3(BLOCK_SIZE, BLOCK_SIZE * 1.8f, BLOCK_SIZE));
            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(glGetUniformLocation(program, "blockType"), 0); // �� ������������
            glUniform1i(glGetUniformLocation(program, "isPlayer"), 1);
            glUniform3fv(glGetUniformLocation(program, "playerColor"), 1, glm::value_ptr(playerColors[i % MAX_PLAYERS]));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glUniform1i(glGetUniformLocation(program, "isPlayer"), 0); // �������� ��� ������� ������

        // --- 2D Overlay ---
        drawCrosshair(crossVAO, lineProgram);
        drawHotbar(quadVAO, quadProgram, lineProgram, texGrass, texDirt, texStone);

        // --- Debug overlay ---
        if (showDebug) {
            /*auto textWidth = [](const char* text, float scale) {
                return (int)(strlen(text) * 8 * scale);
            };
            float legendY = 100 + 100 + 650; // всегда чуть выше графика, вне зависимости от разрешения
            float legendX = 100;
            float scale = 2.0f;

            // Формируем строки с текущими значениями
            char txt1[64], txt2[64];
            snprintf(txt1, sizeof(txt1), "FrameTime (ms): %.2f", frameTime * 1000.0);
            snprintf(txt2, sizeof(txt2), "FPS: %.1f", fps);

            // Рисуем каждую метку с цветом и значением
            drawTextGL(textProg, fontTex, w, h, legendX, legendY, txt1, scale, 1.0f, 0.8f, 0.2f);
            legendX += textWidth(txt1, scale) + 32;
            drawTextGL(textProg, fontTex, w, h, legendX, legendY, txt2, scale, 0.2f, 1.0f, 0.2f);
            legendX += textWidth(txt2, scale) + 32;
            */
           char debugText[1024];
            snprintf(debugText, sizeof(debugText),
                "FPS: %.1f\r\n"
                "Pos: (%.2f, %.2f, %.2f)\r\n"
                "Yaw: %.1f  Pitch: %.1f\r\n"
                "block: %d\r\n"
                "V-Y: %.2f  OnGr: %d\r\n"
                "Players: %d\r\n"
                "Pick: %s\r\n",
                fps,
                cameraPos.x, cameraPos.y, cameraPos.z,
                yaw, pitch,
                selectedBlock,
                velocityY, (int)onGround,
                [](){
                    int c=0; for(int i=0;i<MAX_PLAYERS;++i) if(players[i].active) ++c; return c;
                }(),
                pick.hit ? "YES" : "NO"
            );
            drawTextGL(textProg, fontTex, g_winWidth, g_winHeight, 1, 1, debugText, 2, 1.0f, 0.8f, 0.2f);
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &crossVAO);
    glDeleteBuffers(1, &crossVBO);
    glDeleteProgram(program);
    glDeleteProgram(quadProgram);
    glDeleteProgram(lineProgram);
    glDeleteProgram(textProg);

    // --- ���������� ������������ ---
    networkRunning = false;
    if (networkThread.joinable()) networkThread.join();
    cleanupSockets();

    glfwTerminate();
    return 0;
}
