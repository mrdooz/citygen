#include "citygen.hpp"

#include <stdio.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <GL/GLU.h>
#else
#include <OpenGL/glu.h>
#include <unistd.h>
#endif


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

  GLuint sphereVbo;
  float sphereRadius = 100;
  vec3 sphereCenter(-20,-100,0);

  //---------------------------------------------------------------------------------
  //
  vec4 Unproject(const vec4& v, const mat4& view, const mat4& proj)
  {
    mat4 inv = glm::inverse(proj * view);
    return inv * v;
  }

  //---------------------------------------------------------------------------------
  //
  static void RenderFrame()
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

    float nearPlane = 2;
    float farPlane = 10000;

    vec3 pos(xPos, yPos, zPos);
    mat4 view = glm::lookAt(pos, vec3(0, 0, 0), vec3(0, 1, 0));
    mat4 proj = glm::perspective(45.0f, 4.0f / 3.0f, nearPlane, farPlane);

    vec4 r = proj * vec4(0,0,-nearPlane, 1);

    vec2 vecNdc((2.0f * mouseX) / width - 1, 1.0f - (2.0f * mouseY) / height);
    vec4 vecClip(vecNdc.x, vecNdc.y, -1, 1);

    // multiply by the perspective divide
    vecClip *= nearPlane;

    vec4 nearPos = Unproject(vecClip, view, proj);
    vec3 np(nearPos.x, nearPos.y, nearPos.z);
    vec3 d = glm::normalize(np - pos);

    // check if the ray intersects the sphere
    float a = dot(d,d);
    float b = 2 * dot(pos - sphereCenter, d);
    float c = dot(pos - sphereCenter, pos - sphereCenter) - sphereRadius * sphereRadius;

    bool hit = false;
    float t1=0, t2=0;
    float discriminant = sqrtf(b*b - 4*a*c);
    if (discriminant > 0)
    {
      hit = true;
      t1 = (-b - discriminant) / (2*a);
      t2 = (-b + discriminant) / (2*a);
    }

    sprintf(buf, "dir: %f, %f, %f. t: %f, hit: %s", d.x, d.y, d.z, min(t1,t2), hit ? "Y" : "N");
    imguiLabel(buf);

    imguiEndScrollArea();
    imguiEndFrame();

    // render stuff
    glUseProgram(shaderProgram);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));

    glPointSize(2);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, heightVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_POINTS, 0, 256*256);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_POINTS, 0, 10000);

    glLineWidth(1);

    glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    if (vec3* ptr = (vec3*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY))
    {
      ptr[0] = np;
      ptr[1] + np + 10.0f * d;
      glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    glDrawArrays(GL_LINES, 0, 2);

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

  //---------------------------------------------------------------------------------
  //
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

      chdir("..");

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

  //---------------------------------------------------------------------------------
  //
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
      printf("%s", buf.data());
    }

    return shader;
  }

  //---------------------------------------------------------------------------------
  //
  Error InitShaders()
  {
    shaderProgram   = glCreateProgram();
    vertexShader    = loadShader("simple.vert.glsl", GL_VERTEX_SHADER);
    fragmentShader  = loadShader("simple.frag.glsl", GL_FRAGMENT_SHADER);

    if (vertexShader == -1 || fragmentShader == -1)
    {
      return Error::INVALID_SHADER;
    }

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);
    return Error::OK;
  }

  //---------------------------------------------------------------------------------
  //
  void GenerateSphere()
  {
    vector<vec3> verts;

    while (verts.size() < 10000)
    {
      float x = -1 + 2 * (rand() / (float)RAND_MAX);
      float y = -1 + 2 * (rand() / (float)RAND_MAX);
      float z = -1 + 2 * (rand() / (float)RAND_MAX);

      vec3 p(x,y,z);
      p *= sphereRadius;

      if (glm::length(p) < sphereRadius)
      {
        verts.push_back(p + sphereCenter);
      }
    }

    glGenBuffers(1, &sphereVbo);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vec3), verts.data(), GL_STATIC_DRAW);
  }

  //---------------------------------------------------------------------------------
  //
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

  Error err = InitShaders();
  if (err != Error::OK)
  {
    return (int)err;
  }

  if (CreateHeightField() != Error::OK)
    return 1;

  GenerateSphere();

  glGenBuffers(1, &lineVbo);
  glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
  glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

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
    RenderFrame();
  }

  SDL_GL_DeleteContext(glcontext);
  SDL_Quit();

  return 0;
}
