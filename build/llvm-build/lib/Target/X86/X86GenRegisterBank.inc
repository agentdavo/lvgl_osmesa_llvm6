/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Register Bank Source Fragments                                             *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifdef GET_REGBANK_DECLARATIONS
#undef GET_REGBANK_DECLARATIONS
namespace llvm {
namespace X86 {
enum : unsigned {
  InvalidRegBankID = ~0u,
  GPRRegBankID = 0,
  PSRRegBankID = 1,
  VECRRegBankID = 2,
  NumRegisterBanks,
};
} // end namespace X86
} // end namespace llvm
#endif // GET_REGBANK_DECLARATIONS

#ifdef GET_TARGET_REGBANK_CLASS
#undef GET_TARGET_REGBANK_CLASS
private:
  static const RegisterBank *RegBanks[];
  static const unsigned Sizes[];

public:
  const RegisterBank &getRegBankFromRegClass(const TargetRegisterClass &RC, LLT Ty) const override;
protected:
  X86GenRegisterBankInfo(unsigned HwMode = 0);

#endif // GET_TARGET_REGBANK_CLASS

#ifdef GET_TARGET_REGBANK_IMPL
#undef GET_TARGET_REGBANK_IMPL
namespace llvm {
namespace X86 {
const uint32_t GPRRegBankCoverageData[] = {
    // 0-31
    (1u << (X86::GR8RegClassID - 0)) |
    (1u << (X86::GR16RegClassID - 0)) |
    (1u << (X86::LOW32_ADDR_ACCESS_RBPRegClassID - 0)) |
    (1u << (X86::GR8_NOREX2RegClassID - 0)) |
    (1u << (X86::GR16_NOREX2RegClassID - 0)) |
    (1u << (X86::GR8_NOREXRegClassID - 0)) |
    (1u << (X86::GR8_ABCD_HRegClassID - 0)) |
    (1u << (X86::GR8_ABCD_LRegClassID - 0)) |
    (1u << (X86::GR16_NOREXRegClassID - 0)) |
    (1u << (X86::GR16_ABCDRegClassID - 0)) |
    0,
    // 32-63
    (1u << (X86::LOW32_ADDR_ACCESSRegClassID - 32)) |
    (1u << (X86::LOW32_ADDR_ACCESS_RBP_with_sub_8bitRegClassID - 32)) |
    (1u << (X86::GR32RegClassID - 32)) |
    (1u << (X86::GR32_NOSPRegClassID - 32)) |
    (1u << (X86::LOW32_ADDR_ACCESS_RBP_with_sub_16bit_in_GR16_NOREX2RegClassID - 32)) |
    (1u << (X86::GR32_NOREX2RegClassID - 32)) |
    (1u << (X86::GR32_NOREX2_NOSPRegClassID - 32)) |
    (1u << (X86::LOW32_ADDR_ACCESS_RBP_with_sub_16bit_in_GR16_NOREXRegClassID - 32)) |
    (1u << (X86::GR32_NOREXRegClassID - 32)) |
    (1u << (X86::GR32_NOREX_NOSPRegClassID - 32)) |
    (1u << (X86::GR32_ABCDRegClassID - 32)) |
    (1u << (X86::GR32_TCRegClassID - 32)) |
    (1u << (X86::GR32_ABCD_and_GR32_TCRegClassID - 32)) |
    (1u << (X86::GR32_ADRegClassID - 32)) |
    (1u << (X86::GR32_ArgRefRegClassID - 32)) |
    (1u << (X86::GR32_DCRegClassID - 32)) |
    (1u << (X86::GR32_AD_and_GR32_ArgRefRegClassID - 32)) |
    (1u << (X86::GR32_CBRegClassID - 32)) |
    (1u << (X86::GR32_SIDIRegClassID - 32)) |
    (1u << (X86::GR32_BSIRegClassID - 32)) |
    (1u << (X86::GR32_DIBPRegClassID - 32)) |
    (1u << (X86::GR32_ABCD_and_GR32_BSIRegClassID - 32)) |
    (1u << (X86::GR32_BPSPRegClassID - 32)) |
    0,
    // 64-95
    (1u << (X86::GR64RegClassID - 64)) |
    (1u << (X86::GR64_with_sub_8bitRegClassID - 64)) |
    (1u << (X86::GR64_NOSPRegClassID - 64)) |
    (1u << (X86::GR64_NOREX2_NOSPRegClassID - 64)) |
    (1u << (X86::GR64PLTSafeRegClassID - 64)) |
    (1u << (X86::GR64PLTSafe_and_GR64_TCRegClassID - 64)) |
    (1u << (X86::GR32_ArgRef_and_GR32_CBRegClassID - 64)) |
    (1u << (X86::GR32_BSI_and_GR32_SIDIRegClassID - 64)) |
    (1u << (X86::GR32_DIBP_and_GR32_SIDIRegClassID - 64)) |
    (1u << (X86::GR64_NOREX_NOSPRegClassID - 64)) |
    (1u << (X86::GR32_BPSP_and_GR32_DIBPRegClassID - 64)) |
    (1u << (X86::GR64_NOREX2_NOSP_and_GR64_TCRegClassID - 64)) |
    (1u << (X86::GR64_NOREX2_NOSP_and_GR64_TCW64RegClassID - 64)) |
    (1u << (X86::GR64_with_sub_16bit_in_GR16_NOREX2RegClassID - 64)) |
    (1u << (X86::GR64_TC_with_sub_8bitRegClassID - 64)) |
    (1u << (X86::GR32_BPSP_and_GR32_TCRegClassID - 64)) |
    (1u << (X86::GR64_TCW64_with_sub_8bitRegClassID - 64)) |
    (1u << (X86::GR64_with_sub_16bit_in_GR16_NOREXRegClassID - 64)) |
    (1u << (X86::GR64_NOREX2RegClassID - 64)) |
    (1u << (X86::GR64_TCRegClassID - 64)) |
    (1u << (X86::GR64_TC_and_GR64_TCW64RegClassID - 64)) |
    (1u << (X86::GR64_NOREX_and_GR64_TCRegClassID - 64)) |
    (1u << (X86::GR64_NOREXRegClassID - 64)) |
    (1u << (X86::GR64_TCW64RegClassID - 64)) |
    0,
    // 96-127
    (1u << (X86::GR64PLTSafe_and_GR64_TCW64RegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_TCRegClassID - 96)) |
    (1u << (X86::GR64_ADRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_AD_and_GR32_ArgRefRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_ArgRefRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_ArgRef_and_GR32_CBRegClassID - 96)) |
    (1u << (X86::GR64_NOREX_and_GR64PLTSafe_and_GR64_TCRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_SIDIRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_BSI_and_GR32_SIDIRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_DIBP_and_GR32_SIDIRegClassID - 96)) |
    (1u << (X86::GR64_ABCDRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_CBRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_BSIRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_BSIRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_DIBPRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_BPSP_and_GR32_DIBPRegClassID - 96)) |
    (1u << (X86::GR64_TC_and_GR64_NOREX2_NOSP_and_GR64_TCW64RegClassID - 96)) |
    (1u << (X86::GR64_ArgRef_and_GR64_TCRegClassID - 96)) |
    (1u << (X86::GR64_ArgRefRegClassID - 96)) |
    (1u << (X86::GR64_TCW64_and_GR64_TC_with_sub_8bitRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_TCRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_BPSP_and_GR32_TCRegClassID - 96)) |
    (1u << (X86::GR64_TC_and_GR64_with_sub_16bit_in_GR16_NOREXRegClassID - 96)) |
    (1u << (X86::GR64_with_sub_32bit_in_GR32_BPSPRegClassID - 96)) |
    (1u << (X86::GR64_NOREX_and_GR64_TCW64RegClassID - 96)) |
    (1u << (X86::GR64_and_LOW32_ADDR_ACCESSRegClassID - 96)) |
    (1u << (X86::GR64_and_LOW32_ADDR_ACCESS_RBPRegClassID - 96)) |
    0,
    // 128-159
    0,
};
const uint32_t PSRRegBankCoverageData[] = {
    // 0-31
    0,
    // 32-63
    (1u << (X86::RFP32RegClassID - 32)) |
    0,
    // 64-95
    (1u << (X86::RFP64RegClassID - 64)) |
    0,
    // 96-127
    (1u << (X86::RFP80RegClassID - 96)) |
    0,
    // 128-159
    0,
};
const uint32_t VECRRegBankCoverageData[] = {
    // 0-31
    (1u << (X86::FR16XRegClassID - 0)) |
    (1u << (X86::FR16RegClassID - 0)) |
    0,
    // 32-63
    (1u << (X86::FR32XRegClassID - 32)) |
    (1u << (X86::FR32RegClassID - 32)) |
    0,
    // 64-95
    (1u << (X86::FR64XRegClassID - 64)) |
    (1u << (X86::FR64RegClassID - 64)) |
    0,
    // 96-127
    (1u << (X86::VR128XRegClassID - 96)) |
    0,
    // 128-159
    (1u << (X86::VR512RegClassID - 128)) |
    (1u << (X86::VR256XRegClassID - 128)) |
    (1u << (X86::VR512_0_15RegClassID - 128)) |
    (1u << (X86::VR128RegClassID - 128)) |
    (1u << (X86::VR256RegClassID - 128)) |
    0,
};

constexpr RegisterBank GPRRegBank(/* ID */ X86::GPRRegBankID, /* Name */ "GPR", /* CoveredRegClasses */ GPRRegBankCoverageData, /* NumRegClasses */ 135);
constexpr RegisterBank PSRRegBank(/* ID */ X86::PSRRegBankID, /* Name */ "PSR", /* CoveredRegClasses */ PSRRegBankCoverageData, /* NumRegClasses */ 135);
constexpr RegisterBank VECRRegBank(/* ID */ X86::VECRRegBankID, /* Name */ "VECR", /* CoveredRegClasses */ VECRRegBankCoverageData, /* NumRegClasses */ 135);
} // end namespace X86

const RegisterBank *X86GenRegisterBankInfo::RegBanks[] = {
    &X86::GPRRegBank,
    &X86::PSRRegBank,
    &X86::VECRRegBank,
};

const unsigned X86GenRegisterBankInfo::Sizes[] = {
    // Mode = 0 (Default)
    64,
    80,
    512,
};

X86GenRegisterBankInfo::X86GenRegisterBankInfo(unsigned HwMode)
    : RegisterBankInfo(RegBanks, X86::NumRegisterBanks, Sizes, HwMode) {
  // Assert that RegBank indices match their ID's
#ifndef NDEBUG
  for (auto RB : enumerate(RegBanks))
    assert(RB.index() == RB.value()->getID() && "Index != ID");
#endif // NDEBUG
}
const RegisterBank &
X86GenRegisterBankInfo::getRegBankFromRegClass(const TargetRegisterClass &RC, LLT) const {
  constexpr uint32_t InvalidRegBankID = uint32_t(X86::InvalidRegBankID) & 3;
  static const uint32_t RegClass2RegBank[9] = {
    (uint32_t(X86::GPRRegBankID) << 0) | // GR8RegClassID
    (uint32_t(InvalidRegBankID) << 2) |
    (uint32_t(X86::GPRRegBankID) << 4) | // GR8_NOREX2RegClassID
    (uint32_t(X86::GPRRegBankID) << 6) | // GR8_NOREXRegClassID
    (uint32_t(X86::GPRRegBankID) << 8) | // GR8_ABCD_HRegClassID
    (uint32_t(X86::GPRRegBankID) << 10) | // GR8_ABCD_LRegClassID
    (uint32_t(InvalidRegBankID) << 12) |
    (uint32_t(X86::GPRRegBankID) << 14) | // GR16RegClassID
    (uint32_t(X86::GPRRegBankID) << 16) | // GR16_NOREX2RegClassID
    (uint32_t(X86::GPRRegBankID) << 18) | // GR16_NOREXRegClassID
    (uint32_t(InvalidRegBankID) << 20) |
    (uint32_t(InvalidRegBankID) << 22) |
    (uint32_t(InvalidRegBankID) << 24) |
    (uint32_t(InvalidRegBankID) << 26) |
    (uint32_t(InvalidRegBankID) << 28) |
    (uint32_t(InvalidRegBankID) << 30),
    (uint32_t(InvalidRegBankID) << 0) |
    (uint32_t(InvalidRegBankID) << 2) |
    (uint32_t(InvalidRegBankID) << 4) |
    (uint32_t(InvalidRegBankID) << 6) |
    (uint32_t(InvalidRegBankID) << 8) |
    (uint32_t(X86::GPRRegBankID) << 10) | // GR16_ABCDRegClassID
    (uint32_t(InvalidRegBankID) << 12) |
    (uint32_t(X86::VECRRegBankID) << 14) | // FR16XRegClassID
    (uint32_t(X86::VECRRegBankID) << 16) | // FR16RegClassID
    (uint32_t(InvalidRegBankID) << 18) |
    (uint32_t(InvalidRegBankID) << 20) |
    (uint32_t(InvalidRegBankID) << 22) |
    (uint32_t(InvalidRegBankID) << 24) |
    (uint32_t(InvalidRegBankID) << 26) |
    (uint32_t(InvalidRegBankID) << 28) |
    (uint32_t(X86::GPRRegBankID) << 30), // LOW32_ADDR_ACCESS_RBPRegClassID
    (uint32_t(X86::GPRRegBankID) << 0) | // LOW32_ADDR_ACCESSRegClassID
    (uint32_t(X86::GPRRegBankID) << 2) | // LOW32_ADDR_ACCESS_RBP_with_sub_8bitRegClassID
    (uint32_t(X86::VECRRegBankID) << 4) | // FR32XRegClassID
    (uint32_t(X86::GPRRegBankID) << 6) | // GR32RegClassID
    (uint32_t(X86::GPRRegBankID) << 8) | // GR32_NOSPRegClassID
    (uint32_t(X86::GPRRegBankID) << 10) | // LOW32_ADDR_ACCESS_RBP_with_sub_16bit_in_GR16_NOREX2RegClassID
    (uint32_t(InvalidRegBankID) << 12) |
    (uint32_t(X86::VECRRegBankID) << 14) | // FR32RegClassID
    (uint32_t(X86::GPRRegBankID) << 16) | // GR32_NOREX2RegClassID
    (uint32_t(X86::GPRRegBankID) << 18) | // GR32_NOREX2_NOSPRegClassID
    (uint32_t(X86::GPRRegBankID) << 20) | // LOW32_ADDR_ACCESS_RBP_with_sub_16bit_in_GR16_NOREXRegClassID
    (uint32_t(X86::GPRRegBankID) << 22) | // GR32_NOREXRegClassID
    (uint32_t(InvalidRegBankID) << 24) |
    (uint32_t(X86::GPRRegBankID) << 26) | // GR32_NOREX_NOSPRegClassID
    (uint32_t(X86::PSRRegBankID) << 28) | // RFP32RegClassID
    (uint32_t(InvalidRegBankID) << 30),
    (uint32_t(X86::GPRRegBankID) << 0) | // GR32_ABCDRegClassID
    (uint32_t(X86::GPRRegBankID) << 2) | // GR32_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 4) | // GR32_ABCD_and_GR32_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 6) | // GR32_ADRegClassID
    (uint32_t(X86::GPRRegBankID) << 8) | // GR32_ArgRefRegClassID
    (uint32_t(X86::GPRRegBankID) << 10) | // GR32_BPSPRegClassID
    (uint32_t(X86::GPRRegBankID) << 12) | // GR32_BSIRegClassID
    (uint32_t(X86::GPRRegBankID) << 14) | // GR32_CBRegClassID
    (uint32_t(X86::GPRRegBankID) << 16) | // GR32_DCRegClassID
    (uint32_t(X86::GPRRegBankID) << 18) | // GR32_DIBPRegClassID
    (uint32_t(X86::GPRRegBankID) << 20) | // GR32_SIDIRegClassID
    (uint32_t(InvalidRegBankID) << 22) |
    (uint32_t(InvalidRegBankID) << 24) |
    (uint32_t(InvalidRegBankID) << 26) |
    (uint32_t(X86::GPRRegBankID) << 28) | // GR32_ABCD_and_GR32_BSIRegClassID
    (uint32_t(X86::GPRRegBankID) << 30), // GR32_AD_and_GR32_ArgRefRegClassID
    (uint32_t(X86::GPRRegBankID) << 0) | // GR32_ArgRef_and_GR32_CBRegClassID
    (uint32_t(X86::GPRRegBankID) << 2) | // GR32_BPSP_and_GR32_DIBPRegClassID
    (uint32_t(X86::GPRRegBankID) << 4) | // GR32_BPSP_and_GR32_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 6) | // GR32_BSI_and_GR32_SIDIRegClassID
    (uint32_t(X86::GPRRegBankID) << 8) | // GR32_DIBP_and_GR32_SIDIRegClassID
    (uint32_t(InvalidRegBankID) << 10) |
    (uint32_t(InvalidRegBankID) << 12) |
    (uint32_t(X86::PSRRegBankID) << 14) | // RFP64RegClassID
    (uint32_t(X86::GPRRegBankID) << 16) | // GR64RegClassID
    (uint32_t(X86::VECRRegBankID) << 18) | // FR64XRegClassID
    (uint32_t(X86::GPRRegBankID) << 20) | // GR64_with_sub_8bitRegClassID
    (uint32_t(X86::GPRRegBankID) << 22) | // GR64_NOSPRegClassID
    (uint32_t(X86::GPRRegBankID) << 24) | // GR64_NOREX2RegClassID
    (uint32_t(InvalidRegBankID) << 26) |
    (uint32_t(X86::VECRRegBankID) << 28) | // FR64RegClassID
    (uint32_t(X86::GPRRegBankID) << 30), // GR64_with_sub_16bit_in_GR16_NOREX2RegClassID
    (uint32_t(X86::GPRRegBankID) << 0) | // GR64_NOREX2_NOSPRegClassID
    (uint32_t(X86::GPRRegBankID) << 2) | // GR64PLTSafeRegClassID
    (uint32_t(X86::GPRRegBankID) << 4) | // GR64_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 6) | // GR64_NOREXRegClassID
    (uint32_t(X86::GPRRegBankID) << 8) | // GR64_TCW64RegClassID
    (uint32_t(X86::GPRRegBankID) << 10) | // GR64_TC_with_sub_8bitRegClassID
    (uint32_t(X86::GPRRegBankID) << 12) | // GR64_NOREX2_NOSP_and_GR64_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 14) | // GR64_TCW64_with_sub_8bitRegClassID
    (uint32_t(X86::GPRRegBankID) << 16) | // GR64_TC_and_GR64_TCW64RegClassID
    (uint32_t(X86::GPRRegBankID) << 18) | // GR64_with_sub_16bit_in_GR16_NOREXRegClassID
    (uint32_t(InvalidRegBankID) << 20) |
    (uint32_t(InvalidRegBankID) << 22) |
    (uint32_t(X86::GPRRegBankID) << 24) | // GR64PLTSafe_and_GR64_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 26) | // GR64_NOREX2_NOSP_and_GR64_TCW64RegClassID
    (uint32_t(X86::GPRRegBankID) << 28) | // GR64_NOREX_NOSPRegClassID
    (uint32_t(X86::GPRRegBankID) << 30), // GR64_NOREX_and_GR64_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 0) | // GR64_TCW64_and_GR64_TC_with_sub_8bitRegClassID
    (uint32_t(InvalidRegBankID) << 2) |
    (uint32_t(X86::GPRRegBankID) << 4) | // GR64_TC_and_GR64_NOREX2_NOSP_and_GR64_TCW64RegClassID
    (uint32_t(X86::GPRRegBankID) << 6) | // GR64_TC_and_GR64_with_sub_16bit_in_GR16_NOREXRegClassID
    (uint32_t(X86::GPRRegBankID) << 8) | // GR64PLTSafe_and_GR64_TCW64RegClassID
    (uint32_t(X86::GPRRegBankID) << 10) | // GR64_NOREX_and_GR64PLTSafe_and_GR64_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 12) | // GR64_NOREX_and_GR64_TCW64RegClassID
    (uint32_t(X86::GPRRegBankID) << 14) | // GR64_ABCDRegClassID
    (uint32_t(X86::GPRRegBankID) << 16) | // GR64_with_sub_32bit_in_GR32_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 18) | // GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 20) | // GR64_ADRegClassID
    (uint32_t(X86::GPRRegBankID) << 22) | // GR64_ArgRefRegClassID
    (uint32_t(X86::GPRRegBankID) << 24) | // GR64_and_LOW32_ADDR_ACCESS_RBPRegClassID
    (uint32_t(X86::GPRRegBankID) << 26) | // GR64_with_sub_32bit_in_GR32_ArgRefRegClassID
    (uint32_t(X86::GPRRegBankID) << 28) | // GR64_with_sub_32bit_in_GR32_BPSPRegClassID
    (uint32_t(X86::GPRRegBankID) << 30), // GR64_with_sub_32bit_in_GR32_BSIRegClassID
    (uint32_t(X86::GPRRegBankID) << 0) | // GR64_with_sub_32bit_in_GR32_CBRegClassID
    (uint32_t(X86::GPRRegBankID) << 2) | // GR64_with_sub_32bit_in_GR32_DIBPRegClassID
    (uint32_t(X86::GPRRegBankID) << 4) | // GR64_with_sub_32bit_in_GR32_SIDIRegClassID
    (uint32_t(X86::GPRRegBankID) << 6) | // GR64_ArgRef_and_GR64_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 8) | // GR64_and_LOW32_ADDR_ACCESSRegClassID
    (uint32_t(X86::GPRRegBankID) << 10) | // GR64_with_sub_32bit_in_GR32_ABCD_and_GR32_BSIRegClassID
    (uint32_t(X86::GPRRegBankID) << 12) | // GR64_with_sub_32bit_in_GR32_AD_and_GR32_ArgRefRegClassID
    (uint32_t(X86::GPRRegBankID) << 14) | // GR64_with_sub_32bit_in_GR32_ArgRef_and_GR32_CBRegClassID
    (uint32_t(X86::GPRRegBankID) << 16) | // GR64_with_sub_32bit_in_GR32_BPSP_and_GR32_DIBPRegClassID
    (uint32_t(X86::GPRRegBankID) << 18) | // GR64_with_sub_32bit_in_GR32_BPSP_and_GR32_TCRegClassID
    (uint32_t(X86::GPRRegBankID) << 20) | // GR64_with_sub_32bit_in_GR32_BSI_and_GR32_SIDIRegClassID
    (uint32_t(X86::GPRRegBankID) << 22) | // GR64_with_sub_32bit_in_GR32_DIBP_and_GR32_SIDIRegClassID
    (uint32_t(InvalidRegBankID) << 24) |
    (uint32_t(X86::PSRRegBankID) << 26) | // RFP80RegClassID
    (uint32_t(InvalidRegBankID) << 28) |
    (uint32_t(X86::VECRRegBankID) << 30), // VR128XRegClassID
    (uint32_t(X86::VECRRegBankID) << 0) | // VR128RegClassID
    (uint32_t(X86::VECRRegBankID) << 2) | // VR256XRegClassID
    (uint32_t(X86::VECRRegBankID) << 4) | // VR256RegClassID
    (uint32_t(X86::VECRRegBankID) << 6) | // VR512RegClassID
    (uint32_t(X86::VECRRegBankID) << 8) // VR512_0_15RegClassID
  };
  const unsigned RegClassID = RC.getID();
  if (LLVM_LIKELY(RegClassID < 133)) {
    unsigned RegBankID = (RegClass2RegBank[RegClassID / 16] >> ((RegClassID % 16) * 2)) & 3;
    if (RegBankID != InvalidRegBankID)
      return getRegBank(RegBankID);
  }
  llvm_unreachable(llvm::Twine("Target needs to handle register class ID 0x").concat(llvm::Twine::utohexstr(RegClassID)).str().c_str());
}
} // end namespace llvm
#endif // GET_TARGET_REGBANK_IMPL
