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
    void CreateMesh();
    void CalcIntersection(const vec3& org, const vec3& dir);
    Tri* FindTri(const vec3& v, vec3* out);
    vector<vec3> _verts;
    vector<Tri> _tris;
    vector<u32> _indices;
    vector<vec3> _intersected;
    vec3 _minValues, _maxValues;
    vec3 _intersection;
    u8* _data = nullptr;
    int _w, _h, _depth;
    float _scale = 20.f;
    float _heightScale = 1.5f;
  };

  struct StepSettings
  {
    int numSegments = 20;
    float stepSize = 0.5f;
    float deviation = DEG_TO_RAD(45);
    float roadHeight = 5;
  };

  struct State
  {

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

    void LoadSettings(const char* filename);
    void SaveSettings(const char* filename);

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

    glm::mat4x4 _rot;

    StepSettings _stepSettings;
    State _state;

    string _configFile;
  };

#define CITYGEN CityGen::Instance()
}