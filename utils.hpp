#pragma once

namespace citygen
{
  template <typename T>
  bool LoadProto(const char* filename, T* out, bool textFormat = true)
  {
#pragma warning(suppress: 4996)
    FILE* f = fopen(filename, "rb");
    if (!f)
      return false;

    fseek(f, 0, 2);
    size_t s = ftell(f);
    fseek(f, 0, 0);
    std::string str;
    str.resize(s);
    fread((char*)str.c_str(), 1, s, f);
    fclose(f);

    return textFormat
      ? google::protobuf::TextFormat::ParseFromString(str, out)
      : out->ParseFromString(str);
  }

}