#include "file_utils.hpp"
#include <direct.h>
#include <sys/stat.h>
#include <io.h>

namespace citygen
{
  //---------------------------------------------------------------------------
  string CurrentDirectory()
  {
    char buf[256];
    _getcwd(buf, sizeof(buf));
    return string(buf);
  }

  //---------------------------------------------------------------------------
  bool FileExists(const char* filename)
  {
    struct _stat status;
    return _access(filename, 0) == 0 && _stat(filename, &status) == 0 && (status.st_mode & _S_IFREG);
  }

  //---------------------------------------------------------------------------
  bool DirectoryExists(const char* name)
  {
    struct _stat status;
    return _access(name, 0) == 0 && _stat(name, &status) == 0 && (status.st_mode & _S_IFDIR);
  }

  //---------------------------------------------------------------------------
  Error LoadTextFile(const char* filename, vector<char>* buf)
  {
    Error err = LoadFile(filename, buf);
    buf->push_back(0);
    return err;
  }

}
