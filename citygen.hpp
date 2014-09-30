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

    bool OnLostFocus(const Event& event);
    bool OnGainedFocus(const Event& event);
    bool OnKeyPressed(const Event& event);
    bool OnKeyReleased(const Event& event);
    bool OnTextEntered(const Event& event);

    bool OnMouseButtonReleased(const Event& event);

    RenderWindow* _renderWindow;
    WindowEventManager* _eventManager;
    VirtualWindowManager* _virtualWindowManager;

    static CityGen* _instance;
    string _appRoot;
    //ptime _lastUpdate;
    //time_duration _curTime;
    //time_duration _fileWatchAcc;
    StateFlags _stateFlags;

    FileWatcher _fileWatcher;
  };

#define CITYGEN CityGen::Instance()
}