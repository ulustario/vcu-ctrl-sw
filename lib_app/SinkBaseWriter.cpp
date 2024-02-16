// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/SinkBaseWriter.h"
#include "lib_app/CompFrameCommon.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
}

#include <iostream>
#include <algorithm>

using namespace std;

/****************************************************************************/
const std::string BaseFrameSink::ErrorMessageWrite = "Error writing in compressed YUV file.";
const std::string BaseFrameSink::ErrorMessageBuffer = "Null buffer provided.";
const std::string BaseFrameSink::ErrorMessagePitch = "U and V plane pitches must be identical.";

/****************************************************************************/
void BaseFrameSink::FactorsCalculus()
{
  if(m_tPicFormat.ePlaneMode == AL_PLANE_MODE_INTERLEAVED)
    m_iNbBytesPerPix = (m_tPicFormat.uBitDepth == 8 || (m_tPicFormat.uBitDepth == 10 && m_tPicFormat.eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED)) ? sizeof(uint32_t) : sizeof(uint64_t);
  else
    m_iNbBytesPerPix = m_tPicFormat.uBitDepth > 8 ? sizeof(uint16_t) : sizeof(uint8_t);

  m_iChromaVertScale = m_tPicFormat.eChromaMode == AL_CHROMA_4_2_0 ? 2 : 1;
  m_iChromaHorzScale = m_tPicFormat.eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;
}

/****************************************************************************/
void BaseFrameSink::DimInTileCalculus()
{
  static const uint32_t MIN_HEIGHT_ROUNDING = 8;

  int iTileWidth = GetTileWidth(m_tPicFormat.eStorageMode, m_tPicFormat.uBitDepth);
  int iTileHeight = GetTileHeight(m_tPicFormat.eStorageMode);

  FactorsCalculus();

  {
    m_uPitchYFile = AL_RoundUpAndMul(m_tPicDim.iWidth, iTileWidth, iTileHeight) * m_tPicFormat.uBitDepth >> 3;
  }

  m_uPitchCFile = m_uPitchYFile;
  m_uHeightInTileYFile = AL_RoundUpAndDivide(m_tPicDim.iHeight, std::max(uint32_t(iTileHeight), MIN_HEIGHT_ROUNDING), iTileHeight);

  if(m_tPicFormat.ePlaneMode == AL_PLANE_MODE_SEMIPLANAR)
    m_uHeightInTileCFile = AL_RoundUp(m_uHeightInTileYFile, m_iChromaVertScale) / m_iChromaVertScale;
  else
  {
    m_iChromaVertScale = 0;
    m_uHeightInTileCFile = 0;
  }
}

/****************************************************************************/
void BaseFrameSink::WritePix(const uint8_t* pPix, uint32_t iPitchInPix, uint16_t uHeightInTile, uint32_t uPitchFile)
{
  CheckNotNull(pPix);

  for(int r = 0; r < uHeightInTile; ++r)
  {
    WriteBuffer(m_recFile, pPix, uPitchFile);
    pPix += iPitchInPix;
  }
}

/****************************************************************************/
void BaseFrameSink::WriteBuffer(std::shared_ptr<std::ostream> stream, const uint8_t* pBuf, uint32_t uWriteSize)
{
  stream->write(reinterpret_cast<const char*>(pBuf), uWriteSize);
}

/****************************************************************************/
void BaseFrameSink::CheckNotNull(const uint8_t* pBuf)
{
  if(nullptr == pBuf)
    throw std::runtime_error(ErrorMessageBuffer);
}

/****************************************************************************/
BaseFrameSink::BaseFrameSink(std::shared_ptr<std::ostream> recFile, AL_EFbStorageMode eStorageMode, AL_EOutputType outputID) :
  m_recFile(recFile), m_eStorageMode(eStorageMode), m_iOutputID(outputID)
{
}

/****************************************************************************/
BaseFrameSink::~BaseFrameSink()
{
  m_recFile->flush();
}
