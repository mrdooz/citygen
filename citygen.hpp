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

  struct Terrain
  {
    Terrain();
    void CreateMesh();
    vector<vec3> verts;
    vector<u32> indices;
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
  };

#define CITYGEN CityGen::Instance()
}