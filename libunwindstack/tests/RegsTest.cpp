/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#include <gtest/gtest.h>

#include <unwindstack/Elf.h>
#include <unwindstack/ElfInterface.h>
#include <unwindstack/MapInfo.h>
#include <unwindstack/Regs.h>

#include "ElfFake.h"
#include "MemoryFake.h"
#include "RegsFake.h"

namespace unwindstack {

class RegsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    memory_ = new MemoryFake;
    elf_.reset(new ElfFake(memory_));
    elf_interface_ = new ElfInterfaceFake(elf_->memory());
    elf_->FakeSetInterface(elf_interface_);
  }

  ElfInterfaceFake* elf_interface_;
  MemoryFake* memory_;
  std::unique_ptr<ElfFake> elf_;
};

TEST_F(RegsTest, regs32) {
  RegsImplFake<uint32_t> regs32(50, 10);
  ASSERT_EQ(50U, regs32.total_regs());
  ASSERT_EQ(10U, regs32.sp_reg());

  uint32_t* raw = reinterpret_cast<uint32_t*>(regs32.RawData());
  for (size_t i = 0; i < 50; i++) {
    raw[i] = 0xf0000000 + i;
  }
  regs32.set_pc(0xf0120340);
  regs32.set_sp(0xa0ab0cd0);

  for (size_t i = 0; i < 50; i++) {
    ASSERT_EQ(0xf0000000U + i, regs32[i]) << "Failed comparing register " << i;
  }

  ASSERT_EQ(0xf0120340U, regs32.pc());
  ASSERT_EQ(0xa0ab0cd0U, regs32.sp());

  regs32[32] = 10;
  ASSERT_EQ(10U, regs32[32]);
}

TEST_F(RegsTest, regs64) {
  RegsImplFake<uint64_t> regs64(30, 12);
  ASSERT_EQ(30U, regs64.total_regs());
  ASSERT_EQ(12U, regs64.sp_reg());

  uint64_t* raw = reinterpret_cast<uint64_t*>(regs64.RawData());
  for (size_t i = 0; i < 30; i++) {
    raw[i] = 0xf123456780000000UL + i;
  }
  regs64.set_pc(0xf123456780102030UL);
  regs64.set_sp(0xa123456780a0b0c0UL);

  for (size_t i = 0; i < 30; i++) {
    ASSERT_EQ(0xf123456780000000U + i, regs64[i]) << "Failed reading register " << i;
  }

  ASSERT_EQ(0xf123456780102030UL, regs64.pc());
  ASSERT_EQ(0xa123456780a0b0c0UL, regs64.sp());

  regs64[8] = 10;
  ASSERT_EQ(10U, regs64[8]);
}

TEST_F(RegsTest, rel_pc) {
  RegsArm64 arm64;
  ASSERT_EQ(0xcU, arm64.GetAdjustedPc(0x10, elf_.get()));
  ASSERT_EQ(0x0U, arm64.GetAdjustedPc(0x4, elf_.get()));
  ASSERT_EQ(0x3U, arm64.GetAdjustedPc(0x3, elf_.get()));
  ASSERT_EQ(0x2U, arm64.GetAdjustedPc(0x2, elf_.get()));
  ASSERT_EQ(0x1U, arm64.GetAdjustedPc(0x1, elf_.get()));
  ASSERT_EQ(0x0U, arm64.GetAdjustedPc(0x0, elf_.get()));

  RegsX86 x86;
  ASSERT_EQ(0xffU,  x86.GetAdjustedPc(0x100, elf_.get()));
  ASSERT_EQ(0x1U,  x86.GetAdjustedPc(0x2, elf_.get()));
  ASSERT_EQ(0x0U,  x86.GetAdjustedPc(0x1, elf_.get()));
  ASSERT_EQ(0x0U,  x86.GetAdjustedPc(0x0, elf_.get()));

  RegsX86_64 x86_64;
  ASSERT_EQ(0xffU,  x86_64.GetAdjustedPc(0x100, elf_.get()));
  ASSERT_EQ(0x1U,  x86_64.GetAdjustedPc(0x2, elf_.get()));
  ASSERT_EQ(0x0U,  x86_64.GetAdjustedPc(0x1, elf_.get()));
  ASSERT_EQ(0x0U,  x86_64.GetAdjustedPc(0x0, elf_.get()));
}

TEST_F(RegsTest, rel_pc_arm) {
  RegsArm arm;

  // Check fence posts.
  elf_->FakeSetLoadBias(0);
  ASSERT_EQ(3U,  arm.GetAdjustedPc(0x5, elf_.get()));
  ASSERT_EQ(4U,  arm.GetAdjustedPc(0x4, elf_.get()));
  ASSERT_EQ(3U,  arm.GetAdjustedPc(0x3, elf_.get()));
  ASSERT_EQ(2U,  arm.GetAdjustedPc(0x2, elf_.get()));
  ASSERT_EQ(1U,  arm.GetAdjustedPc(0x1, elf_.get()));
  ASSERT_EQ(0U,  arm.GetAdjustedPc(0x0, elf_.get()));

  elf_->FakeSetLoadBias(0x100);
  ASSERT_EQ(0xffU,  arm.GetAdjustedPc(0xff, elf_.get()));
  ASSERT_EQ(0x103U,  arm.GetAdjustedPc(0x105, elf_.get()));
  ASSERT_EQ(0x104U,  arm.GetAdjustedPc(0x104, elf_.get()));
  ASSERT_EQ(0x103U,  arm.GetAdjustedPc(0x103, elf_.get()));
  ASSERT_EQ(0x102U,  arm.GetAdjustedPc(0x102, elf_.get()));
  ASSERT_EQ(0x101U,  arm.GetAdjustedPc(0x101, elf_.get()));
  ASSERT_EQ(0x100U,  arm.GetAdjustedPc(0x100, elf_.get()));

  // Check thumb instructions handling.
  elf_->FakeSetLoadBias(0);
  memory_->SetData32(0x2000, 0);
  ASSERT_EQ(0x2003U,  arm.GetAdjustedPc(0x2005, elf_.get()));
  memory_->SetData32(0x2000, 0xe000f000);
  ASSERT_EQ(0x2001U,  arm.GetAdjustedPc(0x2005, elf_.get()));

  elf_->FakeSetLoadBias(0x400);
  memory_->SetData32(0x2100, 0);
  ASSERT_EQ(0x2503U,  arm.GetAdjustedPc(0x2505, elf_.get()));
  memory_->SetData32(0x2100, 0xf111f111);
  ASSERT_EQ(0x2501U,  arm.GetAdjustedPc(0x2505, elf_.get()));
}

TEST_F(RegsTest, elf_invalid) {
  Elf invalid_elf(new MemoryFake);
  RegsArm regs_arm;
  RegsArm64 regs_arm64;
  RegsX86 regs_x86;
  RegsX86_64 regs_x86_64;
  MapInfo map_info{.start = 0x1000, .end = 0x2000};

  regs_arm.set_pc(0x1500);
  ASSERT_EQ(0x500U, invalid_elf.GetRelPc(regs_arm.pc(), &map_info));
  ASSERT_EQ(0x500U, regs_arm.GetAdjustedPc(0x500U, &invalid_elf));

  regs_arm64.set_pc(0x1600);
  ASSERT_EQ(0x600U, invalid_elf.GetRelPc(regs_arm64.pc(), &map_info));
  ASSERT_EQ(0x600U, regs_arm64.GetAdjustedPc(0x600U, &invalid_elf));

  regs_x86.set_pc(0x1700);
  ASSERT_EQ(0x700U, invalid_elf.GetRelPc(regs_x86.pc(), &map_info));
  ASSERT_EQ(0x700U, regs_x86.GetAdjustedPc(0x700U, &invalid_elf));

  regs_x86_64.set_pc(0x1800);
  ASSERT_EQ(0x800U, invalid_elf.GetRelPc(regs_x86_64.pc(), &map_info));
  ASSERT_EQ(0x800U, regs_x86_64.GetAdjustedPc(0x800U, &invalid_elf));
}

TEST_F(RegsTest, arm_set_from_raw) {
  RegsArm arm;
  uint32_t* regs = reinterpret_cast<uint32_t*>(arm.RawData());
  regs[13] = 0x100;
  regs[15] = 0x200;
  arm.SetFromRaw();
  EXPECT_EQ(0x100U, arm.sp());
  EXPECT_EQ(0x200U, arm.pc());
}

TEST_F(RegsTest, arm64_set_from_raw) {
  RegsArm64 arm64;
  uint64_t* regs = reinterpret_cast<uint64_t*>(arm64.RawData());
  regs[31] = 0xb100000000ULL;
  regs[32] = 0xc200000000ULL;
  arm64.SetFromRaw();
  EXPECT_EQ(0xb100000000U, arm64.sp());
  EXPECT_EQ(0xc200000000U, arm64.pc());
}

TEST_F(RegsTest, x86_set_from_raw) {
  RegsX86 x86;
  uint32_t* regs = reinterpret_cast<uint32_t*>(x86.RawData());
  regs[4] = 0x23450000;
  regs[8] = 0xabcd0000;
  x86.SetFromRaw();
  EXPECT_EQ(0x23450000U, x86.sp());
  EXPECT_EQ(0xabcd0000U, x86.pc());
}

TEST_F(RegsTest, x86_64_set_from_raw) {
  RegsX86_64 x86_64;
  uint64_t* regs = reinterpret_cast<uint64_t*>(x86_64.RawData());
  regs[7] = 0x1200000000ULL;
  regs[16] = 0x4900000000ULL;
  x86_64.SetFromRaw();
  EXPECT_EQ(0x1200000000U, x86_64.sp());
  EXPECT_EQ(0x4900000000U, x86_64.pc());
}

}  // namespace unwindstack
