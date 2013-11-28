#include "citygen.hpp"

#include <stdio.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <windows.h>
#include <direct.h>
#include <gl/glew.h>
#include <gl/GLU.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imguiRenderGL.h"
#include "file_utils.hpp"

#ifdef _WIN32
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "sdl2.lib")
#pragma comment(lib, "glew32.lib")
#endif

namespace citygen
{
  SDL_Window* displayWindow;

  static int mouseX = 0, mouseY = 0, mouseButtons = 0;
  static int width, height;

  GLuint shaderProgram, vertexShader, fragmentShader;
  GLuint heightVbo;

  GLuint lineVbo;

  static void renderFrame()
  {
    glClearColor(0 / 255.0f, 0x2b / 255.0f, 0x36 / 255.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // ---- UI

    imguiBeginFrame(mouseX, height - 1 - mouseY, mouseButtons, 0);

    static const int paramSize = 200;
    static int paramScroll = 0;
    imguiBeginScrollArea("Params", 0, height - paramSize, width, paramSize, &paramScroll);

    static float xPos = 0.0f;
    imguiSlider("xPos", &xPos, -1000, 1000.0f, 1);

    static float yPos = 20.0f;
    imguiSlider("yPos", &yPos, -1000, 1000.0f, 1);

    static float zPos = 1000.0f;
    imguiSlider("zPos", &zPos, 1, 1000.0f, 1);

    char buf[256];
    sprintf(buf, "mouse: %d, %d", mouseX, mouseY);
    imguiLabel(buf);

    vec3 pos(xPos, yPos, zPos);
    mat4 view = glm::lookAt(pos, vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 invView = glm::inverse(view);

    mat4 proj = glm::perspective(45.0f, 4.0f / 3.0f, 1.0f, 10000.f);
    mat4 invProj = glm::inverse(proj);

    vec3 vecNdc((2.0f * mouseX) / width - 1, 1.0f - (2.0f * mouseY) / height, 1.0f);
    vec4 vecClip(vecNdc.x, vecNdc.y, -1, 1);
    vec4 vecEye = vecClip * invProj;
    vecEye.z = -1;
    vecEye.w = 1;

    vec4 vecWorld = vecEye * invView;
    vec3 rayDir(glm::normalize(vec3(vecWorld.x, vecWorld.y, vecWorld.z)));

    // check if the ray intersects the plane (0,1,0,0)
    vec3 n(0,1,0);
    float t = glm::dot(pos, n) / glm::dot(rayDir, n);

    sprintf(buf, "dir: %f, %f, %f. t: %f", rayDir.x, rayDir.y, rayDir.z, t);
    imguiLabel(buf);

    imguiEndScrollArea();
    imguiEndFrame();

    // render stuff
    glUseProgram(shaderProgram);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, heightVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));

    glPointSize(2);
    glDrawArrays(GL_POINTS, 0, 256*256);

    vec3 p = pos + 10000.0f * rayDir;
    GLfloat verts[] = { pos.x, pos.y, pos.z, p.x, p.y, p.z };

    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
    if (vec3* ptr = (vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY))
    {
      ptr[0] = pos;
      ptr[1] = p;
      glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    glDrawArrays(GL_LINE, 0, 2);

    GLenum err = glGetError();
    if (err != 0)
    {
      printf("%s\n", gluErrorString(err));
    }

    glUseProgram(0);


    // back to GUI
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1.0f, 1.0f);

    imguiRenderGLDraw();

    glDisable(GL_BLEND);

    SDL_GL_SwapWindow(displayWindow);
  }

  bool Init()
  {
    // find the app.root
    string cur = CurrentDirectory();
    string prev = cur;
    while (true)
    {
      if (FileExists("app.root"))
      {
        break;
      }

      _chdir("..");

      // break if at the root
      string tmp = CurrentDirectory();
      if (tmp == prev)
      {
        break;
      }
      prev = tmp;
    }

    return true;
  }

  GLuint loadShader(const char* filename, GLuint type)
  {
    GLuint shader = glCreateShader(type);
    vector<char> buf;
    if (LoadTextFile(filename, &buf) != Error::OK)
    {
      return -1;
    }

    const char* data = buf.data();
    glShaderSource(shader, 1, &data, NULL);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
      GLint length;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
      vector<char> buf(length);
      glGetShaderInfoLog(shader, length, &length, buf.data());
      printf(buf.data());
    }

    return shader;
  }

  bool initShaders()
  {
    shaderProgram   = glCreateProgram();
    vertexShader    = loadShader("simple.vert.glsl", GL_VERTEX_SHADER);
    fragmentShader  = loadShader("simple.frag.glsl", GL_FRAGMENT_SHADER);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);
    return true;
  }

  Error CreateHeightField()
  {
    vector<u8> buf;
    Error err = LoadFile("noise.tga", &buf);
    if (err != Error::OK)
      return err;

    vector<vec3> verts;
    int width = 256;
    int height = 256;

    u8* data = &buf[18];

    float gridSpacing = 10;
    float zPos = -(width/2 - 0.5f) * gridSpacing;

    // store height info
    for (int y = 0; y < height; ++y)
    {
      float xPos = -(width/2 - 0.5f) * gridSpacing;
      for (int x = 0; x < width; ++x)
      {
        float h = data[(x+y*width)*4];
        verts.push_back(vec3(xPos, h, zPos));
        xPos += gridSpacing;
      }
      zPos += gridSpacing;
    }

    glGenBuffers(1, &heightVbo);
    glBindBuffer(GL_ARRAY_BUFFER, heightVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vec3), verts.data(), GL_STATIC_DRAW);

    return Error::OK;
  }
}


int main(int argc, char** argv)
{
  using namespace citygen;
  Init();
  width = 1024;
  height = 768;
  SDL_SetMainReady();
  SDL_Init(SDL_INIT_VIDEO);
  displayWindow = SDL_CreateWindow(
    "SDL2/OpenGL Demo", 100, 100, width, height, 
    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  SDL_GLContext glcontext = SDL_GL_CreateContext(displayWindow);
  glViewport(0, 0, width, height);

  GLuint res = glewInit();
  if (res != GLEW_OK)
  {
    return 1;
  }

  imguiRenderGLInit("calibri.ttf");

  if (!initShaders())
  {
    return 1;
  }

  if (CreateHeightField() != Error::OK)
    return 1;

  glGenBuffers(1, &lineVbo);

  bool quit = false;
  while (!quit)
  {
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        quit = true;
        break;

      case SDL_MOUSEMOTION:
        mouseX = event.motion.x;
        mouseY = event.motion.y;
        break;

      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
          mouseButtons |= 1;
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
          mouseButtons |= 2;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
          mouseButtons &= ~1;
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
          mouseButtons &= ~2;
        }
        break;
      }
    }
    renderFrame();
  }

  SDL_GL_DeleteContext(glcontext);
  SDL_Quit();

  return 0;
}
