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
    vec3 n;
  };

  struct Terrain
  {
    Terrain();
    void CreateMesh();
    void CalcIntersection(const vec3& org, const vec3& dir);
    Tri* FindTri(const vec3& v);
    vector<vec3> verts;
    vector<Tri> tris;
    vector<u32> indices;
    vector<vec3> intersected;
    vec3 minValues, maxValues;
    vec3 intersection;
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

    void AddPoint(const vec3& pt);

    CityGen();
    ~CityGen();

    void Update();
    void Render();

    void GeneratePrimary();
    void RenderUI();

    static CityGen* _instance;
    string _appRoot;
    //ptime _lastUpdate;
    //time_duration _curTime;
    //time_duration _fileWatchAcc;
    StateFlags _stateFlags;

    Terrain _terrain;
    FileWatcher _fileWatcher;
    Arcball* _arcball;

    vector<vec3> _points;
    vector<vec3> _primary;

    bool _drawDebugLines;
    vector<vec3> _debugLines;

    bool _drawNormals;

    int _numSegments = 20;
    float _sampleSize = 3;
    float _deviation = DEG_TO_RAD(45);

    glm::mat4x4 _rot;
  };

#define CITYGEN CityGen::Instance()
}