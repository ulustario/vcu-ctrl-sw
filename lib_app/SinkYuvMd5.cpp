// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <fstream>
#include "SinkYuvMd5.hpp"
#include "lib_app/MD5.hpp"
#include "lib_app/YuvIO.hpp"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
}

using namespace std;

class YuvMd5Calculator : public IFrameSink
{
public:
  YuvMd5Calculator(std::string& path)
  {
    if(path == "stdout")
    {
      m_pOut = &std::cout;
    }
    else if(!path.empty())
    {
      m_FileOut.open(path);
      m_pOut = &m_FileOut;
    }
  }

  ~YuvMd5Calculator(void)
  {
    if(m_pOut && m_pOut->good())
    {
      *m_pOut << m_Md5Yuv.GetMD5() << std::endl;

      if(m_bHasMap)
        *m_pOut << m_Md5Map.GetMD5() << std::endl;
    }
  }

  void ProcessFrame(AL_TBuffer* pBuf) override
  {
    ComputeMd5SumFrame(pBuf, m_Md5Yuv);

    if(AL_IsCompressed(AL_PixMapBuffer_GetFourCC(pBuf)))
    {
      m_bHasMap = true;
      ComputeMd5SumMap(pBuf, m_Md5Map);
    }
  }

private:
  std::ofstream m_FileOut;
  std::ostream* m_pOut;
  CMD5 m_Md5Yuv;
  CMD5 m_Md5Map;
  bool m_bHasMap = false;
};

IFrameSink* createYuvMd5Calculator(std::string path)
{
  return new YuvMd5Calculator(path);
}
