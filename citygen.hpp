#pragma once

#include "glm/glm.hpp"
#include "glm/core/type_vec3.hpp"

namespace citygen
{
  using std::vector;
  using std::map;
  using std::string;

  using glm::vec2;
  using glm::vec3;
  using glm::vec4;
  using glm::mat4;
  using glm::dot;
}

#pragma once

namespace bristol
{
  class WindowEventManager;
  class VirtualWindowManager;
}

namespace citygen
{
  class Arcball;

  struct Tri
  {
    vec3 v0, v1, v2;
  };

  struct Terrain
  {
    Terrain();
    void CreateMesh();
    void CalcIntersection(const vec3& org, const vec3& dir);
    vector<vec3> verts;
    vector<Tri> tris;
    vector<u32> indices;
    vector<Tri> intersected;
    u8* data;
    int w, h;
    int depth;
    float scale;
    float heightScale;
  };

  class CityGen
  {
  public:

    struct StateFlagsF
    {
      enum Enum { Done = 1 << 0, Paused = 1 << 1, };
      struct Bits { u32 done : 1; u32 paused : 1; };
    };

    typedef Flags<StateFlagsF> StateFlags;

    static void Create();
    static void Destroy();
    static CityGen& Instance();

    bool Init();
    bool Run();
    bool Close();

    CityGen();
    ~CityGen();

    void Update();
    void Render();

    static CityGen* _instance;
    string _appRoot;
    //ptime _lastUpdate;
    //time_duration _curTime;
    //time_duration _fileWatchAcc;
    StateFlags _stateFlags;

    Terrain _terrain;
    FileWatcher _fileWatcher;
    Arcball* _arcball;
  };

#define CITYGEN CityGen::Instance()
}