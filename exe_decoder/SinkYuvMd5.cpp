// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <fstream>
#include "SinkYuvMd5.h"
#include "lib_app/MD5.h"
#include "lib_app/YuvIO.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
}

using namespace std;

class YuvMd5Calculator : public IFrameSink, Md5Calculator
{
public:
  YuvMd5Calculator(std::string& path) :
    Md5Calculator(path)
  {}

  ~YuvMd5Calculator()
  {
    if(IsFileOpen())
    {
      Md5Output();
    }
  }

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    ComputeMd5SumFrame(pBuf, GetCMD5());
  }
};

std::unique_ptr<IFrameSink> createYuvMd5Calculator(std::string path)
{
  return std::unique_ptr<IFrameSink>(new YuvMd5Calculator(path));
}
