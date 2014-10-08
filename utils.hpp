#pragma once

namespace citygen
{
  namespace protocol
  {
    class Vector3;
  }

//----------------------------------------------------------------------------------
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

  //----------------------------------------------------------------------------------
  vec3 FromProtocol(const protocol::Vector3& v);
  void ToProtocol(const vec3& v, protocol::Vector3* out);

  //----------------------------------------------------------------------------------
  struct BitSet
  {
    BitSet(u32 bits)
      : len(bits/8+1)
    {
      mem.resize(len);
    }

    bool IsSet(u32 ofs) const
    {
      assert(ofs/8 < len);

      u32 byteOfs = ofs / 8;
      u32 shift   = ofs % 8;
      u8 mask     = (1 << shift);

      return !!(mem[byteOfs] & mask);
    }

    void Set(u32 ofs)
    {
      assert(ofs/8 < len);

      u32 byteOfs = ofs / 8;
      u32 shift   = ofs % 8;
      u8 mask     = (1 << shift);
      mem[byteOfs] |= mask;
    }

    void Clear(u32 ofs)
    {
      u32 byteOfs = ofs / 8;
      u32 shift   = ofs % 8;
      u8 mask     = (1 << shift);
      mem[byteOfs] &= ~mask;
    }

    bool operator[](u32 ofs) const
    {
      return IsSet(ofs);
    }

    u32 len;
    vector<u8> mem;
  };

  //----------------------------------------------------------------------------------
  template <typename T> typename T::value_type FrontPop(T& cont)
  {
    typename T::value_type tmp = cont.front();
    cont.pop_front();
    return tmp;
  }

}