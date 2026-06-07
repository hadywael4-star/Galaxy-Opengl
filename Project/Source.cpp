#include <iostream>
#include <vector>
#include <gl/glew/glew.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <gl/glm/glm.hpp>
#include <gl/glm/gtc/type_ptr.hpp>
#include <gl/glm/gtx/transform.hpp>
#include <string>
#include <cmath>

using namespace std;
using namespace glm;

const GLint WIDTH = 1200;
const GLint HEIGHT = 900;

// Camera configuration
vec3 cameraPos = vec3(0.0f, 2.0f, 20.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);

float cameraYaw = -90.0f;
float cameraPitch = -10.0f;
float cameraSpeed = 0.02f;
float mouseSensitivity = 0.12f;
bool firstMouse = true;
float lastMouseX = WIDTH / 2.0f;
float lastMouseY = HEIGHT / 2.0f;

// Projection mode toggle: false = perspective, true = orthographic
bool useOrthographic = false;

// Shader programs
GLuint skyProgram;
GLuint phongProgram;

// Uniform locations - skybox
GLuint sky_modelLoc, sky_viewLoc, sky_projLoc;
GLuint sky_cubeMapLoc;

// Uniform locations - phong
GLuint ph_modelLoc, ph_viewLoc, ph_projLoc;
GLuint ph_cameraPosLoc, ph_colorMapLoc, ph_lightPosLoc, ph_emissiveLoc;

// Textures
GLuint CubeMapId;      // skybox cube map

// VBOs
GLuint skyVBO, skyIBO, skyVAO;
GLuint sphereVAO, sphereVBO;

// Planet data
struct Planet
{
    float orbitDistance;
    float size;
    float orbitSpeed;
    float rotationSpeed;
    float tilt;
    float orbitAngle;
    float rotationAngle;
    GLuint textureID;
};

Planet sun;
Planet planets[8];
Planet moon;

GLuint InitShader(const char* vertex_shader_file_name, const char* fragment_shader_file_name);

// Vertex struct with position, normal, UV
struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
};

vector<Vertex> sphere_vertices;

void SphereTriangle(vec3 a, vec3 b, vec3 c)
{
    auto makeVert = [](vec3 p) -> Vertex {
        Vertex v;
        v.position = p;
        v.normal = normalize(p);
        v.uv.x = 0.5f + atan2f(p.z, p.x) / (2.0f * 3.14159265f);
        v.uv.y = 0.5f - asinf(p.y) / 3.14159265f;
        return v;
    };
    sphere_vertices.push_back(makeVert(a));
    sphere_vertices.push_back(makeVert(b));
    sphere_vertices.push_back(makeVert(c));
}

void dividTriangle(vec3 a, vec3 b, vec3 c, int iterations)
{
    if (iterations > 0)
    {
        vec3 v1 = normalize(a + b);
        vec3 v2 = normalize(a + c);
        vec3 v3 = normalize(b + c);
        dividTriangle(a, v1, v2, iterations - 1);
        dividTriangle(v1, b, v3, iterations - 1);
        dividTriangle(v1, v3, v2, iterations - 1);
        dividTriangle(v2, v3, c, iterations - 1);
    }
    else
    {
        SphereTriangle(a, b, c);
    }
}

void CreateSphere(int iterations)
{
    vec3 base[4] = {
        vec3(0.0f,0.0f,1.0f),
        vec3(0.0f,0.942809f,-0.333333f),
        vec3(-0.816497f,-0.471405f,-0.333333f),
        vec3(0.816497f,-0.471405f,-0.333333f)
    };

    sphere_vertices.clear();
    dividTriangle(base[0], base[1], base[2], iterations);
    dividTriangle(base[0], base[3], base[1], iterations);
    dividTriangle(base[0], base[2], base[3], iterations);
    dividTriangle(base[3], base[2], base[1], iterations);

    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphere_vertices.size() * sizeof(Vertex), sphere_vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0); //posiiton
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (char*)sizeof(vec3)); //normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (char*)(2 * sizeof(vec3))); //uv
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// Skybox cube
vec3 skybox_verts[] = {
    vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f), vec3(0.5f, 0.5f, 0.5f),
    vec3(0.5f, 0.5f,-0.5f), vec3(0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f, 0.5f,-0.5f)
};

int skybox_indices[] = {
    0,1,2, 0,2,3,   // front
    3,2,5, 3,5,4,   // right
    4,5,6, 4,6,7,   // back
    7,6,1, 7,1,0,   // left
    7,0,3, 7,3,4,   // top
    2,1,6, 2,6,5    // bottom
};

void CreateSkybox()
{
    glGenVertexArrays(1, &skyVAO);
    glBindVertexArray(skyVAO);

    glGenBuffers(1, &skyVBO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_verts), skybox_verts, GL_STATIC_DRAW);

    glGenBuffers(1, &skyIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skybox_indices), skybox_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

GLuint LoadPlanetTexture(const char* filename)
{
    sf::Image img;
    string path = string("..\\Textures\\") + filename;
    if (!img.loadFromFile(path))
    {
        cout << "Error loading " << path << endl;
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    cout << "Loaded texture: " << path << endl;
    return tex;
}

void LoadCubeMap()
{
    string faces[6] = {
        "..\\Skybox\\right.png",
        "..\\Skybox\\left.png",
        "..\\Skybox\\top.png",
        "..\\Skybox\\bot.png",
        "..\\Skybox\\front.png",
        "..\\Skybox\\back.png"
    };

    glGenTextures(1, &CubeMapId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, CubeMapId);

    sf::Image img;
    for (int i = 0; i < 6; i++)
    {
        if (!img.loadFromFile(faces[i]))
        {
            cout << "Failed: " << faces[i] << endl;
            continue;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
        cout << "Loaded: " << faces[i] << endl;
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void InitPlanet(Planet& p, float dist, float sz, float orbSpd, float rotSpd, float tl, GLuint tex)
{
    p.orbitDistance = dist;
    p.size = sz;
    p.orbitSpeed = orbSpd;
    p.rotationSpeed = rotSpd;
    p.tilt = tl;
    p.orbitAngle = 0.0f;
    p.rotationAngle = 0.0f;
    p.textureID = tex;
}

void InitSolarSystem()
{
    sun.size = 1.5f;
    sun.rotationSpeed = 0.15f;
    sun.tilt = 0.0f;
    sun.rotationAngle = 0.0f;
    sun.textureID = LoadPlanetTexture("sun.jpg");

    InitPlanet(planets[0], 2.2f, 0.15f, 4.0f, 0.8f, 0.03f, LoadPlanetTexture("mercury.jpg"));
    InitPlanet(planets[1], 3.0f, 0.25f, 2.5f, 0.3f, 2.00f, LoadPlanetTexture("venus.jpg"));
    InitPlanet(planets[2], 3.8f, 0.30f, 1.8f, 1.5f, 0.41f, LoadPlanetTexture("earth.jpg"));
    InitPlanet(planets[3], 4.6f, 0.20f, 1.5f, 1.2f, 0.44f, LoadPlanetTexture("mars.jpg"));
    InitPlanet(planets[4], 5.8f, 0.65f, 0.9f, 2.0f, 0.05f, LoadPlanetTexture("jupiter.jpg"));
    InitPlanet(planets[5], 7.0f, 0.50f, 0.7f, 1.8f, 0.47f, LoadPlanetTexture("saturn.jpg"));
    InitPlanet(planets[6], 8.2f, 0.35f, 0.5f, 1.3f, 1.71f, LoadPlanetTexture("uranus.jpg"));
    InitPlanet(planets[7], 9.4f, 0.30f, 0.4f, 1.2f, 0.49f, LoadPlanetTexture("neptune.jpg"));

    moon.orbitDistance = 0.6f;
    moon.size = 0.08f;
    moon.orbitSpeed = 5.0f;
    moon.rotationSpeed = 0.5f;
    moon.tilt = 0.0f;
    moon.orbitAngle = 0.0f;
    moon.rotationAngle = 0.0f;
    moon.textureID = LoadPlanetTexture("moon.jpg");
}

int Init()
{
    GLenum err = glewInit();
    if (err != GLEW_OK) { cout << "GLEW Error"; return 1; }

    skyProgram = InitShader("Skybox_VS.glsl", "Skybox_FS.glsl");
    phongProgram = InitShader("VS_Tex.glsl", "FS_Tex.glsl");

    glUseProgram(skyProgram);
    sky_modelLoc = glGetUniformLocation(skyProgram, "modelMat");
    sky_viewLoc = glGetUniformLocation(skyProgram, "viewMat");
    sky_projLoc = glGetUniformLocation(skyProgram, "projMat");
    sky_cubeMapLoc = glGetUniformLocation(skyProgram, "cube_map");

    glUseProgram(phongProgram);
    ph_modelLoc = glGetUniformLocation(phongProgram, "modelMat");
    ph_viewLoc = glGetUniformLocation(phongProgram, "viewMat");
    ph_projLoc = glGetUniformLocation(phongProgram, "projMat");
    ph_cameraPosLoc = glGetUniformLocation(phongProgram, "cameraPos");
    ph_colorMapLoc = glGetUniformLocation(phongProgram, "color_map");
    ph_lightPosLoc = glGetUniformLocation(phongProgram, "lightPos");
    ph_emissiveLoc = glGetUniformLocation(phongProgram, "emissive");

    CreateSkybox();
    CreateSphere(6); 

    LoadCubeMap();
    InitSolarSystem();

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    return 0;
}

void DrawSphere(mat4 model)
{
    glUniformMatrix4fv(ph_modelLoc, 1, GL_FALSE, value_ptr(model));
    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sphere_vertices.size());
}

void Update()
{
    vec3 right = normalize(cross(cameraFront, cameraUp));

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) cameraPos += cameraFront * cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) cameraPos -= cameraFront * cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) cameraPos -= right * cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) cameraPos += right * cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) cameraPos.y += cameraSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) cameraPos.y -= cameraSpeed;
}

void Render()
{
    
    float timeScale = 0.1f; 

    sun.rotationAngle += sun.rotationSpeed * timeScale;
    for (int i = 0; i < 8; i++)
    {
        planets[i].orbitAngle += planets[i].orbitSpeed * timeScale;
        planets[i].rotationAngle += planets[i].rotationSpeed * timeScale;
    }

    moon.orbitAngle += moon.orbitSpeed * timeScale;
    moon.rotationAngle += moon.rotationSpeed * timeScale;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 viewMat = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    mat4 projMat;
    if (useOrthographic)
    {
        float orthoSize = 12.0f; // half-height of the view volume
        float aspect = (float)WIDTH / (float)HEIGHT;
        projMat = glm::ortho(-orthoSize * aspect, orthoSize * aspect,-orthoSize, orthoSize, 0.1f, 200.0f);
    }
    else
    {
        projMat = perspectiveFov(60.0f, (float)WIDTH, (float)HEIGHT, 0.1f, 100.0f);
    }

    // PASS 1: Skybox
    glUseProgram(skyProgram);
    glUniformMatrix4fv(sky_viewLoc, 1, GL_FALSE, value_ptr(viewMat));
    glUniformMatrix4fv(sky_projLoc, 1, GL_FALSE, value_ptr(projMat));

    mat4 skyModel = translate(cameraPos) * scale(vec3(100.0f, 100.0f, 100.0f));
    glUniformMatrix4fv(sky_modelLoc, 1, GL_FALSE, value_ptr(skyModel));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, CubeMapId);
    glUniform1i(sky_cubeMapLoc, 0);

    glDepthMask(GL_FALSE);
    glBindVertexArray(skyVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);

    // PASS 2: Solar system with Phong + texture
    glUseProgram(phongProgram);
    glUniformMatrix4fv(ph_viewLoc, 1, GL_FALSE, value_ptr(viewMat));
    glUniformMatrix4fv(ph_projLoc, 1, GL_FALSE, value_ptr(projMat));
    glUniform3f(ph_cameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(ph_lightPosLoc, 0.0f, 0.0f, 0.0f); // Sun is light source

    // Draw Sun 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sun.textureID);
    glUniform1i(ph_colorMapLoc, 0);
    glUniform1f(ph_emissiveLoc, 1.0f);
    DrawSphere(rotate(radians(sun.rotationAngle), vec3(0.0f, 1.0f, 0.0f)) * scale(vec3(sun.size)));

    // Draw planets
    glUniform1f(ph_emissiveLoc, 0.0f);
    for (int i = 0; i < 8; i++)
    {
        float rad = radians(planets[i].orbitAngle);
        float x = planets[i].orbitDistance * cos(rad);
        float z = planets[i].orbitDistance * sin(rad);
        mat4 model = translate(vec3(x, 0.0f, z))
            * rotate(planets[i].tilt, vec3(0.0f, 0.0f, 1.0f))
            * rotate(radians(planets[i].rotationAngle), vec3(0.0f, 1.0f, 0.0f))
            * scale(vec3(planets[i].size));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, planets[i].textureID);
        glUniform1i(ph_colorMapLoc, 0);
        DrawSphere(model);
    }

    // Draw Moon (orbiting Earth - index 2)
    vec3 earthPos = vec3(
        planets[2].orbitDistance * cos(radians(planets[2].orbitAngle)),
        0.0f,
        planets[2].orbitDistance * sin(radians(planets[2].orbitAngle)));
    float mx = earthPos.x + moon.orbitDistance * cos(radians(moon.orbitAngle));
    float mz = earthPos.z + moon.orbitDistance * sin(radians(moon.orbitAngle));
    mat4 moonModel = translate(vec3(mx, 0.0f, mz))
        * rotate(radians(moon.rotationAngle), vec3(0.0f, 1.0f, 0.0f))
        * scale(vec3(moon.size));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, moon.textureID);
    glUniform1i(ph_colorMapLoc, 0);
    DrawSphere(moonModel);
}

int main()
{
    sf::ContextSettings context;
    context.depthBits = 24;
    sf::Window window(sf::VideoMode(WIDTH, HEIGHT), "3D Solar System", sf::Style::Close, context);

    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);

    if (Init()) return 1;

    sf::Mouse::setPosition(sf::Vector2i(WIDTH / 2, HEIGHT / 2), window);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                if (event.key.code == sf::Keyboard::Escape) window.close();
                if (event.key.code == sf::Keyboard::O)
                {
                    useOrthographic = !useOrthographic;
                    cout << "Projection: " << (useOrthographic ? "Orthographic" : "Perspective") << endl;
                }
                break;
            case sf::Event::MouseMoved:
            {
                float xpos = (float)event.mouseMove.x;
                float ypos = (float)event.mouseMove.y;

                if (firstMouse)
                {
                    lastMouseX = xpos;
                    lastMouseY = ypos;
                    firstMouse = false;
                }

                float xoffset = xpos - lastMouseX;
                float yoffset = lastMouseY - ypos;

                if (xoffset == 0 && yoffset == 0) break; // Skip if no actual movement

                lastMouseX = xpos;
                lastMouseY = ypos;

                xoffset *= mouseSensitivity;
                yoffset *= mouseSensitivity;

                cameraYaw += xoffset;
                cameraPitch += yoffset;

                if (cameraPitch > 89.0f) cameraPitch = 89.0f;
                if (cameraPitch < -89.0f) cameraPitch = -89.0f;

                vec3 direction;
                direction.x = glm::cos(glm::radians(cameraYaw)) * glm::cos(glm::radians(cameraPitch));
                direction.y = glm::sin(glm::radians(cameraPitch));
                direction.z = glm::sin(glm::radians(cameraYaw)) * glm::cos(glm::radians(cameraPitch));
                cameraFront = glm::normalize(direction);

                sf::Mouse::setPosition(sf::Vector2i(WIDTH / 2, HEIGHT / 2), window);
                lastMouseX = WIDTH / 2.0f;
                lastMouseY = HEIGHT / 2.0f;
                break;
            }
            }
        }

        Update();
        Render();
        window.display();
    }
    return 0;
}