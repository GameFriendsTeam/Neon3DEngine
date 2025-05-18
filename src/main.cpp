#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
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

// --- Пути к текстурам ---
#define TEXTURE_GRASS "textures/grass.bmp"
#define TEXTURE_DIRT  "textures/dirt.bmp"
#define TEXTURE_STONE "textures/stone.bmp"

// --- Размер блока ---
const float BLOCK_SIZE = 0.5f;

// --- Глобальные переменные ---
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
const float playerHeight = BLOCK_SIZE * 1.8f;
const float playerRadius = BLOCK_SIZE * 0.3f;

// --- Инвентарь ---
enum BlockType { AIR = 0, GRASS = 1, DIRT = 2, STONE = 3 };
BlockType inventory[] = { GRASS, DIRT, STONE };
int inventorySize = sizeof(inventory) / sizeof(BlockType);
int selectedBlock = 0;

// --- Настройки мира ---
const int WORLD_X = 32, WORLD_Y = 16, WORLD_Z = 32;
struct Block {
    BlockType type;
};
Block world[WORLD_X][WORLD_Y][WORLD_Z];

// --- Мультиплеер ---
constexpr int MAX_PLAYERS = 4;
struct PlayerNetState {
    float x, y, z;
    bool active;
};
std::mutex netMutex;
PlayerNetState players[MAX_PLAYERS]; // [0] - сервер, [1..] - клиенты
int myPlayerId = 0; // id текущего игрока
std::atomic<bool> networkRunning{ false };
SOCKET sock = 0;
std::thread networkThread;
bool isServer = false;

// --- Сетевые функции ---
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

    // Сделать listenSock неблокирующим
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(listenSock, FIONBIO, &mode);
#else
    int flags = fcntl(listenSock, F_GETFL, 0);
    fcntl(listenSock, F_SETFL, flags | O_NONBLOCK);
#endif

    std::vector<SOCKET> clients;
    printf("Ожидание клиентов на порту %d...\n", port);
    networkRunning = true;
    while (networkRunning) {
        // Принимаем новых клиентов
        SOCKET cs = accept(listenSock, nullptr, nullptr);
        if (cs != -1 && clients.size() < MAX_PLAYERS - 1) {
            // Сделать cs неблокирующим
#ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(cs, FIONBIO, &mode);
#else
            int flags = fcntl(cs, F_GETFL, 0);
            fcntl(cs, F_SETFL, flags | O_NONBLOCK);
#endif
            clients.push_back(cs);
            printf("Клиент подключился! id=%d\n", (int)clients.size());
        }
        // Получаем позиции от клиентов
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
        // Отправляем всем клиентам состояния всех игроков
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
    printf("Подключение к %s:%d...\n", ip, port);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Ошибка подключения!\n");
        return;
    }
    // Сделать sock неблокирующим
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
    printf("Подключено к серверу!\n");
    networkRunning = true;
    while (networkRunning) {
        // Отправляем свою позицию
        float pos[3];
        {
            std::lock_guard<std::mutex> lock(netMutex);
            pos[0] = players[0].x;
            pos[1] = players[0].y;
            pos[2] = players[0].z;
        }
        send(sock, (char*)pos, sizeof(pos), 0);
        // Получаем состояния всех игроков
        int recvd = recv(sock, (char*)players, sizeof(players), 0);
        if (recvd == sizeof(players)) {
            // ok
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    closesocket(sock);
}


// --- Callback для изменения размера окна ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    g_winWidth = width;
    g_winHeight = height;
    glViewport(0, 0, width, height);
    g_projection = glm::perspective(glm::radians(70.0f), (float)width / (float)height, 0.1f, 200.0f);
}

// --- Обработка мыши ---
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

// --- Обработка колесика мыши для инвентаря ---
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset > 0) selectedBlock = (selectedBlock + 1) % inventorySize;
    if (yoffset < 0) selectedBlock = (selectedBlock - 1 + inventorySize) % inventorySize;
}

// --- Обработка клавиш 1/2/3 для выбора слота ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) selectedBlock = 0;
        if (key == GLFW_KEY_2 && inventorySize > 1) selectedBlock = 1;
        if (key == GLFW_KEY_3 && inventorySize > 2) selectedBlock = 2;
    }
}

// --- Загрузка BMP-текстуры ---
GLuint loadBMP(const char* imagepath) {
    unsigned char header[54];
    unsigned int dataPos, width, height, imageSize;
    unsigned char* data;

    FILE* file = fopen(imagepath, "rb");
    if (!file) {
        std::cerr << "Не удалось открыть BMP: " << imagepath << std::endl;
        return 0;
    }
    if (fread(header, 1, 54, file) != 54) {
        std::cerr << "Неверный BMP header: " << imagepath << std::endl;
        fclose(file);
        return 0;
    }
    if (header[0] != 'B' || header[1] != 'M') {
        std::cerr << "Файл не BMP: " << imagepath << std::endl;
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

    // BMP хранит цвета в BGR, OpenGL ожидает RGB
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

// --- Шейдеры ---
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
void main() {
    float ambient = 0.25 + 0.25 * vAO;
    vec3 norm = normalize(vNormal);
    float diff = max(dot(norm, normalize(-lightDir)), 0.0);
    vec3 viewDir = normalize(viewPos - vWorldPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0) * 0.25;
    float lighting = ambient + diff * 0.7 + spec;
    vec4 texColor;
    if (blockType == 1)
        texColor = texture(texGrass, vUV);
    else if (blockType == 2)
        texColor = texture(texDirt, vUV);
    else if (blockType == 3)
        texColor = texture(texStone, vUV);
    else
        texColor = vec4(1,0,1,1);
    FragColor = vec4(texColor.rgb * lighting * lightColor, texColor.a);
}
)";

// --- Шейдер для хотбара и иконок (2D) ---
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

// --- Шейдер для линий (крестик и рамки хотбара) ---
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

// --- Куб (36 вершин, с uv, нормалью и AO для каждой грани) ---
float cubeVertices[] = {
    // pos      // uv   // normal    // AO
    // Перед
    0,0,1, 0,0, 0,0,1, 0,
    1,0,1, 1,0, 0,0,1, 0,
    1,1,1, 1,1, 0,0,1, 0,
    0,0,1, 0,0, 0,0,1, 0,
    1,1,1, 1,1, 0,0,1, 0,
    0,1,1, 0,1, 0,0,1, 0,
    // Зад
    0,0,0, 1,0, 0,0,-1, 0,
    1,0,0, 0,0, 0,0,-1, 0,
    1,1,0, 0,1, 0,0,-1, 0,
    0,0,0, 1,0, 0,0,-1, 0,
    1,1,0, 0,1, 0,0,-1, 0,
    0,1,0, 1,1, 0,0,-1, 0,
    // Лево
    0,0,0, 0,0, -1,0,0, 0,
    0,0,1, 1,0, -1,0,0, 0,
    0,1,1, 1,1, -1,0,0, 0,
    0,0,0, 0,0, -1,0,0, 0,
    0,1,1, 1,1, -1,0,0, 0,
    0,1,0, 0,1, -1,0,0, 0,
    // Право
    1,0,0, 1,0, 1,0,0, 0,
    1,0,1, 0,0, 1,0,0, 0,
    1,1,1, 0,1, 1,0,0, 0,
    1,0,0, 1,0, 1,0,0, 0,
    1,1,1, 0,1, 1,0,0, 0,
    1,1,0, 1,1, 1,0,0, 0,
    // Верх
    0,1,0, 0,1, 0,1,0, 0,
    1,1,0, 1,1, 0,1,0, 0,
    1,1,1, 1,0, 0,1,0, 0,
    0,1,0, 0,1, 0,1,0, 0,
    1,1,1, 1,0, 0,1,0, 0,
    0,1,1, 0,0, 0,1,0, 0,
    // Низ
    0,0,0, 0,0, 0,-1,0, 0,
    1,0,0, 1,0, 0,-1,0, 0,
    1,0,1, 1,1, 0,-1,0, 0,
    0,0,0, 0,0, 0,-1,0, 0,
    1,0,1, 1,1, 0,-1,0, 0,
    0,0,1, 0,1, 0,-1,0, 0,
};

// --- Квад для хотбара/иконок ---
float quadVertices[] = {
    // pos   // uv
    0,0, 0,0,
    1,0, 1,0,
    1,1, 1,1,
    0,0, 0,0,
    1,1, 1,1,
    0,1, 0,1
};

// --- Вершины для крестика (NDC)
float crosshairVertices[] = {
    0.0f, -0.03f,
    0.0f,  0.03f,
    -0.03f, 0.0f,
     0.03f, 0.0f
};

// --- Вершины для рамки хотбара (4 точки, 5 для замыкания)
float hotbarFrameVertices[5 * 2];

// --- Перлин 2D ---
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

// --- Генерация мира с Перлином ---
void generateWorld() {
    initPerlin();
    for (int x = 0; x < WORLD_X; ++x)
        for (int z = 0; z < WORLD_Z; ++z) {
            float fx = x * 0.15f, fz = z * 0.15f;
            float n = perlin2d(fx, fz) * 0.5f + 0.5f;
            int h = 6 + int(n * 7); // высота от 6 до 13
            for (int y = 0; y < WORLD_Y; ++y) {
                if (y > h) world[x][y][z].type = AIR;
                else if (y == h) world[x][y][z].type = GRASS;
                else if (y > h - 3) world[x][y][z].type = DIRT;
                else world[x][y][z].type = STONE;
            }
        }
}

// --- Компиляция шейдера ---
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

// --- Проверка коллизии с блоками (AABB, с учётом радиуса и высоты) ---
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

// --- Raycast для выбора блока ---
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

// --- Ambient Occlusion для блока (если сверху есть блок — темнее) ---
float getBlockAO(int x, int y, int z) {
    if (y + 1 >= WORLD_Y) return 1.0f;
    if (world[x][y + 1][z].type != AIR) return 0.3f;
    return 1.0f;
}

// --- GLFW callbacks для мышиных кликов ---
bool leftPressed = false, rightPressed = false;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) leftPressed = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_RIGHT) rightPressed = (action == GLFW_PRESS);
}

// --- Рендер крестика в центре ---
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

// --- Рендер хотбара (3 слота, выбранный выделен) ---
void drawHotbar(GLuint quadVAO, GLuint quadProgram, GLuint lineProgram, GLuint texGrass, GLuint texDirt, GLuint texStone) {
    float slotSize = 64.0f;
    float margin = 12.0f;
    float y = g_winHeight - slotSize - margin;
    float x0 = (g_winWidth - (slotSize * 3 + margin * 2)) / 2.0f;
    GLuint texArr[3] = { texGrass, texDirt, texStone };

    // --- Иконки ---
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

    // --- Рамки ---
    glUseProgram(lineProgram);
    glUniformMatrix4fv(glGetUniformLocation(lineProgram, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
    for (int i = 0; i < 3; ++i) {
        float x = x0 + i * (slotSize + margin);
        // 4 угла + замыкающая
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

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");
    // --- Мультиплеер: выбор режима ---
    initSockets();
    printf("Выберите режим (1 - сервер, 2 - клиент, 3 - хост): ");
    int mode = 0;
    std::cin >> mode;
    unsigned short port;
    char ip[64] = "127.0.0.1";
    if (mode == 1) {
        // Только сервер (выделенный)
        isServer = true;
        myPlayerId = -1; // Сервер не участвует как игрок
        printf("Введите порт для сервера: ");
        std::cin >> port;
        for (int i = 0; i < MAX_PLAYERS; ++i) players[i].active = false;
        networkThread = std::thread(serverThreadFunc, port);
    }
    else if (mode == 2) {
        // Только клиент
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
        // Хост: сервер + клиент
        isServer = true;
        myPlayerId = 0;
        printf("Введите порт для хоста: ");
        std::cin >> port;
        for (int i = 0; i < MAX_PLAYERS; ++i) players[i].active = false;
        players[0].active = true;
        // Сначала сервер
        networkThread = std::thread(serverThreadFunc, port);
        // Дать серверу время стартовать (иначе connect может не успеть)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // Затем клиент (localhost)
        std::thread(clientThreadFunc, "127.0.0.1", port).detach();
    }

    // --- OpenGL/GLFW ---
    GLFWwindow* window;
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(g_winWidth, g_winHeight, "Mini Minecraft Parody", NULL, NULL);
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

    // --- Генерация мира ---
    generateWorld();

    // --- Загрузка текстур ---
    GLuint texGrass = loadBMP(TEXTURE_GRASS);
    GLuint texDirt = loadBMP(TEXTURE_DIRT);
    GLuint texStone = loadBMP(TEXTURE_STONE);

    // --- Куб VAO/VBO ---
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

    // --- Квад для хотбара ---
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

    // --- VAO/VBO для крестика ---
    GLuint crossVAO, crossVBO;
    glGenVertexArrays(1, &crossVAO);
    glGenBuffers(1, &crossVBO);
    glBindVertexArray(crossVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // --- Программы ---
    GLuint program = createProgram(vertexShaderSource, fragmentShaderSource);
    GLuint quadProgram = createProgram(quadVertexShader, quadFragmentShader);
    GLuint lineProgram = createProgram(lineVertexShader, lineFragmentShader);

    glEnable(GL_DEPTH_TEST);

    g_projection = glm::perspective(glm::radians(70.0f), (float)g_winWidth / (float)g_winHeight, 0.1f, 200.0f);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float deltaTime = float(now - lastTime);
        lastTime = now;

        // --- Сохраняем свою позицию для передачи по сети ---
        {
            std::lock_guard<std::mutex> lock(netMutex);
            players[myPlayerId].x = cameraPos.x;
            players[myPlayerId].y = cameraPos.y;
            players[myPlayerId].z = cameraPos.z;
            players[myPlayerId].active = true;
        }

        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Управление ---
        float speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 8.0f : 4.0f;
        glm::vec3 move(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move += cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move -= cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move -= glm::normalize(glm::cross(cameraFront, cameraUp));
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move += glm::normalize(glm::cross(cameraFront, cameraUp));
        move.y = 0.0f;
        if (glm::length(move) > 0.0f) move = glm::normalize(move);

        // --- Гравитация и прыжок ---
        velocityY -= 18.0f * deltaTime;
        if (onGround && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            velocityY = 7.0f;
            onGround = false;
        }

        // --- Перемещение с учётом коллизий (раздельно по осям) ---
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

        // --- Raycast для выбора блока ---
        RaycastResult pick = raycast(cameraPos + glm::vec3(0, playerHeight * 0.5f, 0), cameraFront);

        // --- Ломать/ставить блоки ---
        static bool lastLeft = false, lastRight = false;
        if (pick.hit) {
            if (leftPressed && !lastLeft) {
                world[pick.x][pick.y][pick.z].type = AIR;
            }
            if (rightPressed && !lastRight) {
                int nx = pick.x + pick.faceNormal.x;
                int ny = pick.y + pick.faceNormal.y;
                int nz = pick.z + pick.faceNormal.z;
                if (nx >= 0 && nx < WORLD_X && ny >= 0 && ny < WORLD_Y && nz >= 0 && nz < WORLD_Z) {
                    if (world[nx][ny][nz].type == AIR)
                        world[nx][ny][nz].type = inventory[selectedBlock];
                }
            }
        }
        lastLeft = leftPressed;
        lastRight = rightPressed;

        // --- View/Projection ---
        glm::mat4 view = glm::lookAt(cameraPos + glm::vec3(0, playerHeight * 0.5f, 0), cameraPos + glm::vec3(0, playerHeight * 0.5f, 0) + cameraFront, cameraUp);

        // --- Рендер всех блоков ---
        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(g_projection));
        glUniform3f(glGetUniformLocation(program, "lightDir"), 0.5f, 1.0f, 0.3f);
        glUniform3f(glGetUniformLocation(program, "lightColor"), 1.0f, 0.95f, 0.85f);
        glUniform3fv(glGetUniformLocation(program, "viewPos"), 1, glm::value_ptr(cameraPos + glm::vec3(0, playerHeight * 0.5f, 0)));

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

        for (int x = 0; x < WORLD_X; ++x)
            for (int y = 0; y < WORLD_Y; ++y)
                for (int z = 0; z < WORLD_Z; ++z) {
                    BlockType t = world[x][y][z].type;
                    if (t == AIR) continue;
                    // Не рисуем внутренние блоки
                    bool hidden = true;
                    for (int dx = -1; dx <= 1 && hidden; ++dx)
                        for (int dy = -1; dy <= 1 && hidden; ++dy)
                            for (int dz = -1; dz <= 1 && hidden; ++dz) {
                                if (abs(dx) + abs(dy) + abs(dz) != 1) continue;
                                int nx = x + dx, ny = y + dy, nz = z + dz;
                                if (nx < 0 || nx >= WORLD_X || ny < 0 || ny >= WORLD_Y || nz < 0 || nz >= WORLD_Z) { hidden = false; break; }
                                if (world[nx][ny][nz].type == AIR) { hidden = false; break; }
                            }
                    if (hidden) continue;

                    float ao = getBlockAO(x, y, z);
                    float coloredCube[36 * 9];
                    for (int i = 0; i < 36; ++i) {
                        coloredCube[i * 9 + 0] = cubeVertices[i * 9 + 0] * BLOCK_SIZE;
                        coloredCube[i * 9 + 1] = cubeVertices[i * 9 + 1] * BLOCK_SIZE;
                        coloredCube[i * 9 + 2] = cubeVertices[i * 9 + 2] * BLOCK_SIZE;
                        coloredCube[i * 9 + 3] = cubeVertices[i * 9 + 3];
                        coloredCube[i * 9 + 4] = cubeVertices[i * 9 + 4];
                        coloredCube[i * 9 + 5] = cubeVertices[i * 9 + 5];
                        coloredCube[i * 9 + 6] = cubeVertices[i * 9 + 6];
                        coloredCube[i * 9 + 7] = cubeVertices[i * 9 + 7];
                        coloredCube[i * 9 + 8] = ao;
                    }
                    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(coloredCube), coloredCube);

                    glUniform1i(glGetUniformLocation(program, "blockType"), t);

                    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x * BLOCK_SIZE, y * BLOCK_SIZE, z * BLOCK_SIZE));
                    glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

        // --- Рендер других игроков как прямоугольники ---
        glUseProgram(program);
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            if (i == myPlayerId) continue; // не рисуем себя
            std::lock_guard<std::mutex> lock(netMutex);
            if (!players[i].active) continue;
            // Дополнительно: не рисуем игрока, если его позиция почти совпадает с вашей (на случай ошибок id)
            if (glm::distance(glm::vec3(players[i].x, players[i].y, players[i].z), cameraPos) < 0.01f)
                continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(players[i].x, players[i].y, players[i].z));
            model = glm::scale(model, glm::vec3(BLOCK_SIZE, BLOCK_SIZE * 1.8f, BLOCK_SIZE));
            glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(glGetUniformLocation(program, "blockType"), 2); // DIRT
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // --- 2D Overlay ---
        drawCrosshair(crossVAO, lineProgram);
        drawHotbar(quadVAO, quadProgram, lineProgram, texGrass, texDirt, texStone);

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

    // --- Завершение мультиплеера ---
    networkRunning = false;
    if (networkThread.joinable()) networkThread.join();
    cleanupSockets();

    glfwTerminate();
    return 0;
}
