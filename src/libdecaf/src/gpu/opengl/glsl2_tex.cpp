#include "glsl2_translate.h"
#include "gpu/microcode/latte_instructions.h"

using namespace latte;

/*
Unimplemented:
VTX_FETCH
VTX_SEMANTIC
MEM
LD
GET_TEXTURE_INFO
GET_SAMPLE_INFO
GET_COMP_TEX_LOD
GET_GRADIENTS_H
GET_GRADIENTS_V
GET_LERP
KEEP_GRADIENTS
SET_GRADIENTS_H
SET_GRADIENTS_V
PASS
SET_CUBEMAP_INDEX
SAMPLE_L
SAMPLE_LB
SAMPLE_G
SAMPLE_G_L
SAMPLE_G_LB
SAMPLE_G_LZ
SAMPLE_C_L
SAMPLE_C_LB
SAMPLE_C_LZ
SAMPLE_C_G
SAMPLE_C_G_L
SAMPLE_C_G_LB
SAMPLE_C_G_LZ
SET_TEXTURE_OFFSETS
GATHER4
GATHER4_O
GATHER4_C
GATHER4_C_O
GET_BUFFER_RESINFO
*/

namespace glsl2
{

static char
getSelectChannel(SQ_SEL sel)
{
   switch (sel) {
   case SQ_SEL_X:
      return 'x';
   case SQ_SEL_Y:
      return 'y';
   case SQ_SEL_Z:
      return 'z';
   case SQ_SEL_W:
      return 'w';
   default:
      throw translate_exception(fmt::format("Unexpected SQ_SEL {}", sel));
   }
}

static bool
isShadowSampler(SamplerType type)
{
   switch (type) {
   case SamplerType::Sampler1DShadow:
   case SamplerType::Sampler2DShadow:
   case SamplerType::SamplerCubeShadow:
   case SamplerType::Sampler2DRectShadow:
   case SamplerType::Sampler1DArrayShadow:
   case SamplerType::Sampler2DArrayShadow:
   case SamplerType::SamplerCubeArrayShadow:
      return true;
   default:
      return false;
   }
}

static unsigned
getSamplerArgCount(SamplerType type)
{
   switch (type) {
   case SamplerType::Sampler1D:
      return 1;
   case SamplerType::Sampler2D:
      return 2;
   case SamplerType::Sampler3D:
      return 3;
   case SamplerType::Sampler1DArray:
      return 1 + 1;
   case SamplerType::Sampler2DArray:
      return 2 + 1;
   case SamplerType::Sampler1DShadow:
      return 1 + 1;
   case SamplerType::Sampler2DShadow:
      return 2 + 1;
   case SamplerType::Sampler1DArrayShadow:
      return 1 + 1 + 1;
   case SamplerType::Sampler2DArrayShadow:
      return 2 + 1 + 1;
   case SamplerType::SamplerCube:
      return 3;
   case SamplerType::SamplerCubeArray:
      return 3 + 1;
   case SamplerType::SamplerCubeShadow:
      return 3 + 1;
   case SamplerType::Sampler2DRect:
   case SamplerType::SamplerBuffer:
   case SamplerType::Sampler2DMS:
   case SamplerType::Sampler2DMSArray:
   case SamplerType::Sampler2DRectShadow:
   case SamplerType::SamplerCubeArrayShadow:
   default:
      throw translate_exception(fmt::format("Unsupported sampler type {}", static_cast<unsigned>(type)));
   }
}

static void
registerSamplerID(State &state, unsigned id)
{
   if (state.shader) {
      state.shader->samplerUsed[id] = true;
   }
}

static void
sampleFunc(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst, const std::string &func, const std::string &extraArgs = "")
{
   auto dstSelX = inst.word1.DST_SEL_X().get();
   auto dstSelY = inst.word1.DST_SEL_Y().get();
   auto dstSelZ = inst.word1.DST_SEL_Z().get();
   auto dstSelW = inst.word1.DST_SEL_W().get();

   auto srcSelX = inst.word2.SRC_SEL_X().get();
   auto srcSelY = inst.word2.SRC_SEL_Y().get();
   auto srcSelZ = inst.word2.SRC_SEL_Z().get();
   auto srcSelW = inst.word2.SRC_SEL_W().get();

   auto resourceID = inst.word0.RESOURCE_ID();
   auto samplerID = inst.word2.SAMPLER_ID();
   auto samplerType = glsl2::SamplerType::Sampler2D;
   registerSamplerID(state, samplerID);

   if (state.shader) {
      samplerType = state.shader->samplers[samplerID];
   }

   if (resourceID != samplerID) {
      throw translate_exception("Unsupported sample with RESOURCE_ID != SAMPLER_ID");
   }

   auto dst = getExportRegister(inst.word1.DST_GPR(), inst.word1.DST_REL());
   auto src = getExportRegister(inst.word0.SRC_GPR(), inst.word0.SRC_REL());

   auto numDstSels = 4u;
   auto dstSelMask = condenseSelections(dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);

   if (numDstSels > 0) {
      insertLineStart(state);
      state.out << "texTmp = " << func << "(sampler_" << samplerID << ", ";

      auto samplerElements = getSamplerArgCount(samplerType);

      if (isShadowSampler(samplerType)) {
         /* In r600 the .w channel holds the compare value whereas OpenGL
          * shadow samplers just expect it to be the last texture coordinate
          * so we must set the last channel to SQ_SEL_W
          */
         if (samplerElements == 2) {
            srcSelY = SQ_SEL_W;
         } else if (samplerElements == 3) {
            srcSelZ = SQ_SEL_W;
         } else if (samplerElements == 4) {
            srcSelW = SQ_SEL_W;
         } else {
            decaf_abort(fmt::format("Unexpected samplerElements {} for shadow sampler", samplerElements));
         }
      }

      insertSelectVector(state.out, src, srcSelX, srcSelY, srcSelZ, srcSelW, samplerElements);

      state.out << extraArgs << ");";
      insertLineEnd(state);

      insertLineStart(state);
      state.out << dst << "." << dstSelMask;
      state.out << " = ";
      insertSelectVector(state.out, "texTmp", dstSelX, dstSelY, dstSelZ, dstSelW, numDstSels);
      state.out << ";";
      insertLineEnd(state);
   }
}

static void
FETCH4(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   sampleFunc(state, cf, inst, "textureGather");
}

static void
SAMPLE(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   sampleFunc(state, cf, inst, "texture");
}

static void
SAMPLE_LZ(State &state, const latte::ControlFlowInst &cf, const latte::TextureFetchInst &inst)
{
   // Sample with LOD Zero
   sampleFunc(state, cf, inst, "textureLod", ", 0");
}

void
registerTexFunctions()
{
   registerInstruction(SQ_TEX_INST_FETCH4, FETCH4);
   registerInstruction(SQ_TEX_INST_SAMPLE, SAMPLE);
   registerInstruction(SQ_TEX_INST_SAMPLE_C, SAMPLE);
   registerInstruction(SQ_TEX_INST_SAMPLE_LZ, SAMPLE_LZ);
}

} // namespace glsl2
