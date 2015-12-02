#include "gx2.h"
#include "gx2_sampler.h"

inline uint32_t
floatToFixedPoint(float value, uint32_t bits, float min, float max)
{
   return static_cast<uint32_t>((value - min) * (static_cast<float>(1 << bits) / (max - min)));
}

void
GX2InitSampler(GX2Sampler *sampler,
               GX2TexClampMode clampMode,
               GX2TexXYFilterMode minMagFilterMode)
{
   auto word0 = sampler->regs.word0.value();
   word0.CLAMP_X = static_cast<latte::SQ_TEX_CLAMP>(clampMode);
   word0.CLAMP_Y = static_cast<latte::SQ_TEX_CLAMP>(clampMode);
   word0.CLAMP_Z = static_cast<latte::SQ_TEX_CLAMP>(clampMode);
   word0.XY_MAG_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(minMagFilterMode);
   word0.XY_MIN_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(minMagFilterMode);
   sampler->regs.word0 = word0;
}

void
GX2InitSamplerBorderType(GX2Sampler *sampler,
                         GX2TexBorderType borderType)
{
   auto word0 = sampler->regs.word0.value();
   word0.BORDER_COLOR_TYPE = static_cast<latte::SQ_TEX_BORDER_COLOR>(borderType);
   sampler->regs.word0 = word0;
}

void
GX2InitSamplerClamping(GX2Sampler *sampler,
                       GX2TexClampMode clampX,
                       GX2TexClampMode clampY,
                       GX2TexClampMode clampZ)
{
   auto word0 = sampler->regs.word0.value();
   word0.CLAMP_X = static_cast<latte::SQ_TEX_CLAMP>(clampX);
   word0.CLAMP_Y = static_cast<latte::SQ_TEX_CLAMP>(clampY);
   word0.CLAMP_Z = static_cast<latte::SQ_TEX_CLAMP>(clampZ);
   sampler->regs.word0 = word0;
}

void
GX2InitSamplerDepthCompare(GX2Sampler *sampler,
                           GX2CompareFunction depthCompare)
{
   auto word0 = sampler->regs.word0.value();
   word0.DEPTH_COMPARE_FUNCTION = static_cast<latte::SQ_TEX_DEPTH_COMPARE>(depthCompare);
   sampler->regs.word0 = word0;
}

void
GX2InitSamplerFilterAdjust(GX2Sampler *sampler,
                           BOOL highPrecision,
                           GX2TexMipPerfMode perfMip,
                           GX2TexZPerfMode perfZ)
{
   auto word2 = sampler->regs.word2.value();
   word2.HIGH_PRECISION_FILTER = highPrecision;
   word2.PERF_MIP = perfMip;
   word2.PERF_Z = perfZ;
   sampler->regs.word2 = word2;
}

void
GX2InitSamplerLOD(GX2Sampler *sampler,
                  float lodMin,
                  float lodMax,
                  float lodBias)
{
   auto word1 = sampler->regs.word1.value();
   word1.MIN_LOD = floatToFixedPoint(lodMin, 10, 0.0f, 16.0f);
   word1.MAX_LOD = floatToFixedPoint(lodMax, 10, 0.0f, 16.0f);
   word1.LOD_BIAS = floatToFixedPoint(lodBias, 12, -32.0f, 32.0f);
   sampler->regs.word1 = word1;
}

void
GX2InitSamplerLODAdjust(GX2Sampler *sampler,
                        float anisoBias,
                        BOOL lodUsesMinorAxis)
{
   auto word0 = sampler->regs.word0.value();
   auto word2 = sampler->regs.word2.value();
   word2.ANISO_BIAS = floatToFixedPoint(anisoBias, 6, 0.0f, 2.0f);
   word0.LOD_USES_MINOR_AXIS = lodUsesMinorAxis;
   sampler->regs.word0 = word0;
   sampler->regs.word2 = word2;
}

void
GX2InitSamplerRoundingMode(GX2Sampler *sampler,
                           GX2RoundingMode roundingMode)
{
   auto word2 = sampler->regs.word2.value();
   word2.TRUNCATE_COORD = roundingMode;
   sampler->regs.word2 = word2;
}

void
GX2InitSamplerXYFilter(GX2Sampler *sampler,
                       GX2TexXYFilterMode filterMag,
                       GX2TexXYFilterMode filterMin,
                       GX2TexAnisoRatio maxAniso)
{
   auto word0 = sampler->regs.word0.value();
   word0.XY_MAG_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(filterMag);
   word0.XY_MIN_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(filterMin);
   word0.MAX_ANISO_RATIO = static_cast<latte::SQ_TEX_ANISO>(maxAniso);
   sampler->regs.word0 = word0;
}

void
GX2InitSamplerZMFilter(GX2Sampler *sampler,
                       GX2TexZFilterMode filterZ,
                       GX2TexMipFilterMode filterMip)
{
   auto word0 = sampler->regs.word0.value();
   word0.Z_FILTER = static_cast<latte::SQ_TEX_Z_FILTER>(filterZ);
   word0.MIP_FILTER = static_cast<latte::SQ_TEX_Z_FILTER>(filterMip);
   sampler->regs.word0 = word0;
}
