#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/OpenGL.hpp>

#include <atomic>
#include <condition_variable>
#include <algorithm>
#include <deque>
#include <functional>
#include <memory>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>

#include <sys/stat.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <CoreGraphics/CGDirectDisplay.h>
#endif

#include <bristol/sfml/sfml_helpers.hpp>
#include <bristol/sfml/window_event_manager.hpp>
#include <bristol/sfml/virtual_window_manager.hpp>
#include <bristol/sfml/virtual_window.hpp>
#include <bristol/flags.hpp>
#include <bristol/utils.hpp>
#include <bristol/string_utils.hpp>
#include <bristol/file_watcher.hpp>

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

  using std::atomic;
  using std::condition_variable;
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
  using std::thread;

  using std::function;

  using std::min;
  using std::max;

  using sf::Color;
  using sf::Font;
  using sf::Text;
  using sf::Image;
  using sf::Texture;
  using sf::Sprite;
  using sf::CircleShape;
  using sf::RectangleShape;
  using sf::RenderTarget;
  using sf::RenderWindow;
  using sf::RenderTexture;
  using sf::Keyboard;
  using sf::Event;
  using sf::View;
  using sf::Time;
  using sf::Clock;
  using sf::VertexArray;
  using sf::Vertex;

  using sf::Vector2f;
  using sf::Vector3f;
  using sf::Vector2i;
  using sf::Vector2u;
  using sf::IntRect;
  using sf::FloatRect;
  using sf::Transform;

  using sf::IpAddress;
  using sf::Socket;
  using sf::TcpListener;
  using sf::TcpSocket;

  using bristol::WindowEventManager;
  using bristol::VirtualWindowManager;
  using bristol::VirtualWindow;
  using bristol::Flags;
  using bristol::FileWatcher;

  using bristol::exch_null;

}

#include "imgui/stb_image.h"                  // for .png loading
#include "imgui/imgui.h"
