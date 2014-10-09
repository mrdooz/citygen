#pragma once

#include "terrain.hpp"
#include "graph.hpp"

namespace citygen
{
  namespace protocol
  {
    class StepSettings;
    class SecondaryParameterSet;
  }

  class Arcball;

  struct SecondaryParameterSet
  {
    void ToProtocol(protocol::SecondaryParameterSet* proto) const;
    void FromProtocol(const protocol::SecondaryParameterSet& proto);

    int cellId;
    float segmentSize = 10;
    float segmentSizeDeviation = 0.5f;
    int degree = 2;
    float degreeDeviation = 0.5f;
    float snapSize = 20;
    float snapSizeDeviation = 0.5f;
    float connectivity = 0.3f;
  };

  struct StepSettings
  {
    void ToProtocol(protocol::StepSettings* proto) const;
    void FromProtocol(const protocol::StepSettings& proto);

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

    struct ClickFlagsF
    {
      enum Enum { FirstClick = 1 << 0, Dragging = 1 << 1, };
      struct Bits { u32 firstClick : 1; u32 dragging : 1; };
    };
    typedef Flags<ClickFlagsF> ClickFlags;

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
    void CalcCells();

    void CalcSecondary(const Cycle& cycle);

    static CityGen* _instance;
    string _appRoot;
    //ptime _lastUpdate;
    //time_duration _curTime;
    //time_duration _fileWatchAcc;
    StateFlags _stateFlags;

    Terrain _terrain;
    FileWatcher _fileWatcher;
    Arcball* _arcball;

    vector<vec3> _nodes;
    vector<vec3> _primary;

    bool _drawDebugLines;
    vector<vec3> _debugLines;

    bool _drawNormals;

    glm::mat4x4 _rot;

    StepSettings _stepSettings;
    vector<SecondaryParameterSet> _parameterSet;
    State _state;

    Graph _graph;

    string _configFile;
    ClickFlags _clickFlags;
    vec3 _dragStart;
  };

#define CITYGEN CityGen::Instance()
}