// glew & glfw
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#ifdef _MSC_VER
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <atomic>
#include <condition_variable>
#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <ostream>
#include <queue>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>

#include <sys/stat.h>
#include <stdio.h>

#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/core/type_vec3.hpp"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <CoreGraphics/CGDirectDisplay.h>
#endif

#include <bristol/flags.hpp>
#include <bristol/utils.hpp>
#include <bristol/string_utils.hpp>
#include <bristol/file_watcher.hpp>

#define DEG_TO_RAD(x) ((x)/180.f*3.1415926f)
#define RAD_TO_DEG(x) ((x)/3.1415926f*180)

#pragma warning(push)
#pragma warning(disable: 4244 4267)
#include <google/protobuf/text_format.h>
#pragma warning(pop)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

namespace citygen
{
  extern const float PI;

  using std::count_if;
  using std::bind;
  using std::function;
  using std::placeholders::_1;
  using std::placeholders::_2;
  using std::placeholders::_3;
  using std::vector;
  using std::string;
  using std::set;
  using std::deque;
  using std::priority_queue;
  using std::map;
  using std::unordered_multimap;
  using std::unordered_map;
  using std::unordered_set;
  using std::pair;
  using std::make_pair;
  using std::make_shared;
  using std::shared_ptr;
  using std::unique_ptr;
  using std::swap;
  using std::remove_if;

  using std::function;

  using std::min;
  using std::max;

  using bristol::Flags;
  using bristol::FileWatcher;

  using bristol::exch_null;
  using bristol::lerp;

  using glm::vec2;
  using glm::vec3;
  using glm::vec4;
  using glm::mat4;
  using glm::dot;
  using glm::cross;
  using glm::normalize;
  using glm::distance;
  using glm::length;
}

#include "imgui/stb_image.h"                  // for .png loading
#include "imgui/imgui.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)         // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#include <Windows.h>
#include <Imm.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "imm32.lib")
#if _DEBUG
#pragma comment(lib, "/projects/glfw/src/debug/glfw3.lib")
#else
#pragma comment(lib, "/projects/glfw/src/release/glfw3.lib")
#endif
#endif

#include "imgui/imgui.h"

#include "arcball.hpp"
#include "glm/gtx/intersect.hpp"
