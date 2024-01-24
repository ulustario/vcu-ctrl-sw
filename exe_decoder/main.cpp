// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <climits>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include "lib_common/BufCommon.h"
#include "lib_common/BufferHandleMeta.h"
#include "lib_common/BufferSeiMeta.h"
#include "lib_common/DisplayInfoMeta.h"
#include "lib_common/Error.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/IpDecFourCC.h"
#include "lib_decode/lib_decode.h"
#include "lib_common_dec/HDRMeta.h"
#include "lib_common/BufferPictureDecMeta.h"
}
#include "lib_app/BufPool.h"
#include "lib_app/MD5.h"
#include "lib_app/PixMapBufPool.h"
#include "lib_app/SinkCrcDump.h"
#include "lib_app/UnCompFrameReader.h"
#include "lib_app/UnCompFrameWriter.h"
#include "lib_app/YuvIO.h"
#include "lib_app/console.h"
#include "lib_app/convert.h"
#include "lib_app/plateform.h"
#include "lib_app/timing.h"
#include "lib_app/utils.h"

#include <cassert>

#include "CmdParser.h"
#include "CodecUtils.h"
#include "Conversion.h"
#include "InputLoader.h"
#include "IpDevice.h"
#include "SinkYuvCrc.h"
#include "SinkYuvMd5.h"
#include "HDRWriter.h"
#include "lib_conv_yuv/lib_conv_yuv.h"

using namespace std;

/******************************************************************************/
enum DeviceType
{
  DEVICE_BASE_DECODER,
};

using Devices = map<DeviceType, shared_ptr<I_IpDevice>>;
using UseBoards = map<DeviceType, bool>;

struct codec_error : public runtime_error
{
  explicit codec_error(AL_ERR eErrCode) : runtime_error(AL_Codec_ErrorToString(eErrCode)), Code(eErrCode)
  {
  }

  const AL_ERR Code;
};

/******************************************************************************/
static void ConvertFrameBuffer(AL_TBuffer* pInput, AL_TBuffer*& pOutput, int iBdOut, AL_TPosition const& tPos, TFourCC tOutFourCC)
{
  (void)tPos;
  TFourCC tRecFourCC = AL_PixMapBuffer_GetFourCC(pInput);
  AL_TDimension tRecDim = AL_PixMapBuffer_GetDimension(pInput);
  AL_EChromaMode eRecChromaMode = AL_GetChromaMode(tRecFourCC);

  TFourCC tConvFourCC = tOutFourCC;
  AL_TPicFormat tConvPicFormat;
  assert(tConvFourCC);

  if(pOutput != nullptr)
  {
    AL_TDimension tYuvDim = AL_PixMapBuffer_GetDimension(pOutput);

    AL_GetPicFormat(tConvFourCC, &tConvPicFormat);

    if(tRecDim.iWidth != tYuvDim.iWidth || tRecDim.iHeight != tYuvDim.iHeight ||
       eRecChromaMode != tConvPicFormat.eChromaMode || iBdOut != tConvPicFormat.uBitDepth)
    {
      AL_Buffer_Destroy(pOutput);
      pOutput = nullptr;
    }
  }

  AL_PixMapBuffer_SetDimension(pInput, { tPos.iX + tRecDim.iWidth, tPos.iY + tRecDim.iHeight });

  if(pOutput == nullptr)
  {
    AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pInput);

    pOutput = AllocateDefaultYuvIOBuffer(tDim, tConvFourCC);

    if(pOutput == nullptr)
      throw runtime_error("Couldn't allocate YuvBuffer");
  }

  if(ConvertPixMapBuffer(pInput, pOutput))
    throw runtime_error("Couldn't convert buffer");

  AL_PixMapBuffer_SetDimension(pInput, tRecDim);
  AL_PixMapBuffer_SetDimension(pOutput, tRecDim);
}

/******************************************************************************/
static bool IsEndOfStream(AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo)
{
  return !pFrame && !pInfo;
}

/******************************************************************************/
static bool IsReleaseFrame(AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo)
{
  return pFrame && !pInfo;
}

/******************************************************************************/
class DisplayManager
{
public:
  void Configure(Config const& config);
  bool Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo, int iBitDepthAlloc, bool& bIsMainDisplay, bool& bNumFrameReached, bool bDecoderExists);

private:
  void ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut, TFourCC tFourCCOut);

  void CopyMetaData(AL_TBuffer* pDstFrame, AL_TBuffer* pSrcFrame, AL_EMetaType eMetaType);

  unique_ptr<MultiSink> multisink = unique_ptr<MultiSink>(new MultiSink);

  AL_EFbStorageMode eMainOutputStorageMode;
  int iBitDepth = 8;
  TFourCC tOutputFourCC = FOURCC(NULL);
  unsigned int NumFrames = 0;
  unsigned int MaxFrames = UINT_MAX;
  unsigned int FirstFrame = 0;

  int iNumFrameConceal = 0;
  bool bHasOutput = false;
  std::shared_ptr<HDRWriter> pHDRWriter;
};

/******************************************************************************/
void DisplayManager::Configure(Config const& config)
{
  bool bMainOutputCompression;

  if(config.tOutputFourCC != FOURCC(NULL))
    eMainOutputStorageMode = AL_GetStorageMode(config.tOutputFourCC);
  else
  {
    eMainOutputStorageMode = GetMainOutputStorageMode(config.tDecSettings, bMainOutputCompression, 8);

    if(!IsRaster(eMainOutputStorageMode) && !bMainOutputCompression)
      eMainOutputStorageMode = AL_FB_RASTER;
  }

  bHasOutput = (config.bEnableYUVOutput || config.bCertCRC || !config.sCrc.empty() || !config.md5File.empty());

  if(bHasOutput)
  {
    if(config.bEnableYUVOutput)
    {
      std::shared_ptr<ofstream> hFileOut(new ofstream(config.sMainOut, ios::binary));

      if(!hFileOut->is_open())
        throw runtime_error("Invalid output file");

      std::shared_ptr<ofstream> hMapOut;

      if(bMainOutputCompression
         )
      {
        hMapOut.reset(new ofstream(config.sMainOut + ".map", ios::binary));

        if(!hMapOut->is_open())
          throw runtime_error("Invalid output map file");
      }

      {
        if(!bMainOutputCompression)
        {
          std::unique_ptr<IFrameSink> sink_main = std::unique_ptr<UnCompFrameWriter>(new UnCompFrameWriter(hFileOut, eMainOutputStorageMode, AL_OUTPUT_MAIN));
          multisink->addSink(sink_main);
        }
      }
    }

    if(!config.md5File.empty() && !bMainOutputCompression)
    {
      std::unique_ptr<IFrameSink> md5Calculator = createYuvMd5Calculator(config.md5File);
      multisink->addSink(md5Calculator);
    }

    std::unique_ptr<IFrameSink> crcDump = createStreamCrcDump(config.sCrc);
    multisink->addSink(crcDump);

    if(config.bCertCRC)
    {
      const string sCertCrcFile = "crc_certif_res.hex";
      std::unique_ptr<IFrameSink> crcCSCalculator = createCSCrcCalculator(sCertCrcFile);
      multisink->addSink(crcCSCalculator);
    }

  }

  iBitDepth = config.iOutputBitDepth;
  tOutputFourCC = config.tOutputFourCC;
  MaxFrames = config.iMaxFrames;

  if(!config.hdrFile.empty())
    pHDRWriter = shared_ptr<HDRWriter>(new HDRWriter(config.hdrFile));
}

/******************************************************************************/
static void sFreeWithoutDestroyingMemory(AL_TBuffer* buffer)
{
  buffer->iChunkCnt = 0;
  AL_Buffer_Destroy(buffer);
}

/******************************************************************************/
void DisplayManager::CopyMetaData(AL_TBuffer* pDstFrame, AL_TBuffer* pSrcFrame, AL_EMetaType eMetaType)
{
  AL_TMetaData* pMetaD = nullptr;

  AL_TMetaData* pOrigMeta = AL_Buffer_GetMetaData(pSrcFrame, eMetaType);

  if(!pOrigMeta)
    throw runtime_error("Metadata does is NULL");
  switch(eMetaType)
  {
  case AL_META_TYPE_PIXMAP:
  {
    pMetaD = (AL_TMetaData*)AL_PixMapMetaData_Clone((AL_TPixMapMetaData*)pOrigMeta);
    break;
  }
  case AL_META_TYPE_DISPLAY_INFO:
  {
    pMetaD = (AL_TMetaData*)AL_DisplayInfoMetaData_Clone((AL_TDisplayInfoMetaData*)pOrigMeta);
    break;
  }
  default:
    throw std::runtime_error("Metadata type is not supported");
    break;
  }

  if(pMetaD == NULL)
    throw runtime_error("Clone of MetaData was not created!");

  if(!AL_Buffer_AddMetaData(pDstFrame, pMetaD))
    throw runtime_error("Cloned pMetaD did not get added!\n");
}

/******************************************************************************/
bool DisplayManager::Process(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo, int iBitDepthAlloc, bool& bIsMainDisplay, bool& bNumFrameReached, bool bDecoderExists)
{
  bNumFrameReached = false;
  bIsMainDisplay = (pInfo->eOutputID == AL_OUTPUT_MAIN || pInfo->eOutputID == AL_OUTPUT_POSTPROC);

  if(bDecoderExists)
  {
    if(NumFrames < MaxFrames)
    {
      if(!AL_Buffer_GetData(pFrame))
        throw runtime_error("Data buffer is null");

      AL_TBuffer* pDisplayFrame = AL_Buffer_ShallowCopy(pFrame, &sFreeWithoutDestroyingMemory);

      auto scopepDisplayFrame = scopeExit([&]() {
        AL_Buffer_Unref(pDisplayFrame);
      });

      AL_Buffer_Ref(pDisplayFrame);
      CopyMetaData(pDisplayFrame, pFrame, AL_META_TYPE_PIXMAP);
      CopyMetaData(pDisplayFrame, pFrame, AL_META_TYPE_DISPLAY_INFO);

      int iCurrentBitDepth = max(pInfo->uBitDepthY, pInfo->uBitDepthC);

      if(iBitDepth == OUTPUT_BD_FIRST)
        iBitDepth = iCurrentBitDepth;
      else if(iBitDepth == OUTPUT_BD_ALLOC)
        iBitDepth = iBitDepthAlloc;

      int iEffectiveBitDepth = iBitDepth == OUTPUT_BD_STREAM ? iCurrentBitDepth : iBitDepth;

      if(bHasOutput)
        ProcessFrame(*pDisplayFrame, *pInfo, iEffectiveBitDepth, tOutputFourCC);

      if(bIsMainDisplay)
      {
        AL_THDRMetaData* pOrigHDRMeta = (AL_THDRMetaData*)AL_Buffer_GetMetaData(pFrame, AL_META_TYPE_HDR);

        if(pOrigHDRMeta != NULL)
        {
          AL_THDRMetaData pHDRMeta;
          AL_HDRMetaData_Copy(pOrigHDRMeta, &pHDRMeta);

          if(pHDRWriter != nullptr)
            pHDRWriter->WriteHDRSEIs(pHDRMeta.eColourDescription, pHDRMeta.eTransferCharacteristics, pHDRMeta.eColourMatrixCoeffs, pHDRMeta.tHDRSEIs);
        }
        // TODO: increase only when last frame
        DisplayFrameStatus(NumFrames);
      }
    }

    if(bIsMainDisplay)
      NumFrames++;
  }

  if(NumFrames >= MaxFrames)
    bNumFrameReached = true;

  return bNumFrameReached;
}

/******************************************************************************/
static void PrintHexdump(ostream* logger, uint8_t* data, int size)
{
  int column = 0;
  int toPrint = size;

  *logger << std::hex;

  while(toPrint > 0)
  {
    *logger << setfill('0') << setw(2) << (int)data[size - toPrint];
    --toPrint;
    ++column;

    if(toPrint > 0)
    {
      if(column % 8 == 0)
        *logger << endl;
      else
        *logger << " ";
    }
  }

  *logger << std::dec;
}

/******************************************************************************/
static void WriteSei(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, ostream* seiOut, int iNumFrame)
{
  if(!seiOut)
    return;

  if(iNumFrame != SEI_NOT_ASSOCIATED_WITH_FRAME)
    *seiOut << "Num Frame: " << iNumFrame << endl;

  *seiOut << "is_prefix: " << boolalpha << bIsPrefix << endl
          << "sei_payload_type: " << iPayloadType << endl
          << "sei_payload_size: " << iPayloadSize << endl
          << "raw:" << endl;
  PrintHexdump(seiOut, pPayload, iPayloadSize);
  *seiOut << endl << endl;
}

/******************************************************************************/
static void WriteSyncSei(std::vector<AL_TSeiMetaData*> seis, ofstream* seiOut, int iNumFrame)
{
  if(!seis.empty())
  {
    for(auto const& pSei: seis)
    {
      auto pPayload = pSei->payload;

      for(auto i = 0; i < pSei->numPayload; ++i, ++pPayload)
        WriteSei(pPayload->bPrefix, pPayload->type, pPayload->pData, pPayload->size, seiOut, iNumFrame);
    }
  }
}

/******************************************************************************/
static string FourCCToString(TFourCC tFourCC)
{
  stringstream ss;
  ss << static_cast<char>(tFourCC & 0xFF) << static_cast<char>((tFourCC & 0xFF00) >> 8) << static_cast<char>((tFourCC & 0xFF0000) >> 16) << static_cast<char>((tFourCC & 0xFF000000) >> 24);
  return ss.str();
}

/******************************************************************************/
static string SequencePictureToString(AL_ESequenceMode sequencePicture)
{
  if(sequencePicture == AL_SM_UNKNOWN)
    return "unknown";

  if(sequencePicture == AL_SM_PROGRESSIVE)
    return "progressive";

  if(sequencePicture == AL_SM_INTERLACED)
    return "interlaced";
  return "max enum";
}

/******************************************************************************/
static void ShowStreamInfo(int BufferNumber, int BufferSize, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo, TFourCC tFourCC)
{
  auto& tDim = pStreamSettings->tDim;
  int iWidth = tDim.iWidth;
  int iHeight = tDim.iHeight;

  stringstream ss;
  ss << "Resolution: " << iWidth << "x" << iHeight << endl;
  ss << "FourCC: " << FourCCToString(tFourCC) << endl;
  ss << "Profile: " << AL_GET_PROFILE_IDC(pStreamSettings->eProfile) << endl;
  int iOutBitdepth = AL_GetBitDepth(tFourCC);

  if(pStreamSettings->iLevel != -1)
    ss << "Level: " << pStreamSettings->iLevel << endl;
  ss << "Bitdepth: " << iOutBitdepth << endl;

  if(AL_NeedsCropping(pCropInfo))
  {
    auto uCropWidth = pCropInfo->uCropOffsetLeft + pCropInfo->uCropOffsetRight;
    auto uCropHeight = pCropInfo->uCropOffsetTop + pCropInfo->uCropOffsetBottom;
    ss << "Crop top: " << pCropInfo->uCropOffsetTop << endl;
    ss << "Crop bottom: " << pCropInfo->uCropOffsetBottom << endl;
    ss << "Crop left: " << pCropInfo->uCropOffsetLeft << endl;
    ss << "Crop right: " << pCropInfo->uCropOffsetRight << endl;
    ss << "Display resolution: " << iWidth - uCropWidth << "x" << iHeight - uCropHeight << endl;
  }
  ss << "Sequence picture: " << SequencePictureToString(pStreamSettings->eSequenceMode) << endl;
  ss << "Buffers needed: " << BufferNumber << " of size " << BufferSize << endl;

  LogInfo(CC_DARK_BLUE, "%s\n", ss.str().c_str());
}

/******************************************************************************/
static int sConfigureDecBufPool(PixMapBufPool& SrcBufPool, AL_TPicFormat const& tPicFormat, AL_TDimension const& tDim, int iPitchY, bool bConfigurePlanarAndSemiplanar, bool bSetMultiChunk)
{
  auto const tFourCC = AL_GetDecFourCC(tPicFormat);
  SrcBufPool.SetFormat(tDim, tFourCC);

  std::vector<AL_TPlaneDescription> vPlaneDesc;
  int iOffset = 0;

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  int iNbPlanes = AL_Plane_GetBufferPixelPlanes(tPicFormat.eChromaOrder, usedPlanes);

  // Set pixels planes
  // -----------------
  for(int iPlane = 0; iPlane < iNbPlanes; iPlane++)
  {
    int iPitch = usedPlanes[iPlane] == AL_PLANE_Y ? iPitchY : AL_GetChromaPitch(tFourCC, iPitchY);
    vPlaneDesc.push_back(AL_TPlaneDescription { usedPlanes[iPlane], iOffset, iPitch });

    /* We ensure compatibility with 420/422. Only required when we use prealloc configured for
     * 444 chroma-mode (worst case) and the real chroma-mode is unknown. Breaks planes agnostic
     * allocation. */

    if(bConfigurePlanarAndSemiplanar && usedPlanes[iPlane] == AL_PLANE_U)
      vPlaneDesc.push_back(AL_TPlaneDescription { AL_PLANE_UV, iOffset, iPitch });

    iOffset += AL_DecGetAllocSize_Frame_PixPlane(tPicFormat.eStorageMode, tDim, iPitch, tPicFormat.eChromaMode, usedPlanes[iPlane]);

    if(bSetMultiChunk)
    {
      SrcBufPool.AddChunk(iOffset, vPlaneDesc);
      vPlaneDesc.clear();
      iOffset = 0;
    }
  }

  if(!bSetMultiChunk)
    SrcBufPool.AddChunk(iOffset, vPlaneDesc);

  return iOffset;
}

/******************************************************************************/
class DecoderContext
{
public:
  DecoderContext(Config& config, AL_TAllocator* pAllocator);
  ~DecoderContext();
  void CreateBaseDecoder(shared_ptr<I_IpDevice> device);
  AL_HDecoder GetBaseDecoderHandle() const { return hBaseDec; }
  AL_ERR SetupBaseDecoderPool(int iBufferNumber, int iBufferSizeLib, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo);

  bool WaitExit(uint32_t uTimeout);
  void ReceiveFrameToDisplayFrom(DeviceType eDevice, AL_TBuffer* pFrame, AL_TInfoDecode* pInfo);
  int GetNumConcealedFrame() const { return iNumFrameConceal; };
  int GetNumDecodedFrames() const { return iNumDecodedFrames; };
  std::unique_lock<mutex> LockDisplay() { return std::unique_lock<mutex>(hDisplayMutex); };
  void StopSendingBuffer() { LockDisplay(); bPushBackToDecoder = false; };
  bool CanSendBackBufferToDecoder() { return bPushBackToDecoder; };
  void ReceiveBaseDecoderDecodedFrame(AL_TBuffer* pFrame);
  void ManageError(AL_ERR eError);
  void StoreSeiMetaData(AL_TBuffer* pParsedFrame, int iParsingId);
  void PrintSei(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize);

private:
  AL_TAllocator* pAllocator;
  AL_HDecoder hBaseDec = nullptr;
  DisplayManager tDisplayManager {};
  bool bPushBackToDecoder = true;
  int iNumFrameConceal = 0;
  int iNumDecodedFrames = 0;
  AL_TDecCallBacks CB {};
  AL_TDecSettings* pDecSettings;
  bool bUsePreAlloc = false;
  bool bBaseBufPoolIsInit = false;
  PixMapBufPool tBaseBufPool;
  bool bSetRecPoolInMultiChunk = false;

  ofstream seiOutput;
  ofstream seiSyncOutput;

  AL_HANDLE GetDecoderHandle(DeviceType eDevice) const;
  AL_ERR TreatError(DeviceType eDevice, AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo);
  void PrintSyncedSeiMetaData(AL_TBuffer* pFrame);
  AL_TDimension ComputeBaseDecoderFinalResolution(AL_TStreamSettings const* pStreamSettings);
  int ComputeBaseDecoderRecBufferSizing(AL_TStreamSettings const* pStreamSettings, AL_TPicFormat* pPicFmt);
  void AttachMetaDataToBaseDecoderRecBuffer(AL_TStreamSettings const* pStreamSettings, AL_TBuffer* pDecPict);

  bool bAddHDRMetaData = false;

  map<AL_TBuffer*, std::vector<AL_TSeiMetaData*>> displaySeis;
  EDecErrorLevel eExitCondition = DEC_ERROR;
  AL_EVENT hExitMain = nullptr;
  mutex hDisplayMutex;
};

/******************************************************************************/
DecoderContext::DecoderContext(Config& config, AL_TAllocator* pAlloc)
{
  pAllocator = pAlloc;
  pDecSettings = &config.tDecSettings;

  tDisplayManager.Configure(config);

  bUsePreAlloc = config.bUsePreAlloc;

  bAddHDRMetaData = !config.hdrFile.empty();

  if(!config.seiFile.empty())
  {
    OpenOutput(seiOutput, config.seiFile);

    if(pDecSettings->eInputMode == AL_DEC_SPLIT_INPUT)
      OpenOutput(seiSyncOutput, config.seiFile + "_sync.txt");
  }

  eExitCondition = config.eExitCondition;
  hExitMain = Rtos_CreateEvent(false);
  bSetRecPoolInMultiChunk = config.bMultiChunk;
}

/******************************************************************************/
DecoderContext::~DecoderContext()
{
  Rtos_DeleteEvent(hExitMain);
}

/******************************************************************************/
AL_HANDLE DecoderContext::GetDecoderHandle(DeviceType eDevice) const
{
  (void)eDevice;
  AL_HANDLE h = hBaseDec;

  return h;
}

/******************************************************************************/
bool DecoderContext::WaitExit(uint32_t uTimeout)
{
  return Rtos_WaitEvent(hExitMain, uTimeout);
}

/******************************************************************************/
static AL_ERR sBaseResolutionFound(int iBufferNumber, int iBufferSizeLib, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo, void* pUserParam)
{
  auto pCtx = (DecoderContext*)pUserParam;
  return pCtx->SetupBaseDecoderPool(iBufferNumber, iBufferSizeLib, pStreamSettings, pCropInfo);
}

/******************************************************************************/
/* duplicated from Utils.h as we can't take these from inside the libraries */
static inline int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) / iRnd * iRnd;
}

/******************************************************************************/
AL_TDimension DecoderContext::ComputeBaseDecoderFinalResolution(AL_TStreamSettings const* pStreamSettings)
{
  AL_TDimension tOutputDim = pStreamSettings->tDim;

  /* For pre-allocation, we must use 8x8 (HEVC) or MB (AVC) rounded dimensions, like the SPS. */
  /* Actually, round up to the LCU so we're able to support resolution changes with the same LCU sizes. */
  /* And because we don't know the codec here, always use 64 as MB/LCU size. */
  tOutputDim.iWidth = RoundUp(tOutputDim.iWidth, 64);
  tOutputDim.iHeight = RoundUp(tOutputDim.iHeight, 64);

  return tOutputDim;
}

/******************************************************************************/
int DecoderContext::ComputeBaseDecoderRecBufferSizing(AL_TStreamSettings const* pStreamSettings, AL_TPicFormat* pPicFmt)
{
  int iBufferSize = 0;
  //
  // Compute output resolution
  // -------------------------
  AL_TDimension tOutputDim = ComputeBaseDecoderFinalResolution(pStreamSettings);

  // Buffer sizing
  // -------------
  auto minPitch = AL_Decoder_GetMinPitch(tOutputDim.iWidth, pStreamSettings->iBitDepth, pPicFmt->eStorageMode);

  if(bBaseBufPoolIsInit)
    iBufferSize = AL_DecGetAllocSize_Frame(tOutputDim, minPitch, pStreamSettings->eChroma, pPicFmt->bCompressed, pPicFmt->eStorageMode);
  else
  {
    bool bConfigurePlanarAndSemiplanar = bUsePreAlloc;
    iBufferSize = sConfigureDecBufPool(tBaseBufPool, *pPicFmt, tOutputDim, minPitch, bConfigurePlanarAndSemiplanar, bSetRecPoolInMultiChunk);
  }

  return iBufferSize;
}

/******************************************************************************/
static void AddHDRMetaData(AL_TBuffer* pBufStream)
{
  if(AL_Buffer_GetMetaData(pBufStream, AL_META_TYPE_HDR))
    return;

  auto pHDReta = AL_HDRMetaData_Create();

  if(pHDReta)
    AL_Buffer_AddMetaData(pBufStream, (AL_TMetaData*)pHDReta);
}

/******************************************************************************/
void DecoderContext::AttachMetaDataToBaseDecoderRecBuffer(AL_TStreamSettings const* pStreamSettings, AL_TBuffer* pDecPict)
{
  (void)pStreamSettings;

  if(bAddHDRMetaData)
    AddHDRMetaData(pDecPict);
  AL_TDisplayInfoMetaData* pDisplayInfoMeta = AL_DisplayInfoMetaData_Create();
  AL_Buffer_AddMetaData(pDecPict, (AL_TMetaData*)pDisplayInfoMeta);
}

/******************************************************************************/
AL_ERR DecoderContext::SetupBaseDecoderPool(int iBufferNumber, int iBufferSizeLib, AL_TStreamSettings const* pStreamSettings, AL_TCropInfo const* pCropInfo)
{
  auto lockDisplay = LockDisplay();

  // Get picture format
  // ------------------
  bool bMainOutputCompression;
  AL_e_FbStorageMode eMainOutputStorageMode = GetMainOutputStorageMode(*pDecSettings, bMainOutputCompression, pStreamSettings->iBitDepth);

  AL_EChromaMode eOutputChromaMode = pStreamSettings->eChroma;
  int iOutputBitdepth = pStreamSettings->iBitDepth;

  auto tPicFormat = AL_GetDecPicFormat(eOutputChromaMode, iOutputBitdepth, eMainOutputStorageMode, bMainOutputCompression);

  // Compute buffer sizing
  // ---------------------
  int iBufferSize = ComputeBaseDecoderRecBufferSizing(pStreamSettings, &tPicFormat);

  if(iBufferSize < iBufferSizeLib)
    throw runtime_error("Buffer size is insufficient");

  ShowStreamInfo(iBufferNumber, iBufferSize, pStreamSettings, pCropInfo, AL_GetDecFourCC(tPicFormat));

  if(bBaseBufPoolIsInit)
    return AL_SUCCESS;

  // Create the buffers
  // ------------------
  int iNumBuf = iBufferNumber + uDefaultNumBuffersHeldByNextComponent;

  if(!tBaseBufPool.Init(pAllocator, iNumBuf, "decoded picture buffer"))
    return AL_ERR_NO_MEMORY;

  bBaseBufPoolIsInit = true;

  // Attach the metas + push to decoder
  // ----------------------------------
  for(int i = 0; i < iNumBuf; ++i)
  {
    auto pDecPict = tBaseBufPool.GetBuffer(AL_BUF_MODE_NONBLOCK);

    if(!pDecPict)
      throw runtime_error("pDecPict is null");

    AL_Buffer_MemSet(pDecPict, 0x00);

    AttachMetaDataToBaseDecoderRecBuffer(pStreamSettings, pDecPict);
    bool const bAdded = AL_Decoder_PutDisplayPicture(GetBaseDecoderHandle(), pDecPict);

    if(!bAdded)
      throw runtime_error("bAdded must be true");

    AL_Buffer_Unref(pDecPict);
  }

  return AL_SUCCESS;
}

/******************************************************************************/
static void sInputParsed(AL_TBuffer* pParsedFrame, void* pUserParam, int iParsingId)
{
  auto pCtx = (DecoderContext*)pUserParam;
  pCtx->StoreSeiMetaData(pParsedFrame, iParsingId);
}

/******************************************************************************/
void DecoderContext::StoreSeiMetaData(AL_TBuffer* pParsedFrame, int iParsingId)
{
  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pParsedFrame, AL_META_TYPE_HANDLE);

  if(!pHandlesMeta)
    return;

  if(iParsingId > AL_HandleMetaData_GetNumHandles(pHandlesMeta))
    throw runtime_error("ParsingId is out of bounds");

  AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, iParsingId);

  if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSED)
  {
    AL_TBuffer* pStream = pDecMetaHandle->pHandle;

    if(!pStream)
      throw runtime_error("pStream is not allocated");

    auto seiMeta = (AL_TSeiMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_SEI);

    if(seiMeta != nullptr)
    {
      AL_Buffer_RemoveMetaData(pStream, (AL_TMetaData*)seiMeta);
      displaySeis[pParsedFrame].push_back(seiMeta);
    }

    return;
  }

  throw runtime_error("Input parsing error");
}

/******************************************************************************/
int convertBitDepthToEven(int iBd)
{
  return ((iBd % 2) != 0) ? iBd + 1 : iBd;
}

static void sFrameDecoded(AL_TBuffer* pFrame, void* pUserParam)
{
  auto pCtx = static_cast<DecoderContext*>(pUserParam);
  pCtx->ReceiveBaseDecoderDecodedFrame(pFrame);
}

void DisplayManager::ProcessFrame(AL_TBuffer& tRecBuf, AL_TInfoDecode info, int iBdOut, TFourCC tFourCCOut)
{
  AL_PixMapBuffer_SetDimension(&tRecBuf, info.tDim);

  iBdOut = convertBitDepthToEven(iBdOut);

  AL_TCropInfo tCrop {};
  tCrop = info.tCrop;
  AL_TPosition tPos = { 0, 0 };

  if(info.tPos.iX || info.tPos.iY)
  {
    tPos = info.tPos;
    tCrop.bCropping = true;
    tCrop.uCropOffsetLeft += info.tPos.iX;
    tCrop.uCropOffsetRight -= info.tPos.iX;
    tCrop.uCropOffsetTop += info.tPos.iY;
    tCrop.uCropOffsetBottom -= info.tPos.iY;
  }

  TFourCC tFourCCRecBuf = AL_PixMapBuffer_GetFourCC(&tRecBuf);
  AL_TPicFormat tRecPicFormat;
  AL_GetPicFormat(tFourCCRecBuf, &tRecPicFormat);

  tFourCCRecBuf = AL_GetFourCC(tRecPicFormat);
  AL_PixMapBuffer_SetFourCC(&tRecBuf, tFourCCRecBuf);

  AL_EFbStorageMode eStorageMode = AL_FB_RASTER;

  AL_TPicFormat tConvPicFormat = AL_TPicFormat {
    tRecPicFormat.eChromaMode, static_cast<uint8_t>(iBdOut), eStorageMode,
    tRecPicFormat.eChromaMode == AL_CHROMA_MONO ? AL_C_ORDER_NO_CHROMA : AL_C_ORDER_U_V, false, false
  };

  if(tFourCCOut == FOURCC(NULL))
    tFourCCOut = AL_GetFourCC(tConvPicFormat);

  bool bCompress = AL_IsCompressed(tFourCCRecBuf);

  if(!bCompress)
  {
    AL_TBuffer* YuvBuffer = NULL;
    ConvertFrameBuffer(&tRecBuf, YuvBuffer, iBdOut, tPos, tFourCCOut);

    auto const iSizePix = (iBdOut + 7) >> 3;

    if(tCrop.bCropping)
      CropFrame(YuvBuffer, iSizePix, tCrop);

    CopyMetaData(YuvBuffer, &tRecBuf, AL_META_TYPE_DISPLAY_INFO);

    multisink->ProcessFrame(YuvBuffer);

    if(tCrop.bCropping)
    {
      tCrop.uCropOffsetBottom = -tCrop.uCropOffsetBottom;
      tCrop.uCropOffsetLeft = -tCrop.uCropOffsetLeft;
      tCrop.uCropOffsetRight = -tCrop.uCropOffsetRight;
      tCrop.uCropOffsetTop = -tCrop.uCropOffsetTop;
      CropFrame(YuvBuffer, iSizePix, tCrop);
    }
    AL_Buffer_Destroy(YuvBuffer);
  }
  else
  {
    AL_TDisplayInfoMetaData* pMeta = reinterpret_cast<AL_TDisplayInfoMetaData*>(AL_Buffer_GetMetaData(&tRecBuf, AL_META_TYPE_DISPLAY_INFO));

    if(pMeta)
      pMeta->tCrop = tCrop;
    multisink->ProcessFrame(&tRecBuf);
  }

}

/******************************************************************************/
void DecoderContext::ReceiveBaseDecoderDecodedFrame(AL_TBuffer* pFrame)
{
  PrintSyncedSeiMetaData(pFrame);

  if(GetBaseDecoderHandle())
    iNumDecodedFrames++;
}

/******************************************************************************/
void DecoderContext::PrintSyncedSeiMetaData(AL_TBuffer* pFrame)
{

  auto seis = displaySeis[pFrame];

  if(seiSyncOutput)
  {
    WriteSyncSei(seis, &seiSyncOutput, iNumDecodedFrames);
  }

  for(auto const& pSei: seis)
    AL_MetaData_Destroy((AL_TMetaData*)pSei);

  displaySeis.erase(pFrame);
}

/******************************************************************************/
static void sParsedSei(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize, void* pUserParam)
{
  auto pCtx = static_cast<DecoderContext*>(pUserParam);
  pCtx->PrintSei(bIsPrefix, iPayloadType, pPayload, iPayloadSize);
}

/******************************************************************************/
void DecoderContext::PrintSei(bool bIsPrefix, int iPayloadType, uint8_t* pPayload, int iPayloadSize)
{
  WriteSei(bIsPrefix, iPayloadType, pPayload, iPayloadSize, &seiOutput, SEI_NOT_ASSOCIATED_WITH_FRAME);
}

/******************************************************************************/
static void sDecoderError(AL_ERR eError, void* pUserParam)
{
  auto pCtx = static_cast<DecoderContext*>(pUserParam);

  pCtx->ManageError(eError);
}

/******************************************************************************/
static void sBaseDecoderFrameDisplay(AL_TBuffer* pFrame, AL_TInfoDecode* pInfo, void* pUserParam)
{
  auto pCtx = reinterpret_cast<DecoderContext*>(pUserParam);
  pCtx->ReceiveFrameToDisplayFrom(DEVICE_BASE_DECODER, pFrame, pInfo);
}

/******************************************************************************/
void DecoderContext::CreateBaseDecoder(shared_ptr<I_IpDevice> device)
{
  CB.endParsingCB = { &sInputParsed, this };
  CB.endDecodingCB = { &sFrameDecoded, this };
  CB.displayCB = { &sBaseDecoderFrameDisplay, this };
  CB.resolutionFoundCB = { &sBaseResolutionFound, this };
  CB.parsedSeiCB = { &sParsedSei, this };
  CB.errorCB = { &sDecoderError, this };

  AL_IDecScheduler* pScheduler = static_cast<AL_IDecScheduler*>(device->GetScheduler());

  AL_ERR error;
  error = AL_Decoder_Create(&hBaseDec, pScheduler, pAllocator, pDecSettings, &CB);

  if(AL_IS_ERROR_CODE(error))
    throw codec_error(error);

  if(!hBaseDec)
    throw runtime_error("Cannot create base decoder");
}

/******************************************************************************/
void DecoderContext::ManageError(AL_ERR eError)
{
  if(AL_IS_ERROR_CODE(eError) || eExitCondition == DEC_WARNING)
    Rtos_SetEvent(hExitMain);
}

/******************************************************************************/
void DecoderContext::ReceiveFrameToDisplayFrom(DeviceType eDevice, AL_TBuffer* pFrame, AL_TInfoDecode* pInfo)
{
  unique_lock<mutex> lock(hDisplayMutex);

  bool bLastFrame = false;

  if(IsEndOfStream(pFrame, pInfo))
  {
    LogVerbose(CC_GREY, "Complete\n\n");
    bLastFrame = true;

  }
  else if(!IsReleaseFrame(pFrame, pInfo))
  {
    AL_Buffer_Ref(pFrame);
    AL_Buffer_InvalidateMemory(pFrame);

    auto err = TreatError(eDevice, pFrame, pInfo);

    if(AL_IS_ERROR_CODE(err))
      bLastFrame = true;
    else
    {
      {
        bool bIsBaseDecoder = eDevice == DEVICE_BASE_DECODER;
        bool bIsFrameMainDisplay;
        auto hDec = GetDecoderHandle(eDevice);
        int iBitDepthAlloc = 8;

        if(bIsBaseDecoder)
          iBitDepthAlloc = AL_Decoder_GetMaxBD(hDec);
        bool bDecoderExists = GetBaseDecoderHandle() != NULL;
        tDisplayManager.Process(pFrame, pInfo, iBitDepthAlloc, bIsFrameMainDisplay, bLastFrame, bDecoderExists);

        if(bIsFrameMainDisplay && CanSendBackBufferToDecoder() && !bLastFrame)
        {
          if(err == AL_WARN_CONCEAL_DETECT || err == AL_WARN_HW_CONCEAL_DETECT || err == AL_WARN_INVALID_ACCESS_UNIT_STRUCTURE)
            iNumFrameConceal++;

          if(bIsBaseDecoder && !AL_Decoder_PutDisplayPicture(GetDecoderHandle(eDevice), pFrame))
            throw runtime_error("bAdded must be true");
        }
      }
    }

    AL_Buffer_Unref(pFrame);
  }

  bool bJobDone = bLastFrame;

  if(bJobDone)
    Rtos_SetEvent(hExitMain);
}

/******************************************************************************/
AL_ERR DecoderContext::TreatError(DeviceType eDevice, AL_TBuffer const* pFrame, AL_TInfoDecode const* pInfo)
{
  bool bExitError = false;
  AL_ERR err = AL_SUCCESS;
  (void)pInfo;

  auto hDec = GetDecoderHandle(eDevice);

  if(hDec)
  {
    if(eDevice == DEVICE_BASE_DECODER)
      err = AL_Decoder_GetFrameError(hDec, pFrame);

    bExitError |= AL_IS_ERROR_CODE(err);
  }

  if(bExitError)
  {
    LogDimmedWarning("\n%s\n", AL_Codec_ErrorToString(err));

    if(err == AL_WARN_SEI_OVERFLOW)
      LogDimmedWarning("\nDecoder has discarded some SEI while the SEI metadata buffer was too small\n");

    LogError("Error: %d\n", err);
  }

  return err;
}

/******************************************************************************/
void ShowStatistics(double durationInSeconds, int iNumFrameConceal, int decodedFrameNumber, bool timeoutOccurred)
{
  string guard = "Decoded time = ";

  if(timeoutOccurred)
    guard = "TIMEOUT = ";

  auto msg = guard + "%.4f s;  Decoding FrameRate ~ %.4f Fps; Frame(s) conceal = %d\n";
  LogInfo(msg.c_str(),
          durationInSeconds,
          decodedFrameNumber / durationInSeconds,
          iNumFrameConceal);
}

/******************************************************************************/
AL_TPixMapMetaData* CreateAndFillPixMapMeta(TFourCC tFourCC, AL_TDimension tDim, int iPitchY)
{
  AL_TPicFormat tPicFmt;
  AL_GetPicFormat(tFourCC, &tPicFmt);
  bool bHasChroma = tPicFmt.eChromaMode != AL_CHROMA_4_0_0;
  bool bIs444 = tPicFmt.eChromaMode == AL_CHROMA_4_4_4;

  auto uSizeY = AL_DecGetAllocSize_Frame_PixPlane(tPicFmt.eStorageMode, tDim, iPitchY, tPicFmt.eChromaMode, AL_PLANE_Y);
  AL_TPlane tPlaneY {
    0, 0, iPitchY
  };

  int iPitchC = 0;
  auto uSizeC = 0;
  AL_TPlane tPlaneU {
    0, 0, 0
  };
  AL_TPlane tPlaneV {
    0, 0, 0
  };
  int uPlaneOffset = 0;

  if(bHasChroma)
  {
    if(bIs444)
    {
      tPlaneU = { 0, uSizeY, iPitchY };
      tPlaneV = { 0, 2 * uSizeY, iPitchY };
    }
    else
    {
      iPitchC = AL_GetChromaPitch(tFourCC, iPitchY);
      uSizeC = AL_DecGetAllocSize_Frame_PixPlane(tPicFmt.eStorageMode, tDim, iPitchC, tPicFmt.eChromaMode, AL_PLANE_UV);
      tPlaneU = { 0, uSizeY, iPitchC };
      tPlaneV = { 0, uSizeY + uSizeC, iPitchC };
    }
  }

  // Attach PixMap
  AL_TPixMapMetaData* pMeta = AL_PixMapMetaData_CreateEmpty(tFourCC);
  AL_PixMapMetaData_AddPlane(pMeta, tPlaneY, AL_PLANE_Y);
  uPlaneOffset += uSizeY;

  if(bHasChroma)
  {
    if(bIs444 || tPicFmt.eChromaOrder != AL_C_ORDER_SEMIPLANAR)
    {
      AL_PixMapMetaData_AddPlane(pMeta, tPlaneU, AL_PLANE_U);
      AL_PixMapMetaData_AddPlane(pMeta, tPlaneV, AL_PLANE_V);
      uPlaneOffset += 2 * uSizeY;
    }
    else
    {
      AL_PixMapMetaData_AddPlane(pMeta, tPlaneU, AL_PLANE_UV);
      uPlaneOffset += uSizeC;
    }
  }

  pMeta->tDim = tDim;

  return pMeta;
}

/******************************************************************************/
typedef void (* EndOfInputCallBack)(AL_HANDLE hDec);
typedef bool (* PushBufferCallBack)(AL_HANDLE hDec, AL_TBuffer* pBuf, size_t uSize, uint8_t uFlags);

struct AsyncFileInput
{
  AsyncFileInput();
  ~AsyncFileInput();
  void Init(AL_HDecoder hDec_, BufPool& bufPool_, EndOfInputCallBack endOfInputCB_, PushBufferCallBack pushBufferCB_);
  void ConfigureStreamInput(string const& sPath, string const& sPathSplitSizes, bool bSplitInput, AL_ECodec eCodec, bool bVclSplit);
  void Start();

private:
  void Run();

  AL_HDecoder m_hDec;
  ifstream m_ifFileStream;
  ifstream ifFileSizes;
  BufPool* m_pBufPool;
  bool m_bStreamInputSet = false;
  std::unique_ptr<InputLoader> m_StreamLoader;
  thread m_thread;
  PushBufferCallBack m_pushBufferCB;
  EndOfInputCallBack m_endOfInputCB;
  atomic<bool> m_bExit;

};

/******************************************************************************/
AsyncFileInput::AsyncFileInput() {}

/******************************************************************************/
AsyncFileInput::~AsyncFileInput()
{
  m_bExit = true;

  if(m_thread.joinable())
    m_thread.join();

}

/******************************************************************************/
void AsyncFileInput::Init(AL_HDecoder hDec, BufPool& bufPool, EndOfInputCallBack endOfInputCB, PushBufferCallBack pushBufferCB)
{
  m_hDec = hDec;
  m_pBufPool = &bufPool;
  m_pushBufferCB = pushBufferCB;
  m_endOfInputCB = endOfInputCB;
  m_bExit = false;
}

/******************************************************************************/
void AsyncFileInput::ConfigureStreamInput(string const& sPath, string const& sPathSplitSizes, bool bSplitInput, AL_ECodec eCodec, bool bVclSplit)
{
  (void)eCodec;
  (void)sPathSplitSizes;
  OpenInput(m_ifFileStream, sPath);
  m_bStreamInputSet = true;

  if(!sPathSplitSizes.empty())
  {
    OpenInput(ifFileSizes, sPathSplitSizes, false);
    m_StreamLoader.reset(new SplitInputFromSizes(ifFileSizes));
  }

  if(bSplitInput)
  {

    if(AL_IS_ITU_CODEC(eCodec))
      m_StreamLoader.reset(new SplitInput(m_pBufPool->GetBufSize(), eCodec, bVclSplit));

  }
  else
    m_StreamLoader.reset(new BasicLoader());
}

/******************************************************************************/
void AsyncFileInput::Start()
{
  if(!m_bStreamInputSet)
    throw runtime_error("Stream input must be set (call AsyncFileInput::ConfigureStreamInput)");

  m_thread = thread(&AsyncFileInput::Run, this);
}

/******************************************************************************/
void AsyncFileInput::Run()
{
  Rtos_SetCurrentThreadName("FileInput");

  while(!m_bExit)
  {
    shared_ptr<AL_TBuffer> pInputBuf;
    try
    {
      pInputBuf = shared_ptr<AL_TBuffer>(
        m_pBufPool->GetBuffer(),
        &AL_Buffer_Unref);
    }
    catch(bufpool_decommited_error &)
    {
      continue;
    }

    uint8_t uBufFlags;
    bool bInputFinished = false;
    uint32_t uAvailSize = 0;

    uAvailSize = m_StreamLoader->ReadStream(m_ifFileStream, pInputBuf.get(), uBufFlags);
    bInputFinished = !uAvailSize;

    if(bInputFinished)
    {
      m_endOfInputCB(m_hDec);
      break;
    }

    auto bRet = m_pushBufferCB(m_hDec, pInputBuf.get(), uAvailSize, uBufFlags);

    if(!bRet)
      throw runtime_error("Failed to push buffer");
  }
}

/******************************************************************************/
constexpr int MAX_CHANNELS = 32;

/******************************************************************************/
int GetChannelsArgv(vector<char*>* argvChannels, int argc, char** argv)
{
  int curChan = 0;

  for(int i = 0; i < argc; ++i)
  {
    if(string(argv[i]) == "--next-chan")
    {
      ++curChan;

      if(curChan >= MAX_CHANNELS)
        throw runtime_error("Too many channels");

      argvChannels[curChan].push_back(argv[0]);
      continue;
    }

    argvChannels[curChan].push_back(argv[i]);
  }

  return curChan;
}

/******************************************************************************/
struct WorkerConfig
{
  Config* pConfig;
  Devices* devices;
  UseBoards* useBoards;
};

/******************************************************************************/
void AdjustStreamBufferSettings(Config& config)
{
  unsigned int uMinStreamBuf = config.tDecSettings.iStackSize;
  config.uInputBufferNum = max(uMinStreamBuf, config.uInputBufferNum);
  config.zInputBufferSize = max(size_t(1), config.zInputBufferSize);

  bool bUsePreAlloc = config.UseBaseDecoder() && config.bUsePreAlloc && config.zInputBufferSize == zDefaultInputBufferSize;

  if(bUsePreAlloc)
    config.zInputBufferSize = AL_GetMaxNalSize(config.tDecSettings.tStream.tDim, config.tDecSettings.tStream.eChroma,
                                               config.tDecSettings.tStream.iBitDepth, config.tDecSettings.tStream.eProfile,
                                               config.tDecSettings.tStream.iLevel);
}

/******************************************************************************/
void CheckAndAdjustChannelConfiguration(Config& config)
{
  FILE* out = g_Verbosity ? stdout : nullptr;

  // Check base decoder settings
  // ---------------------------
  if(config.UseBaseDecoder())
  {
    auto const err = AL_DecSettings_CheckValidity(&config.tDecSettings, out);

    if(err)
    {
      stringstream ss;
      ss << err << " errors(s). " << "Invalid settings, please check your command line.";
      throw runtime_error(ss.str());
    }

    auto const incoherencies = AL_DecSettings_CheckCoherency(&config.tDecSettings, out);

    if(incoherencies == -1)
      throw runtime_error("Fatal coherency error in settings, please check your command line.");
  }

  // Adjust settings
  // ---------------
  AdjustStreamBufferSettings(config);
}

/******************************************************************************/
void ConfigureInputPool(Config const& config, AL_TAllocator* pAllocator, BufPool& tInputPool)
{
  std::string sDebugName = "input_pool";
  unsigned int uNumBuf = config.uInputBufferNum;
  unsigned int zBufSize = config.zInputBufferSize;
  auto pBufPoolAllocator = config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT ? pAllocator : AL_GetDefaultAllocator();
  AL_TMetaData* pBufMeta = nullptr;

  auto ret = tInputPool.Init(pBufPoolAllocator, uNumBuf, zBufSize, pBufMeta, sDebugName);

  if(pBufMeta != nullptr)
    AL_MetaData_Destroy(pBufMeta);

  if(!ret)
    throw runtime_error("Can't create BufPool");
}

/******************************************************************************/
void SafeRunChannelMain(WorkerConfig& w)
{
  auto& config = *w.pConfig;
  AL_TAllocator* pAllocator = nullptr;

  if(config.UseBaseDecoder())
    pAllocator = w.devices->at(DEVICE_BASE_DECODER)->GetAllocator();

  // Settings checkings
  // ------------------
  CheckAndAdjustChannelConfiguration(config);

  // Configure the decoders
  // ----------------------
  DecoderContext tDecCtx(config, pAllocator);

  // Create the decoders
  // -------------------
  if(config.UseBaseDecoder())
  {
    shared_ptr<I_IpDevice> device = w.devices->at(DEVICE_BASE_DECODER);
    tDecCtx.CreateBaseDecoder(device);
  }

  // Parametrization of the base decoder for traces
  // ----------------------------------------------
  if(config.UseBaseDecoder())
  {
    auto hDec = tDecCtx.GetBaseDecoderHandle();
    AL_Decoder_SetParam(hDec, w.useBoards->at(DEVICE_BASE_DECODER) ? "Fpga" : "Ref", config.iTraceIdx, config.iTraceNumber, config.bForceCleanBuffers, config.ipCtrlMode == AL_IPCTRL_MODE_TRACE);
  }

  // Parametrization of the lcevc decoder for traces
  // -----------------------------------------------

  // Configure the stream buffer pool
  // --------------------------------
  // Note : Must be before scopeExit so that AL_Decoder_Destroy can be called
  // before the BufPool destroyer. Can it be done differently so that it is not dependant of this order ?
  BufPool tInputPool;
  ConfigureInputPool(config, pAllocator, tInputPool);

  // Insure destroying is done even after throwing
  // ---------------------------------------------
  auto scopeDecoder = scopeExit([&]() {
    tDecCtx.StopSendingBuffer(); // Prevent to push buffer to the decoder while destroying it

    if(config.UseBaseDecoder())
      AL_Decoder_Destroy(tDecCtx.GetBaseDecoderHandle());
  });

  // Use preallocation for buffer sizing
  // -----------------------------------
  if(config.UseBaseDecoder() && config.bUsePreAlloc)
  {
    auto hDec = tDecCtx.GetBaseDecoderHandle();

    if(!AL_Decoder_PreallocateBuffers(hDec))
      if(auto eErr = AL_Decoder_GetLastError(hDec))
        throw codec_error(eErr);
  }

  // Start feeding the decoder
  // -------------------------
  auto const uBegin = GetPerfTime();
  bool timeoutOccurred = false;

  for(int iLoop = 0; iLoop < config.iLoop; ++iLoop)
  {
    tInputPool.Commit();

    if(iLoop > 0)
      LogVerbose(CC_GREY, "  Looping\n");

    // Setup the reader of bitstream in the file.
    // It will send bitstream chunk to the decoder
    AsyncFileInput producer;
    AL_ECodec eCodec = config.tDecSettings.eCodec;

    producer.Init(tDecCtx.GetBaseDecoderHandle(), tInputPool, AL_Decoder_Flush, AL_Decoder_PushStreamBuffer);

    producer.ConfigureStreamInput(config.sIn, config.sSplitSizesFile, config.tDecSettings.eInputMode == AL_DEC_SPLIT_INPUT, eCodec, config.tDecSettings.eDecUnit == AL_VCL_NAL_UNIT);
    producer.Start();

    auto const maxWait = config.iTimeoutInSeconds * 1000;
    auto const timeout = maxWait >= 0 ? maxWait : AL_WAIT_FOREVER;

    if(!tDecCtx.WaitExit(timeout))
      timeoutOccurred = true;

    tInputPool.Decommit();
  }

  auto const uEnd = GetPerfTime();

  // Prevent the display to produce some outputs
  auto lock = tDecCtx.LockDisplay();

  // Get the errors
  // --------------
  AL_ERR eErr = AL_SUCCESS;

  if(tDecCtx.GetBaseDecoderHandle())
    eErr = AL_Decoder_GetLastError(tDecCtx.GetBaseDecoderHandle());

  if(AL_IS_ERROR_CODE(eErr) || (AL_IS_WARNING_CODE(eErr) && config.eExitCondition == DEC_WARNING))
    throw codec_error(eErr);

  if(!tDecCtx.GetNumDecodedFrames())
    throw runtime_error("No frame decoded");

  auto const duration = (uEnd - uBegin) / 1000.0;
  ShowStatistics(duration, tDecCtx.GetNumConcealedFrame(), tDecCtx.GetNumDecodedFrames(), timeoutOccurred);
}

/******************************************************************************/
static std::shared_ptr<CIpDevice> CreateAndConfigureBaseDecoderIpDevice(Config const* pConfig)
{
  CIpDeviceParam param;

  param.iSchedulerType = pConfig->iSchedulerType;
  param.iDeviceType = pConfig->iDeviceType;
  param.bTrackDma = pConfig->trackDma;
  param.uNumCore = pConfig->tDecSettings.uNumCore;
  param.iHangers = pConfig->hangers;
  param.ipCtrlMode = pConfig->ipCtrlMode;
  param.apbFile = pConfig->apbFile;
  static std::set<std::string> decDevicePath = pConfig->sDecDevicePath;
  param.bSelectDeviceWithLowestAvailableResources = pConfig->bSelectDeviceWithLowestAvailableResources;

  std::shared_ptr<CIpDevice> pIpDevice = std::shared_ptr<CIpDevice>(new CIpDevice(param, pConfig->iDeviceType, { decDevicePath }));

  if(!pIpDevice)
    throw runtime_error("Can't create BaseDecoderIpDevice");

  return pIpDevice;
}

/******************************************************************************/
void SetupArchitecture(Config const& conf)
{
  (void)conf;
  AL_ELibDecoderArch eArch = AL_LIB_DECODER_ARCH_HOST;

  if(AL_Lib_Decoder_Init(eArch) != AL_SUCCESS)
    throw runtime_error("Can't setup decode library");

}

/******************************************************************************/
int GetChannelConfigurations(int argc, char** argv, array<Config, MAX_CHANNELS>& cfgChannels)
{
  vector<char*> argvChannels[MAX_CHANNELS] {};
  int const iNbChan = GetChannelsArgv(argvChannels, argc, argv) + 1;

  for(int chan = 0; chan < iNbChan; ++chan)
    cfgChannels.at(chan) = ParseCommandLine((int)argvChannels[chan].size(), argvChannels[chan].data());

  return iNbChan;
}

/******************************************************************************/
// Run one channel only
static void RunChannelMain(WorkerConfig& w, std::exception_ptr& exception)
{
  try
  {
    SafeRunChannelMain(w);
    exception = nullptr;
    return;
  }
  catch(codec_error const& error)
  {
    (void)error;
    exception = std::current_exception();
  }
  catch(runtime_error const& error)
  {
    (void)error;
    exception = std::current_exception();
  }
}

/******************************************************************************/
void RunChannels(array<Config, MAX_CHANNELS>& cfgChannels, uint8_t uNbChan, Devices& devices, UseBoards& useBoards)
{
  array<std::exception_ptr, MAX_CHANNELS> errorChannels {};
  array<WorkerConfig, MAX_CHANNELS> workerConfigs;

  // Set the worker configurations
  // -----------------------------
  for(int chan = 0; chan < uNbChan; ++chan)
  {
    WorkerConfig w
    {
      &cfgChannels.at(chan),
      &devices,
      &useBoards,
    };

    workerConfigs.at(chan) = w;
  }

  // Mono channel case
  // -----------------
  if(uNbChan == 1)
  {
    RunChannelMain(workerConfigs.at(0), errorChannels.at(0));

    if(errorChannels[0])
      std::rethrow_exception(errorChannels[0]);
  }
  // Multichannel case
  // -----------------
  else
  {
    // Launch all channel in different threads
    array<std::thread, MAX_CHANNELS> workers;

    for(int chan = 0; chan < uNbChan; ++chan)
      workers[chan] = std::thread(&RunChannelMain, std::ref(workerConfigs[chan]), std::ref(errorChannels[chan]));

    // Wait all the channels are finished
    for(int chan = 0; chan < uNbChan; ++chan)
      workers[chan].join();

    // Check for errors
    for(int chan = 0; chan < uNbChan; ++chan)
    {
      if(errorChannels[chan])
      {
        cerr << "Channel " << chan << " has errors" << endl;
        std::rethrow_exception(errorChannels[chan]);
      }
    }
  }
}

/******************************************************************************/
void SafeMain(int argc, char** argv)
{
  InitializePlateform();

  // Get all channel configuration
  // -----------------------------
  array<Config, MAX_CHANNELS> cfgChannels;
  int const maxChan = GetChannelConfigurations(argc, argv, cfgChannels);

  // Use first channel to configure the ip devices
  auto config = cfgChannels[0];

  if(config.help)
    return;

  DisplayVersionInfo();

  // Setup of the decoder(s) architecture
  // ------------------------------------
  SetupArchitecture(config);

  // Create the devices
  // ------------------
  Devices devices;
  UseBoards useBoards;

  if(config.UseBaseDecoder())
  {
    devices.insert({ DEVICE_BASE_DECODER, CreateAndConfigureBaseDecoderIpDevice(&config) });
    useBoards.insert({ DEVICE_BASE_DECODER, (config.iDeviceType == AL_DEVICE_TYPE_BOARD) });
  }

  // Run all the channels
  // --------------------
  RunChannels(cfgChannels, maxChan, devices, useBoards);

  AL_Lib_Decoder_DeInit();
}

/******************************************************************************/
int main(int argc, char** argv)
{
  try
  {
    SafeMain(argc, argv);
    return 0;
  }
  catch(codec_error const& error)
  {
    cerr << endl << "Codec error: " << error.what() << endl;
    return error.Code;
  }
  catch(runtime_error const& error)
  {
    cerr << endl << "Exception caught: " << error.what() << endl;
    return 1;
  }
}

/******************************************************************************/
