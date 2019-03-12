//
// Copyright 2019 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <cmath>
#include <iterator>
#include <string>
#include <vector>

#include "gtest/gtest.h"

// Write() functions must be declared here before they can be used by
// ExpectWrite (defined in write_test_utils.h below).
#include "wasp/binary/write/write_block_type.h"
#include "wasp/binary/write/write_br_table_immediate.h"
#include "wasp/binary/write/write_bytes.h"
#include "wasp/binary/write/write_call_indirect_immediate.h"
#include "wasp/binary/write/write_constant_expression.h"
#include "wasp/binary/write/write_copy_immediate.h"
#include "wasp/binary/write/write_data_segment.h"
#include "wasp/binary/write/write_element_expression.h"
#include "wasp/binary/write/write_element_segment.h"
#include "wasp/binary/write/write_element_type.h"
#include "wasp/binary/write/write_export.h"
#include "wasp/binary/write/write_external_kind.h"
#include "wasp/binary/write/write_f32.h"
#include "wasp/binary/write/write_f64.h"
#include "wasp/binary/write/write_fixed_var_int.h"
#include "wasp/binary/write/write_function.h"
#include "wasp/binary/write/write_function_type.h"
#include "wasp/binary/write/write_global.h"
#include "wasp/binary/write/write_global_type.h"
#include "wasp/binary/write/write_import.h"
#include "wasp/binary/write/write_init_immediate.h"
#include "wasp/binary/write/write_instruction.h"
#include "wasp/binary/write/write_mem_arg_immediate.h"
#include "wasp/binary/write/write_memory.h"
#include "wasp/binary/write/write_memory_type.h"
#include "wasp/binary/write/write_mutability.h"
#include "wasp/binary/write/write_name_subsection_id.h"
#include "wasp/binary/write/write_opcode.h"
#include "wasp/binary/write/write_s32.h"
#include "wasp/binary/write/write_s64.h"
#include "wasp/binary/write/write_section_id.h"
#include "wasp/binary/write/write_shuffle_immediate.h"
#include "wasp/binary/write/write_start.h"
#include "wasp/binary/write/write_string.h"
#include "wasp/binary/write/write_table.h"
#include "wasp/binary/write/write_table_type.h"
#include "wasp/binary/write/write_type_entry.h"
#include "wasp/binary/write/write_u32.h"
#include "wasp/binary/write/write_u8.h"
#include "wasp/binary/write/write_value_type.h"
#include "wasp/binary/write/write_vector.h"

#include "src/binary/test_utils.h"
#include "src/binary/write_test_utils.h"

using namespace ::wasp;
using namespace ::wasp::binary;
using namespace ::wasp::binary::test;

TEST(WriteTest, BlockType) {
  ExpectWrite<BlockType>(MakeSpanU8("\x7f"), BlockType::I32);
  ExpectWrite<BlockType>(MakeSpanU8("\x7e"), BlockType::I64);
  ExpectWrite<BlockType>(MakeSpanU8("\x7d"), BlockType::F32);
  ExpectWrite<BlockType>(MakeSpanU8("\x7c"), BlockType::F64);
  ExpectWrite<BlockType>(MakeSpanU8("\x40"), BlockType::Void);
}

TEST(WriteTest, BrTableImmediate) {
  ExpectWrite<BrTableImmediate>(MakeSpanU8("\x00\x00"),
                                BrTableImmediate{{}, 0});
  ExpectWrite<BrTableImmediate>(MakeSpanU8("\x02\x01\x02\x03"),
                                BrTableImmediate{{1, 2}, 3});
}

TEST(WriteTest, Bytes) {
  const std::vector<u8> input{{0x12, 0x34, 0x56}};
  std::vector<u8> output;
  WriteBytes(input, std::back_inserter(output));
  EXPECT_EQ(input, output);
}

TEST(WriteTest, CallIndirectImmediate) {
  ExpectWrite<CallIndirectImmediate>(MakeSpanU8("\x01\x00"),
                                     CallIndirectImmediate{1, 0});
  ExpectWrite<CallIndirectImmediate>(MakeSpanU8("\x80\x01\x00"),
                                     CallIndirectImmediate{128, 0});
}

TEST(WriteTest, ConstantExpression) {
  // i32.const
  ExpectWrite<ConstantExpression>(
      MakeSpanU8("\x41\x00\x0b"),
      ConstantExpression{Instruction{Opcode::I32Const, s32{0}}});

  // i64.const
  ExpectWrite<ConstantExpression>(
      MakeSpanU8("\x42\x80\x80\x80\x80\x80\x01\x0b"),
      ConstantExpression{Instruction{Opcode::I64Const, s64{34359738368}}});

  // f32.const
  ExpectWrite<ConstantExpression>(
      MakeSpanU8("\x43\x00\x00\x00\x00\x0b"),
      ConstantExpression{Instruction{Opcode::F32Const, f32{0}}});

  // f64.const
  ExpectWrite<ConstantExpression>(
      MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b"),
      ConstantExpression{Instruction{Opcode::F64Const, f64{0}}});

  // global.get
  ExpectWrite<ConstantExpression>(
      MakeSpanU8("\x23\x00\x0b"),
      ConstantExpression{Instruction{Opcode::GlobalGet, Index{0}}});
}

TEST(WriteTest, CopyImmediate) {
  ExpectWrite<CopyImmediate>(MakeSpanU8("\x00\x00"), CopyImmediate{0, 0});
}

TEST(WriteTest, DataSegment) {
  ExpectWrite<DataSegment>(
      MakeSpanU8("\x00\x42\x01\x0b\x04wxyz"),
      DataSegment{0, ConstantExpression{Instruction{Opcode::I64Const, s64{1}}},
                  MakeSpanU8("wxyz")});
}

TEST(WriteTest, DataSegment_BulkMemory) {
  // Active data segment with non-zero memory index.
  ExpectWrite<DataSegment>(
      MakeSpanU8("\x02\x01\x42\x01\x0b\x04wxyz"),
      DataSegment{1, ConstantExpression{Instruction{Opcode::I64Const, s64{1}}},
                  MakeSpanU8("wxyz")});

  // Passive data segment.
  ExpectWrite<DataSegment>(MakeSpanU8("\x01\x04wxyz"),
                           DataSegment{MakeSpanU8("wxyz")});
}

TEST(WriteTest, ElementExpression) {
  // ref.null
  ExpectWrite<ElementExpression>(
      MakeSpanU8("\xd0\x0b"), ElementExpression{Instruction{Opcode::RefNull}});

  // ref.func 2
  ExpectWrite<ElementExpression>(
      MakeSpanU8("\xd2\x02\x0b"),
      ElementExpression{Instruction{Opcode::RefFunc, Index{2u}}});
}

TEST(WriteTest, ElementSegment) {
  ExpectWrite<ElementSegment>(
      MakeSpanU8("\x00\x41\x01\x0b\x03\x01\x02\x03"),
      ElementSegment{0,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{1}}},
                     {1, 2, 3}});
}

TEST(WriteTest, ElementSegment_BulkMemory) {
  // Active element segment with non-zero table index.
  ExpectWrite<ElementSegment>(
      MakeSpanU8("\x02\x01\x41\x01\x0b\x03\x01\x02\x03"),
      ElementSegment{1,
                     ConstantExpression{Instruction{Opcode::I32Const, s32{1}}},
                     {1, 2, 3}});

  // Passive element segment.
  ExpectWrite<ElementSegment>(
      MakeSpanU8("\x01\x70\x02\xd2\x01\x0b\xd0\x0b"),
      ElementSegment{
          ElementType::Funcref,
          {ElementExpression{Instruction{Opcode::RefFunc, Index{1u}}},
           ElementExpression{Instruction{Opcode::RefNull}}}});
}

TEST(WriteTest, ElementType) {
  ExpectWrite<ElementType>( MakeSpanU8("\x70"), ElementType::Funcref);
}

TEST(WriteTest, Export) {
  ExpectWrite<Export>(MakeSpanU8("\x02hi\x00\x03"),
                      Export{ExternalKind::Function, "hi", 3});
  ExpectWrite<Export>(MakeSpanU8("\x00\x01\xe8\x07"),
                      Export{ExternalKind::Table, "", 1000});
  ExpectWrite<Export>(MakeSpanU8("\x03mem\x02\x00"),
                      Export{ExternalKind::Memory, "mem", 0});
  ExpectWrite<Export>(MakeSpanU8("\x01g\x03\x01"),
                      Export{ExternalKind::Global, "g", 1});
}

TEST(WriteTest, ExternalKind) {
  ExpectWrite<ExternalKind>(MakeSpanU8("\x00"), ExternalKind::Function);
  ExpectWrite<ExternalKind>(MakeSpanU8("\x01"), ExternalKind::Table);
  ExpectWrite<ExternalKind>(MakeSpanU8("\x02"), ExternalKind::Memory);
  ExpectWrite<ExternalKind>(MakeSpanU8("\x03"), ExternalKind::Global);
}

TEST(WriteTest, F32) {
  ExpectWrite<f32>(MakeSpanU8("\x00\x00\x00\x00"), 0.0f);
  ExpectWrite<f32>(MakeSpanU8("\x00\x00\x80\xbf"), -1.0f);
  ExpectWrite<f32>(MakeSpanU8("\x38\xb4\x96\x49"), 1234567.0f);
  ExpectWrite<f32>(MakeSpanU8("\x00\x00\x80\x7f"), INFINITY);
  ExpectWrite<f32>(MakeSpanU8("\x00\x00\x80\xff"), -INFINITY);
  // TODO: NaN
}

TEST(WriteTest, F64) {
  ExpectWrite<f64>(MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00\x00"), 0.0);
  ExpectWrite<f64>(MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xbf"), -1.0);
  ExpectWrite<f64>(MakeSpanU8("\xc0\x71\xbc\x93\x84\x43\xd9\x42"),
                   111111111111111);
  ExpectWrite<f64>(MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\x7f"), INFINITY);
  ExpectWrite<f64>(MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xff"), -INFINITY);
  // TODO: NaN
}

namespace {

template <typename T>
void ExpectWriteFixedVarInt(SpanU8 expected, T value, size_t length) {
  std::vector<wasp::u8> result(expected.size());
  auto iter = wasp::binary::WriteFixedVarInt(
      value, MakeClampedIterator(result.begin(), result.end()), length);
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), result.end());
  EXPECT_EQ(expected, SpanU8{result});
}

}  // namespace

TEST(WriteTest, FixedVarInt_u32) {
  // Naturally 1 byte.
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x11"), 0x11, 1);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x00"), 0x11, 2);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x80\x00"), 0x11, 3);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x80\x80\x00"), 0x11, 4);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x80\x80\x80\x00"), 0x11, 5);

  // Naturally 2 bytes.
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x02"), 0x111, 2);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x82\x00"), 0x111, 3);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x82\x80\x00"), 0x111, 4);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\x82\x80\x80\x00"), 0x111, 5);

  // Naturally 3 bytes.
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\xa2\x04"), 0x11111, 3);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\xa2\x84\x00"), 0x11111, 4);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\xa2\x84\x80\x00"), 0x11111, 5);

  // Naturally 4 bytes.
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\xa2\xc4\x08"), 0x1111111, 4);
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\xa2\xc4\x88\x00"), 0x1111111, 5);

  // Naturally 5 bytes.
  ExpectWriteFixedVarInt<u32>(MakeSpanU8("\x91\xa2\xc4\x88\x01"), 0x11111111,
                              5);
}

TEST(WriteTest, FixedVarInt_s32) {
  // Naturally 1 byte, positive.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x11"), 0x11, 1);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x00"), 0x11, 2);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x80\x00"), 0x11, 3);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x80\x80\x00"), 0x11, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x80\x80\x80\x00"), 0x11, 5);

  // Naturally 2 bytes, positive.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x02"), 0x111, 2);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x82\x00"), 0x111, 3);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x82\x80\x00"), 0x111, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\x82\x80\x80\x00"), 0x111, 5);

  // Naturally 3 bytes, positive.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\xa2\x04"), 0x11111, 3);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\xa2\x84\x00"), 0x11111, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\xa2\x84\x80\x00"), 0x11111, 5);

  // Naturally 4 bytes, positive.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\xa2\xc4\x08"), 0x1111111, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\xa2\xc4\x88\x00"), 0x1111111, 5);

  // Naturally 5 bytes, positive.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x91\xa2\xc4\x88\x01"), 0x11111111,
                              5);

  // Naturally 1 byte, negative.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\x6f"), -0x11, 1);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\x7f"), -0x11, 2);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xff\x7f"), -0x11, 3);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xff\xff\x7f"), -0x11, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xff\xff\xff\x7f"), -0x11, 5);

  // Naturally 2 bytes, negative.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\x7d"), -0x111, 2);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xfd\x7f"), -0x111, 3);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xfd\xff\x7f"), -0x111, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xfd\xff\xff\x7f"), -0x111, 5);

  // Naturally 3 bytes, negative.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xdd\x7b"), -0x11111, 3);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xdd\xfb\x7f"), -0x11111, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xdd\xfb\xff\x7f"), -0x11111, 5);

  // Naturally 4 bytes, negative.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xdd\xbb\x77"), -0x1111111, 4);
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xdd\xbb\xf7\x7f"), -0x1111111,
                              5);

  // Naturally 5 bytes, negative.
  ExpectWriteFixedVarInt<s32>(MakeSpanU8("\xef\xdd\xbb\xf7\x7e"), -0x11111111,
                              5);
}

TEST(WriteTest, Function) {
  ExpectWrite<Function>(MakeSpanU8("\x01"), Function{1});
}

TEST(WriteTest, FunctionType) {
  ExpectWrite<FunctionType>(MakeSpanU8("\x00\x00"), FunctionType{{}, {}});
  ExpectWrite<FunctionType>(
      MakeSpanU8("\x02\x7f\x7e\x01\x7c"),
      FunctionType{{ValueType::I32, ValueType::I64}, {ValueType::F64}});
}

TEST(WriteTest, Global) {
  ExpectWrite<Global>(
      MakeSpanU8("\x7f\x01\x41\x00\x0b"),
      Global{GlobalType{ValueType::I32, Mutability::Var},
             ConstantExpression{Instruction{Opcode::I32Const, s32{0}}}});
}

TEST(WriteTest, GlobalType) {
  ExpectWrite<GlobalType>(MakeSpanU8("\x7f\x00"),
                          GlobalType{ValueType::I32, Mutability::Const});
  ExpectWrite<GlobalType>(MakeSpanU8("\x7d\x01"),
                          GlobalType{ValueType::F32, Mutability::Var});
}

TEST(WriteTest, Import) {
  ExpectWrite<Import>(MakeSpanU8("\x01\x61\x04\x66unc\x00\x0b"),
                      Import{"a", "func", 11u});

  ExpectWrite<Import>(
      MakeSpanU8("\x01\x62\x05table\x01\x70\x00\x01"),
      Import{"b", "table", TableType{Limits{1}, ElementType::Funcref}});

  ExpectWrite<Import>(MakeSpanU8("\x01\x63\x06memory\x02\x01\x00\x02"),
                      Import{"c", "memory", MemoryType{Limits{0, 2}}});

  ExpectWrite<Import>(
      MakeSpanU8("\x01\x64\x06global\x03\x7f\x00"),
      Import{"d", "global", GlobalType{ValueType::I32, Mutability::Const}});
}

TEST(WriteTest, InitImmediate) {
  ExpectWrite<InitImmediate>(MakeSpanU8("\x01\x00"), InitImmediate{1, 0});
  ExpectWrite<InitImmediate>(MakeSpanU8("\x80\x01\x00"), InitImmediate{128, 0});
}

TEST(WriteTest, Instruction) {
  using I = Instruction;
  using O = Opcode;
  using MemArg = MemArgImmediate;

  ExpectWrite<I>(MakeSpanU8("\x00"), I{O::Unreachable});
  ExpectWrite<I>(MakeSpanU8("\x01"), I{O::Nop});
  ExpectWrite<I>(MakeSpanU8("\x02\x7f"), I{O::Block, BlockType::I32});
  ExpectWrite<I>(MakeSpanU8("\x03\x40"), I{O::Loop, BlockType::Void});
  ExpectWrite<I>(MakeSpanU8("\x04\x7c"), I{O::If, BlockType::F64});
  ExpectWrite<I>(MakeSpanU8("\x05"), I{O::Else});
  ExpectWrite<I>(MakeSpanU8("\x0b"), I{O::End});
  ExpectWrite<I>(MakeSpanU8("\x0c\x01"), I{O::Br, Index{1}});
  ExpectWrite<I>(MakeSpanU8("\x0d\x02"), I{O::BrIf, Index{2}});
  ExpectWrite<I>(MakeSpanU8("\x0e\x03\x03\x04\x05\x06"),
                 I{O::BrTable, BrTableImmediate{{3, 4, 5}, 6}});
  ExpectWrite<I>(MakeSpanU8("\x0f"), I{O::Return});
  ExpectWrite<I>(MakeSpanU8("\x10\x07"), I{O::Call, Index{7}});
  ExpectWrite<I>(MakeSpanU8("\x11\x08\x00"),
                 I{O::CallIndirect, CallIndirectImmediate{8, 0}});
  ExpectWrite<I>(MakeSpanU8("\x1a"), I{O::Drop});
  ExpectWrite<I>(MakeSpanU8("\x1b"), I{O::Select});
  ExpectWrite<I>(MakeSpanU8("\x20\x05"), I{O::LocalGet, Index{5}});
  ExpectWrite<I>(MakeSpanU8("\x21\x06"), I{O::LocalSet, Index{6}});
  ExpectWrite<I>(MakeSpanU8("\x22\x07"), I{O::LocalTee, Index{7}});
  ExpectWrite<I>(MakeSpanU8("\x23\x08"), I{O::GlobalGet, Index{8}});
  ExpectWrite<I>(MakeSpanU8("\x24\x09"), I{O::GlobalSet, Index{9}});
  ExpectWrite<I>(MakeSpanU8("\x28\x0a\x0b"), I{O::I32Load, MemArg{10, 11}});
  ExpectWrite<I>(MakeSpanU8("\x29\x0c\x0d"), I{O::I64Load, MemArg{12, 13}});
  ExpectWrite<I>(MakeSpanU8("\x2a\x0e\x0f"), I{O::F32Load, MemArg{14, 15}});
  ExpectWrite<I>(MakeSpanU8("\x2b\x10\x11"), I{O::F64Load, MemArg{16, 17}});
  ExpectWrite<I>(MakeSpanU8("\x2c\x12\x13"), I{O::I32Load8S, MemArg{18, 19}});
  ExpectWrite<I>(MakeSpanU8("\x2d\x14\x15"), I{O::I32Load8U, MemArg{20, 21}});
  ExpectWrite<I>(MakeSpanU8("\x2e\x16\x17"), I{O::I32Load16S, MemArg{22, 23}});
  ExpectWrite<I>(MakeSpanU8("\x2f\x18\x19"), I{O::I32Load16U, MemArg{24, 25}});
  ExpectWrite<I>(MakeSpanU8("\x30\x1a\x1b"), I{O::I64Load8S, MemArg{26, 27}});
  ExpectWrite<I>(MakeSpanU8("\x31\x1c\x1d"), I{O::I64Load8U, MemArg{28, 29}});
  ExpectWrite<I>(MakeSpanU8("\x32\x1e\x1f"), I{O::I64Load16S, MemArg{30, 31}});
  ExpectWrite<I>(MakeSpanU8("\x33\x20\x21"), I{O::I64Load16U, MemArg{32, 33}});
  ExpectWrite<I>(MakeSpanU8("\x34\x22\x23"), I{O::I64Load32S, MemArg{34, 35}});
  ExpectWrite<I>(MakeSpanU8("\x35\x24\x25"), I{O::I64Load32U, MemArg{36, 37}});
  ExpectWrite<I>(MakeSpanU8("\x36\x26\x27"), I{O::I32Store, MemArg{38, 39}});
  ExpectWrite<I>(MakeSpanU8("\x37\x28\x29"), I{O::I64Store, MemArg{40, 41}});
  ExpectWrite<I>(MakeSpanU8("\x38\x2a\x2b"), I{O::F32Store, MemArg{42, 43}});
  ExpectWrite<I>(MakeSpanU8("\x39\x2c\x2d"), I{O::F64Store, MemArg{44, 45}});
  ExpectWrite<I>(MakeSpanU8("\x3a\x2e\x2f"), I{O::I32Store8, MemArg{46, 47}});
  ExpectWrite<I>(MakeSpanU8("\x3b\x30\x31"), I{O::I32Store16, MemArg{48, 49}});
  ExpectWrite<I>(MakeSpanU8("\x3c\x32\x33"), I{O::I64Store8, MemArg{50, 51}});
  ExpectWrite<I>(MakeSpanU8("\x3d\x34\x35"), I{O::I64Store16, MemArg{52, 53}});
  ExpectWrite<I>(MakeSpanU8("\x3e\x36\x37"), I{O::I64Store32, MemArg{54, 55}});
  ExpectWrite<I>(MakeSpanU8("\x3f\x00"), I{O::MemorySize, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\x40\x00"), I{O::MemoryGrow, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\x41\x00"), I{O::I32Const, s32{0}});
  ExpectWrite<I>(MakeSpanU8("\x42\x00"), I{O::I64Const, s64{0}});
  ExpectWrite<I>(MakeSpanU8("\x43\x00\x00\x00\x00"), I{O::F32Const, f32{0}});
  ExpectWrite<I>(MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00"),
                 I{O::F64Const, f64{0}});
  ExpectWrite<I>(MakeSpanU8("\x45"), I{O::I32Eqz});
  ExpectWrite<I>(MakeSpanU8("\x46"), I{O::I32Eq});
  ExpectWrite<I>(MakeSpanU8("\x47"), I{O::I32Ne});
  ExpectWrite<I>(MakeSpanU8("\x48"), I{O::I32LtS});
  ExpectWrite<I>(MakeSpanU8("\x49"), I{O::I32LtU});
  ExpectWrite<I>(MakeSpanU8("\x4a"), I{O::I32GtS});
  ExpectWrite<I>(MakeSpanU8("\x4b"), I{O::I32GtU});
  ExpectWrite<I>(MakeSpanU8("\x4c"), I{O::I32LeS});
  ExpectWrite<I>(MakeSpanU8("\x4d"), I{O::I32LeU});
  ExpectWrite<I>(MakeSpanU8("\x4e"), I{O::I32GeS});
  ExpectWrite<I>(MakeSpanU8("\x4f"), I{O::I32GeU});
  ExpectWrite<I>(MakeSpanU8("\x50"), I{O::I64Eqz});
  ExpectWrite<I>(MakeSpanU8("\x51"), I{O::I64Eq});
  ExpectWrite<I>(MakeSpanU8("\x52"), I{O::I64Ne});
  ExpectWrite<I>(MakeSpanU8("\x53"), I{O::I64LtS});
  ExpectWrite<I>(MakeSpanU8("\x54"), I{O::I64LtU});
  ExpectWrite<I>(MakeSpanU8("\x55"), I{O::I64GtS});
  ExpectWrite<I>(MakeSpanU8("\x56"), I{O::I64GtU});
  ExpectWrite<I>(MakeSpanU8("\x57"), I{O::I64LeS});
  ExpectWrite<I>(MakeSpanU8("\x58"), I{O::I64LeU});
  ExpectWrite<I>(MakeSpanU8("\x59"), I{O::I64GeS});
  ExpectWrite<I>(MakeSpanU8("\x5a"), I{O::I64GeU});
  ExpectWrite<I>(MakeSpanU8("\x5b"), I{O::F32Eq});
  ExpectWrite<I>(MakeSpanU8("\x5c"), I{O::F32Ne});
  ExpectWrite<I>(MakeSpanU8("\x5d"), I{O::F32Lt});
  ExpectWrite<I>(MakeSpanU8("\x5e"), I{O::F32Gt});
  ExpectWrite<I>(MakeSpanU8("\x5f"), I{O::F32Le});
  ExpectWrite<I>(MakeSpanU8("\x60"), I{O::F32Ge});
  ExpectWrite<I>(MakeSpanU8("\x61"), I{O::F64Eq});
  ExpectWrite<I>(MakeSpanU8("\x62"), I{O::F64Ne});
  ExpectWrite<I>(MakeSpanU8("\x63"), I{O::F64Lt});
  ExpectWrite<I>(MakeSpanU8("\x64"), I{O::F64Gt});
  ExpectWrite<I>(MakeSpanU8("\x65"), I{O::F64Le});
  ExpectWrite<I>(MakeSpanU8("\x66"), I{O::F64Ge});
  ExpectWrite<I>(MakeSpanU8("\x67"), I{O::I32Clz});
  ExpectWrite<I>(MakeSpanU8("\x68"), I{O::I32Ctz});
  ExpectWrite<I>(MakeSpanU8("\x69"), I{O::I32Popcnt});
  ExpectWrite<I>(MakeSpanU8("\x6a"), I{O::I32Add});
  ExpectWrite<I>(MakeSpanU8("\x6b"), I{O::I32Sub});
  ExpectWrite<I>(MakeSpanU8("\x6c"), I{O::I32Mul});
  ExpectWrite<I>(MakeSpanU8("\x6d"), I{O::I32DivS});
  ExpectWrite<I>(MakeSpanU8("\x6e"), I{O::I32DivU});
  ExpectWrite<I>(MakeSpanU8("\x6f"), I{O::I32RemS});
  ExpectWrite<I>(MakeSpanU8("\x70"), I{O::I32RemU});
  ExpectWrite<I>(MakeSpanU8("\x71"), I{O::I32And});
  ExpectWrite<I>(MakeSpanU8("\x72"), I{O::I32Or});
  ExpectWrite<I>(MakeSpanU8("\x73"), I{O::I32Xor});
  ExpectWrite<I>(MakeSpanU8("\x74"), I{O::I32Shl});
  ExpectWrite<I>(MakeSpanU8("\x75"), I{O::I32ShrS});
  ExpectWrite<I>(MakeSpanU8("\x76"), I{O::I32ShrU});
  ExpectWrite<I>(MakeSpanU8("\x77"), I{O::I32Rotl});
  ExpectWrite<I>(MakeSpanU8("\x78"), I{O::I32Rotr});
  ExpectWrite<I>(MakeSpanU8("\x79"), I{O::I64Clz});
  ExpectWrite<I>(MakeSpanU8("\x7a"), I{O::I64Ctz});
  ExpectWrite<I>(MakeSpanU8("\x7b"), I{O::I64Popcnt});
  ExpectWrite<I>(MakeSpanU8("\x7c"), I{O::I64Add});
  ExpectWrite<I>(MakeSpanU8("\x7d"), I{O::I64Sub});
  ExpectWrite<I>(MakeSpanU8("\x7e"), I{O::I64Mul});
  ExpectWrite<I>(MakeSpanU8("\x7f"), I{O::I64DivS});
  ExpectWrite<I>(MakeSpanU8("\x80"), I{O::I64DivU});
  ExpectWrite<I>(MakeSpanU8("\x81"), I{O::I64RemS});
  ExpectWrite<I>(MakeSpanU8("\x82"), I{O::I64RemU});
  ExpectWrite<I>(MakeSpanU8("\x83"), I{O::I64And});
  ExpectWrite<I>(MakeSpanU8("\x84"), I{O::I64Or});
  ExpectWrite<I>(MakeSpanU8("\x85"), I{O::I64Xor});
  ExpectWrite<I>(MakeSpanU8("\x86"), I{O::I64Shl});
  ExpectWrite<I>(MakeSpanU8("\x87"), I{O::I64ShrS});
  ExpectWrite<I>(MakeSpanU8("\x88"), I{O::I64ShrU});
  ExpectWrite<I>(MakeSpanU8("\x89"), I{O::I64Rotl});
  ExpectWrite<I>(MakeSpanU8("\x8a"), I{O::I64Rotr});
  ExpectWrite<I>(MakeSpanU8("\x8b"), I{O::F32Abs});
  ExpectWrite<I>(MakeSpanU8("\x8c"), I{O::F32Neg});
  ExpectWrite<I>(MakeSpanU8("\x8d"), I{O::F32Ceil});
  ExpectWrite<I>(MakeSpanU8("\x8e"), I{O::F32Floor});
  ExpectWrite<I>(MakeSpanU8("\x8f"), I{O::F32Trunc});
  ExpectWrite<I>(MakeSpanU8("\x90"), I{O::F32Nearest});
  ExpectWrite<I>(MakeSpanU8("\x91"), I{O::F32Sqrt});
  ExpectWrite<I>(MakeSpanU8("\x92"), I{O::F32Add});
  ExpectWrite<I>(MakeSpanU8("\x93"), I{O::F32Sub});
  ExpectWrite<I>(MakeSpanU8("\x94"), I{O::F32Mul});
  ExpectWrite<I>(MakeSpanU8("\x95"), I{O::F32Div});
  ExpectWrite<I>(MakeSpanU8("\x96"), I{O::F32Min});
  ExpectWrite<I>(MakeSpanU8("\x97"), I{O::F32Max});
  ExpectWrite<I>(MakeSpanU8("\x98"), I{O::F32Copysign});
  ExpectWrite<I>(MakeSpanU8("\x99"), I{O::F64Abs});
  ExpectWrite<I>(MakeSpanU8("\x9a"), I{O::F64Neg});
  ExpectWrite<I>(MakeSpanU8("\x9b"), I{O::F64Ceil});
  ExpectWrite<I>(MakeSpanU8("\x9c"), I{O::F64Floor});
  ExpectWrite<I>(MakeSpanU8("\x9d"), I{O::F64Trunc});
  ExpectWrite<I>(MakeSpanU8("\x9e"), I{O::F64Nearest});
  ExpectWrite<I>(MakeSpanU8("\x9f"), I{O::F64Sqrt});
  ExpectWrite<I>(MakeSpanU8("\xa0"), I{O::F64Add});
  ExpectWrite<I>(MakeSpanU8("\xa1"), I{O::F64Sub});
  ExpectWrite<I>(MakeSpanU8("\xa2"), I{O::F64Mul});
  ExpectWrite<I>(MakeSpanU8("\xa3"), I{O::F64Div});
  ExpectWrite<I>(MakeSpanU8("\xa4"), I{O::F64Min});
  ExpectWrite<I>(MakeSpanU8("\xa5"), I{O::F64Max});
  ExpectWrite<I>(MakeSpanU8("\xa6"), I{O::F64Copysign});
  ExpectWrite<I>(MakeSpanU8("\xa7"), I{O::I32WrapI64});
  ExpectWrite<I>(MakeSpanU8("\xa8"), I{O::I32TruncF32S});
  ExpectWrite<I>(MakeSpanU8("\xa9"), I{O::I32TruncF32U});
  ExpectWrite<I>(MakeSpanU8("\xaa"), I{O::I32TruncF64S});
  ExpectWrite<I>(MakeSpanU8("\xab"), I{O::I32TruncF64U});
  ExpectWrite<I>(MakeSpanU8("\xac"), I{O::I64ExtendI32S});
  ExpectWrite<I>(MakeSpanU8("\xad"), I{O::I64ExtendI32U});
  ExpectWrite<I>(MakeSpanU8("\xae"), I{O::I64TruncF32S});
  ExpectWrite<I>(MakeSpanU8("\xaf"), I{O::I64TruncF32U});
  ExpectWrite<I>(MakeSpanU8("\xb0"), I{O::I64TruncF64S});
  ExpectWrite<I>(MakeSpanU8("\xb1"), I{O::I64TruncF64U});
  ExpectWrite<I>(MakeSpanU8("\xb2"), I{O::F32ConvertI32S});
  ExpectWrite<I>(MakeSpanU8("\xb3"), I{O::F32ConvertI32U});
  ExpectWrite<I>(MakeSpanU8("\xb4"), I{O::F32ConvertI64S});
  ExpectWrite<I>(MakeSpanU8("\xb5"), I{O::F32ConvertI64U});
  ExpectWrite<I>(MakeSpanU8("\xb6"), I{O::F32DemoteF64});
  ExpectWrite<I>(MakeSpanU8("\xb7"), I{O::F64ConvertI32S});
  ExpectWrite<I>(MakeSpanU8("\xb8"), I{O::F64ConvertI32U});
  ExpectWrite<I>(MakeSpanU8("\xb9"), I{O::F64ConvertI64S});
  ExpectWrite<I>(MakeSpanU8("\xba"), I{O::F64ConvertI64U});
  ExpectWrite<I>(MakeSpanU8("\xbb"), I{O::F64PromoteF32});
  ExpectWrite<I>(MakeSpanU8("\xbc"), I{O::I32ReinterpretF32});
  ExpectWrite<I>(MakeSpanU8("\xbd"), I{O::I64ReinterpretF64});
  ExpectWrite<I>(MakeSpanU8("\xbe"), I{O::F32ReinterpretI32});
  ExpectWrite<I>(MakeSpanU8("\xbf"), I{O::F64ReinterpretI64});
}

TEST(WriteTest, Instruction_tail_call) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>(MakeSpanU8("\x12\x00"), I{O::ReturnCall, Index{0}});
  ExpectWrite<I>(MakeSpanU8("\x13\x08\x00"),
                 I{O::ReturnCallIndirect, CallIndirectImmediate{8, 0}});
}

TEST(WriteTest, Instruction_sign_extension) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>(MakeSpanU8("\xc0"), I{O::I32Extend8S});
  ExpectWrite<I>(MakeSpanU8("\xc1"), I{O::I32Extend16S});
  ExpectWrite<I>(MakeSpanU8("\xc2"), I{O::I64Extend8S});
  ExpectWrite<I>(MakeSpanU8("\xc3"), I{O::I64Extend16S});
  ExpectWrite<I>(MakeSpanU8("\xc4"), I{O::I64Extend32S});
}

TEST(WriteTest, Instruction_reference_types) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>(MakeSpanU8("\xd0"), I{O::RefNull});
  ExpectWrite<I>(MakeSpanU8("\xd1"), I{O::RefIsNull});
}

TEST(WriteTest, Instruction_function_references) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>(MakeSpanU8("\xd2\x00"), I{O::RefFunc, Index{0}});
}

TEST(WriteTest, Instruction_saturating_float_to_int) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>(MakeSpanU8("\xfc\x00"), I{O::I32TruncSatF32S});
  ExpectWrite<I>(MakeSpanU8("\xfc\x01"), I{O::I32TruncSatF32U});
  ExpectWrite<I>(MakeSpanU8("\xfc\x02"), I{O::I32TruncSatF64S});
  ExpectWrite<I>(MakeSpanU8("\xfc\x03"), I{O::I32TruncSatF64U});
  ExpectWrite<I>(MakeSpanU8("\xfc\x04"), I{O::I64TruncSatF32S});
  ExpectWrite<I>(MakeSpanU8("\xfc\x05"), I{O::I64TruncSatF32U});
  ExpectWrite<I>(MakeSpanU8("\xfc\x06"), I{O::I64TruncSatF64S});
  ExpectWrite<I>(MakeSpanU8("\xfc\x07"), I{O::I64TruncSatF64U});
}

TEST(WriteTest, Instruction_bulk_memory) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>(MakeSpanU8("\xfc\x08\x01\x00"),
                 I{O::MemoryInit, InitImmediate{1, 0}});
  ExpectWrite<I>(MakeSpanU8("\xfc\x09\x02"), I{O::DataDrop, Index{2}});
  ExpectWrite<I>(MakeSpanU8("\xfc\x0a\x00\x00"),
                 I{O::MemoryCopy, CopyImmediate{0, 0}});
  ExpectWrite<I>(MakeSpanU8("\xfc\x0b\x00"), I{O::MemoryFill, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfc\x0c\x03\x00"),
                 I{O::TableInit, InitImmediate{3, 0}});
  ExpectWrite<I>(MakeSpanU8("\xfc\x0d\x04"), I{O::ElemDrop, Index{4}});
  ExpectWrite<I>(MakeSpanU8("\xfc\x0e\x00\x00"),
                 I{O::TableCopy, CopyImmediate{0, 0}});
}

TEST(WriteTest, Instruction_simd) {
  using I = Instruction;
  using O = Opcode;

  ExpectWrite<I>(MakeSpanU8("\xfd\x00\x01\x02"),
                 I{O::V128Load, MemArgImmediate{1, 2}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x01\x03\x04"),
                 I{O::V128Store, MemArgImmediate{3, 4}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x02\x05\x00\x00\x00\x00\x00\x00\x00\x06\x00"
                            "\x00\x00\x00\x00\x00\x00"),
                 I{O::V128Const, v128{u64{5}, u64{6}}});
  ExpectWrite<I>(
      MakeSpanU8("\xfd\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                 "\x00\x00\x00\x00"),
      I{O::V8X16Shuffle,
        ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x04"), I{O::I8X16Splat});
  ExpectWrite<I>(MakeSpanU8("\xfd\x05\x00"), I{O::I8X16ExtractLaneS, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x06\x00"), I{O::I8X16ExtractLaneU, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x07\x00"), I{O::I8X16ReplaceLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x08"), I{O::I16X8Splat});
  ExpectWrite<I>(MakeSpanU8("\xfd\x09\x00"), I{O::I16X8ExtractLaneS, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x0a\x00"), I{O::I16X8ExtractLaneU, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x0b\x00"), I{O::I16X8ReplaceLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x0c"), I{O::I32X4Splat});
  ExpectWrite<I>(MakeSpanU8("\xfd\x0d\x00"), I{O::I32X4ExtractLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x0e\x00"), I{O::I32X4ReplaceLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x0f"), I{O::I64X2Splat});
  ExpectWrite<I>(MakeSpanU8("\xfd\x10\x00"), I{O::I64X2ExtractLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x11\x00"), I{O::I64X2ReplaceLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x12"), I{O::F32X4Splat});
  ExpectWrite<I>(MakeSpanU8("\xfd\x13\x00"), I{O::F32X4ExtractLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x14\x00"), I{O::F32X4ReplaceLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x15"), I{O::F64X2Splat});
  ExpectWrite<I>(MakeSpanU8("\xfd\x16\x00"), I{O::F64X2ExtractLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x17\x00"), I{O::F64X2ReplaceLane, u8{0}});
  ExpectWrite<I>(MakeSpanU8("\xfd\x18"), I{O::I8X16Eq});
  ExpectWrite<I>(MakeSpanU8("\xfd\x19"), I{O::I8X16Ne});
  ExpectWrite<I>(MakeSpanU8("\xfd\x1a"), I{O::I8X16LtS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x1b"), I{O::I8X16LtU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x1c"), I{O::I8X16GtS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x1d"), I{O::I8X16GtU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x1e"), I{O::I8X16LeS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x1f"), I{O::I8X16LeU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x20"), I{O::I8X16GeS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x21"), I{O::I8X16GeU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x22"), I{O::I16X8Eq});
  ExpectWrite<I>(MakeSpanU8("\xfd\x23"), I{O::I16X8Ne});
  ExpectWrite<I>(MakeSpanU8("\xfd\x24"), I{O::I16X8LtS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x25"), I{O::I16X8LtU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x26"), I{O::I16X8GtS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x27"), I{O::I16X8GtU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x28"), I{O::I16X8LeS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x29"), I{O::I16X8LeU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x2a"), I{O::I16X8GeS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x2b"), I{O::I16X8GeU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x2c"), I{O::I32X4Eq});
  ExpectWrite<I>(MakeSpanU8("\xfd\x2d"), I{O::I32X4Ne});
  ExpectWrite<I>(MakeSpanU8("\xfd\x2e"), I{O::I32X4LtS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x2f"), I{O::I32X4LtU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x30"), I{O::I32X4GtS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x31"), I{O::I32X4GtU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x32"), I{O::I32X4LeS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x33"), I{O::I32X4LeU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x34"), I{O::I32X4GeS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x35"), I{O::I32X4GeU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x40"), I{O::F32X4Eq});
  ExpectWrite<I>(MakeSpanU8("\xfd\x41"), I{O::F32X4Ne});
  ExpectWrite<I>(MakeSpanU8("\xfd\x42"), I{O::F32X4Lt});
  ExpectWrite<I>(MakeSpanU8("\xfd\x43"), I{O::F32X4Gt});
  ExpectWrite<I>(MakeSpanU8("\xfd\x44"), I{O::F32X4Le});
  ExpectWrite<I>(MakeSpanU8("\xfd\x45"), I{O::F32X4Ge});
  ExpectWrite<I>(MakeSpanU8("\xfd\x46"), I{O::F64X2Eq});
  ExpectWrite<I>(MakeSpanU8("\xfd\x47"), I{O::F64X2Ne});
  ExpectWrite<I>(MakeSpanU8("\xfd\x48"), I{O::F64X2Lt});
  ExpectWrite<I>(MakeSpanU8("\xfd\x49"), I{O::F64X2Gt});
  ExpectWrite<I>(MakeSpanU8("\xfd\x4a"), I{O::F64X2Le});
  ExpectWrite<I>(MakeSpanU8("\xfd\x4b"), I{O::F64X2Ge});
  ExpectWrite<I>(MakeSpanU8("\xfd\x4c"), I{O::V128Not});
  ExpectWrite<I>(MakeSpanU8("\xfd\x4d"), I{O::V128And});
  ExpectWrite<I>(MakeSpanU8("\xfd\x4e"), I{O::V128Or});
  ExpectWrite<I>(MakeSpanU8("\xfd\x4f"), I{O::V128Xor});
  ExpectWrite<I>(MakeSpanU8("\xfd\x50"), I{O::V128BitSelect});
  ExpectWrite<I>(MakeSpanU8("\xfd\x51"), I{O::I8X16Neg});
  ExpectWrite<I>(MakeSpanU8("\xfd\x52"), I{O::I8X16AnyTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x53"), I{O::I8X16AllTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x54"), I{O::I8X16Shl});
  ExpectWrite<I>(MakeSpanU8("\xfd\x55"), I{O::I8X16ShrS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x56"), I{O::I8X16ShrU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x57"), I{O::I8X16Add});
  ExpectWrite<I>(MakeSpanU8("\xfd\x58"), I{O::I8X16AddSaturateS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x59"), I{O::I8X16AddSaturateU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x5a"), I{O::I8X16Sub});
  ExpectWrite<I>(MakeSpanU8("\xfd\x5b"), I{O::I8X16SubSaturateS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x5c"), I{O::I8X16SubSaturateU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x5d"), I{O::I8X16Mul});
  ExpectWrite<I>(MakeSpanU8("\xfd\x62"), I{O::I16X8Neg});
  ExpectWrite<I>(MakeSpanU8("\xfd\x63"), I{O::I16X8AnyTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x64"), I{O::I16X8AllTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x65"), I{O::I16X8Shl});
  ExpectWrite<I>(MakeSpanU8("\xfd\x66"), I{O::I16X8ShrS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x67"), I{O::I16X8ShrU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x68"), I{O::I16X8Add});
  ExpectWrite<I>(MakeSpanU8("\xfd\x69"), I{O::I16X8AddSaturateS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x6a"), I{O::I16X8AddSaturateU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x6b"), I{O::I16X8Sub});
  ExpectWrite<I>(MakeSpanU8("\xfd\x6c"), I{O::I16X8SubSaturateS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x6d"), I{O::I16X8SubSaturateU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x6e"), I{O::I16X8Mul});
  ExpectWrite<I>(MakeSpanU8("\xfd\x73"), I{O::I32X4Neg});
  ExpectWrite<I>(MakeSpanU8("\xfd\x74"), I{O::I32X4AnyTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x75"), I{O::I32X4AllTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x76"), I{O::I32X4Shl});
  ExpectWrite<I>(MakeSpanU8("\xfd\x77"), I{O::I32X4ShrS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x78"), I{O::I32X4ShrU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x79"), I{O::I32X4Add});
  ExpectWrite<I>(MakeSpanU8("\xfd\x7c"), I{O::I32X4Sub});
  ExpectWrite<I>(MakeSpanU8("\xfd\x7f"), I{O::I32X4Mul});
  ExpectWrite<I>(MakeSpanU8("\xfd\x84\x01"), I{O::I64X2Neg});
  ExpectWrite<I>(MakeSpanU8("\xfd\x85\x01"), I{O::I64X2AnyTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x86\x01"), I{O::I64X2AllTrue});
  ExpectWrite<I>(MakeSpanU8("\xfd\x87\x01"), I{O::I64X2Shl});
  ExpectWrite<I>(MakeSpanU8("\xfd\x88\x01"), I{O::I64X2ShrS});
  ExpectWrite<I>(MakeSpanU8("\xfd\x89\x01"), I{O::I64X2ShrU});
  ExpectWrite<I>(MakeSpanU8("\xfd\x8a\x01"), I{O::I64X2Add});
  ExpectWrite<I>(MakeSpanU8("\xfd\x8d\x01"), I{O::I64X2Sub});
  ExpectWrite<I>(MakeSpanU8("\xfd\x95\x01"), I{O::F32X4Abs});
  ExpectWrite<I>(MakeSpanU8("\xfd\x96\x01"), I{O::F32X4Neg});
  ExpectWrite<I>(MakeSpanU8("\xfd\x97\x01"), I{O::F32X4Sqrt});
  ExpectWrite<I>(MakeSpanU8("\xfd\x9a\x01"), I{O::F32X4Add});
  ExpectWrite<I>(MakeSpanU8("\xfd\x9b\x01"), I{O::F32X4Sub});
  ExpectWrite<I>(MakeSpanU8("\xfd\x9c\x01"), I{O::F32X4Mul});
  ExpectWrite<I>(MakeSpanU8("\xfd\x9d\x01"), I{O::F32X4Div});
  ExpectWrite<I>(MakeSpanU8("\xfd\x9e\x01"), I{O::F32X4Min});
  ExpectWrite<I>(MakeSpanU8("\xfd\x9f\x01"), I{O::F32X4Max});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa0\x01"), I{O::F64X2Abs});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa1\x01"), I{O::F64X2Neg});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa2\x01"), I{O::F64X2Sqrt});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa5\x01"), I{O::F64X2Add});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa6\x01"), I{O::F64X2Sub});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa7\x01"), I{O::F64X2Mul});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa8\x01"), I{O::F64X2Div});
  ExpectWrite<I>(MakeSpanU8("\xfd\xa9\x01"), I{O::F64X2Min});
  ExpectWrite<I>(MakeSpanU8("\xfd\xaa\x01"), I{O::F64X2Max});
  ExpectWrite<I>(MakeSpanU8("\xfd\xab\x01"), I{O::I32X4TruncSatF32X4S});
  ExpectWrite<I>(MakeSpanU8("\xfd\xac\x01"), I{O::I32X4TruncSatF32X4U});
  ExpectWrite<I>(MakeSpanU8("\xfd\xad\x01"), I{O::I64X2TruncSatF64X2S});
  ExpectWrite<I>(MakeSpanU8("\xfd\xae\x01"), I{O::I64X2TruncSatF64X2U});
  ExpectWrite<I>(MakeSpanU8("\xfd\xaf\x01"), I{O::F32X4ConvertI32X4S});
  ExpectWrite<I>(MakeSpanU8("\xfd\xb0\x01"), I{O::F32X4ConvertI32X4U});
  ExpectWrite<I>(MakeSpanU8("\xfd\xb1\x01"), I{O::F64X2ConvertI64X2S});
  ExpectWrite<I>(MakeSpanU8("\xfd\xb2\x01"), I{O::F64X2ConvertI64X2U});
}

TEST(WriteTest, Instruction_threads) {
  using I = Instruction;
  using O = Opcode;

  const MemArgImmediate m{0, 0};

  ExpectWrite<I>(MakeSpanU8("\xfe\x00\x00\x00"), I{O::AtomicNotify, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x01\x00\x00"), I{O::I32AtomicWait, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x02\x00\x00"), I{O::I64AtomicWait, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x10\x00\x00"), I{O::I32AtomicLoad, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x11\x00\x00"), I{O::I64AtomicLoad, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x12\x00\x00"), I{O::I32AtomicLoad8U, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x13\x00\x00"), I{O::I32AtomicLoad16U, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x14\x00\x00"), I{O::I64AtomicLoad8U, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x15\x00\x00"), I{O::I64AtomicLoad16U, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x16\x00\x00"), I{O::I64AtomicLoad32U, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x17\x00\x00"), I{O::I32AtomicStore, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x18\x00\x00"), I{O::I64AtomicStore, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x19\x00\x00"), I{O::I32AtomicStore8, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x1a\x00\x00"), I{O::I32AtomicStore16, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x1b\x00\x00"), I{O::I64AtomicStore8, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x1c\x00\x00"), I{O::I64AtomicStore16, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x1d\x00\x00"), I{O::I64AtomicStore32, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x1e\x00\x00"), I{O::I32AtomicRmwAdd, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x1f\x00\x00"), I{O::I64AtomicRmwAdd, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x20\x00\x00"), I{O::I32AtomicRmw8AddU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x21\x00\x00"), I{O::I32AtomicRmw16AddU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x22\x00\x00"), I{O::I64AtomicRmw8AddU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x23\x00\x00"), I{O::I64AtomicRmw16AddU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x24\x00\x00"), I{O::I64AtomicRmw32AddU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x25\x00\x00"), I{O::I32AtomicRmwSub, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x26\x00\x00"), I{O::I64AtomicRmwSub, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x27\x00\x00"), I{O::I32AtomicRmw8SubU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x28\x00\x00"), I{O::I32AtomicRmw16SubU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x29\x00\x00"), I{O::I64AtomicRmw8SubU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x2a\x00\x00"), I{O::I64AtomicRmw16SubU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x2b\x00\x00"), I{O::I64AtomicRmw32SubU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x2c\x00\x00"), I{O::I32AtomicRmwAnd, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x2d\x00\x00"), I{O::I64AtomicRmwAnd, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x2e\x00\x00"), I{O::I32AtomicRmw8AndU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x2f\x00\x00"), I{O::I32AtomicRmw16AndU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x30\x00\x00"), I{O::I64AtomicRmw8AndU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x31\x00\x00"), I{O::I64AtomicRmw16AndU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x32\x00\x00"), I{O::I64AtomicRmw32AndU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x33\x00\x00"), I{O::I32AtomicRmwOr, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x34\x00\x00"), I{O::I64AtomicRmwOr, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x35\x00\x00"), I{O::I32AtomicRmw8OrU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x36\x00\x00"), I{O::I32AtomicRmw16OrU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x37\x00\x00"), I{O::I64AtomicRmw8OrU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x38\x00\x00"), I{O::I64AtomicRmw16OrU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x39\x00\x00"), I{O::I64AtomicRmw32OrU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x3a\x00\x00"), I{O::I32AtomicRmwXor, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x3b\x00\x00"), I{O::I64AtomicRmwXor, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x3c\x00\x00"), I{O::I32AtomicRmw8XorU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x3d\x00\x00"), I{O::I32AtomicRmw16XorU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x3e\x00\x00"), I{O::I64AtomicRmw8XorU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x3f\x00\x00"), I{O::I64AtomicRmw16XorU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x40\x00\x00"), I{O::I64AtomicRmw32XorU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x41\x00\x00"), I{O::I32AtomicRmwXchg, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x42\x00\x00"), I{O::I64AtomicRmwXchg, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x43\x00\x00"), I{O::I32AtomicRmw8XchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x44\x00\x00"), I{O::I32AtomicRmw16XchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x45\x00\x00"), I{O::I64AtomicRmw8XchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x46\x00\x00"), I{O::I64AtomicRmw16XchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x47\x00\x00"), I{O::I64AtomicRmw32XchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x48\x00\x00"), I{O::I32AtomicRmwCmpxchg, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x49\x00\x00"), I{O::I64AtomicRmwCmpxchg, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x4a\x00\x00"),
                 I{O::I32AtomicRmw8CmpxchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x4b\x00\x00"),
                 I{O::I32AtomicRmw16CmpxchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x4c\x00\x00"),
                 I{O::I64AtomicRmw8CmpxchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x4d\x00\x00"),
                 I{O::I64AtomicRmw16CmpxchgU, m});
  ExpectWrite<I>(MakeSpanU8("\xfe\x4e\x00\x00"),
                 I{O::I64AtomicRmw32CmpxchgU, m});
}

TEST(WriteTest, Limits) {
  ExpectWrite<Limits>(MakeSpanU8("\x00\x81\x01"), Limits{129});
  ExpectWrite<Limits>(MakeSpanU8("\x01\x02\xe8\x07"), Limits{2, 1000});
}

TEST(WriteTest, MemArgImmediate) {
  ExpectWrite<MemArgImmediate>(MakeSpanU8("\x00\x00"), MemArgImmediate{0, 0});
  ExpectWrite<MemArgImmediate>(MakeSpanU8("\x01\x80\x02"),
                               MemArgImmediate{1, 256});
}

TEST(WriteTest, Memory) {
  ExpectWrite<Memory>(MakeSpanU8("\x01\x01\x02"),
                      Memory{MemoryType{Limits{1, 2}}});
}

TEST(WriteTest, MemoryType) {
  ExpectWrite<MemoryType>(MakeSpanU8("\x00\x01"), MemoryType{Limits{1}});
  ExpectWrite<MemoryType>(MakeSpanU8("\x01\x00\x80\x01"),
                          MemoryType{Limits{0, 128}});
}

TEST(WriteTest, Mutability) {
  ExpectWrite<Mutability>(MakeSpanU8("\x00"), Mutability::Const);
  ExpectWrite<Mutability>(MakeSpanU8("\x01"), Mutability::Var);
}

TEST(WriteTest, NameSubsectionId) {
  ExpectWrite<NameSubsectionId>(MakeSpanU8("\x00"),
                                NameSubsectionId::ModuleName);
  ExpectWrite<NameSubsectionId>(MakeSpanU8("\x01"),
                                NameSubsectionId::FunctionNames);
  ExpectWrite<NameSubsectionId>(MakeSpanU8("\x02"),
                                NameSubsectionId::LocalNames);
}

TEST(WriteTest, Opcode) {
  ExpectWrite<Opcode>(MakeSpanU8("\x00"), Opcode::Unreachable);
  ExpectWrite<Opcode>(MakeSpanU8("\x01"), Opcode::Nop);
  ExpectWrite<Opcode>(MakeSpanU8("\x02"), Opcode::Block);
  ExpectWrite<Opcode>(MakeSpanU8("\x03"), Opcode::Loop);
  ExpectWrite<Opcode>(MakeSpanU8("\x04"), Opcode::If);
  ExpectWrite<Opcode>(MakeSpanU8("\x05"), Opcode::Else);
  ExpectWrite<Opcode>(MakeSpanU8("\x0b"), Opcode::End);
  ExpectWrite<Opcode>(MakeSpanU8("\x0c"), Opcode::Br);
  ExpectWrite<Opcode>(MakeSpanU8("\x0d"), Opcode::BrIf);
  ExpectWrite<Opcode>(MakeSpanU8("\x0e"), Opcode::BrTable);
  ExpectWrite<Opcode>(MakeSpanU8("\x0f"), Opcode::Return);
  ExpectWrite<Opcode>(MakeSpanU8("\x10"), Opcode::Call);
  ExpectWrite<Opcode>(MakeSpanU8("\x11"), Opcode::CallIndirect);
  ExpectWrite<Opcode>(MakeSpanU8("\x1a"), Opcode::Drop);
  ExpectWrite<Opcode>(MakeSpanU8("\x1b"), Opcode::Select);
  ExpectWrite<Opcode>(MakeSpanU8("\x20"), Opcode::LocalGet);
  ExpectWrite<Opcode>(MakeSpanU8("\x21"), Opcode::LocalSet);
  ExpectWrite<Opcode>(MakeSpanU8("\x22"), Opcode::LocalTee);
  ExpectWrite<Opcode>(MakeSpanU8("\x23"), Opcode::GlobalGet);
  ExpectWrite<Opcode>(MakeSpanU8("\x24"), Opcode::GlobalSet);
  ExpectWrite<Opcode>(MakeSpanU8("\x28"), Opcode::I32Load);
  ExpectWrite<Opcode>(MakeSpanU8("\x29"), Opcode::I64Load);
  ExpectWrite<Opcode>(MakeSpanU8("\x2a"), Opcode::F32Load);
  ExpectWrite<Opcode>(MakeSpanU8("\x2b"), Opcode::F64Load);
  ExpectWrite<Opcode>(MakeSpanU8("\x2c"), Opcode::I32Load8S);
  ExpectWrite<Opcode>(MakeSpanU8("\x2d"), Opcode::I32Load8U);
  ExpectWrite<Opcode>(MakeSpanU8("\x2e"), Opcode::I32Load16S);
  ExpectWrite<Opcode>(MakeSpanU8("\x2f"), Opcode::I32Load16U);
  ExpectWrite<Opcode>(MakeSpanU8("\x30"), Opcode::I64Load8S);
  ExpectWrite<Opcode>(MakeSpanU8("\x31"), Opcode::I64Load8U);
  ExpectWrite<Opcode>(MakeSpanU8("\x32"), Opcode::I64Load16S);
  ExpectWrite<Opcode>(MakeSpanU8("\x33"), Opcode::I64Load16U);
  ExpectWrite<Opcode>(MakeSpanU8("\x34"), Opcode::I64Load32S);
  ExpectWrite<Opcode>(MakeSpanU8("\x35"), Opcode::I64Load32U);
  ExpectWrite<Opcode>(MakeSpanU8("\x36"), Opcode::I32Store);
  ExpectWrite<Opcode>(MakeSpanU8("\x37"), Opcode::I64Store);
  ExpectWrite<Opcode>(MakeSpanU8("\x38"), Opcode::F32Store);
  ExpectWrite<Opcode>(MakeSpanU8("\x39"), Opcode::F64Store);
  ExpectWrite<Opcode>(MakeSpanU8("\x3a"), Opcode::I32Store8);
  ExpectWrite<Opcode>(MakeSpanU8("\x3b"), Opcode::I32Store16);
  ExpectWrite<Opcode>(MakeSpanU8("\x3c"), Opcode::I64Store8);
  ExpectWrite<Opcode>(MakeSpanU8("\x3d"), Opcode::I64Store16);
  ExpectWrite<Opcode>(MakeSpanU8("\x3e"), Opcode::I64Store32);
  ExpectWrite<Opcode>(MakeSpanU8("\x3f"), Opcode::MemorySize);
  ExpectWrite<Opcode>(MakeSpanU8("\x40"), Opcode::MemoryGrow);
  ExpectWrite<Opcode>(MakeSpanU8("\x41"), Opcode::I32Const);
  ExpectWrite<Opcode>(MakeSpanU8("\x42"), Opcode::I64Const);
  ExpectWrite<Opcode>(MakeSpanU8("\x43"), Opcode::F32Const);
  ExpectWrite<Opcode>(MakeSpanU8("\x44"), Opcode::F64Const);
  ExpectWrite<Opcode>(MakeSpanU8("\x45"), Opcode::I32Eqz);
  ExpectWrite<Opcode>(MakeSpanU8("\x46"), Opcode::I32Eq);
  ExpectWrite<Opcode>(MakeSpanU8("\x47"), Opcode::I32Ne);
  ExpectWrite<Opcode>(MakeSpanU8("\x48"), Opcode::I32LtS);
  ExpectWrite<Opcode>(MakeSpanU8("\x49"), Opcode::I32LtU);
  ExpectWrite<Opcode>(MakeSpanU8("\x4a"), Opcode::I32GtS);
  ExpectWrite<Opcode>(MakeSpanU8("\x4b"), Opcode::I32GtU);
  ExpectWrite<Opcode>(MakeSpanU8("\x4c"), Opcode::I32LeS);
  ExpectWrite<Opcode>(MakeSpanU8("\x4d"), Opcode::I32LeU);
  ExpectWrite<Opcode>(MakeSpanU8("\x4e"), Opcode::I32GeS);
  ExpectWrite<Opcode>(MakeSpanU8("\x4f"), Opcode::I32GeU);
  ExpectWrite<Opcode>(MakeSpanU8("\x50"), Opcode::I64Eqz);
  ExpectWrite<Opcode>(MakeSpanU8("\x51"), Opcode::I64Eq);
  ExpectWrite<Opcode>(MakeSpanU8("\x52"), Opcode::I64Ne);
  ExpectWrite<Opcode>(MakeSpanU8("\x53"), Opcode::I64LtS);
  ExpectWrite<Opcode>(MakeSpanU8("\x54"), Opcode::I64LtU);
  ExpectWrite<Opcode>(MakeSpanU8("\x55"), Opcode::I64GtS);
  ExpectWrite<Opcode>(MakeSpanU8("\x56"), Opcode::I64GtU);
  ExpectWrite<Opcode>(MakeSpanU8("\x57"), Opcode::I64LeS);
  ExpectWrite<Opcode>(MakeSpanU8("\x58"), Opcode::I64LeU);
  ExpectWrite<Opcode>(MakeSpanU8("\x59"), Opcode::I64GeS);
  ExpectWrite<Opcode>(MakeSpanU8("\x5a"), Opcode::I64GeU);
  ExpectWrite<Opcode>(MakeSpanU8("\x5b"), Opcode::F32Eq);
  ExpectWrite<Opcode>(MakeSpanU8("\x5c"), Opcode::F32Ne);
  ExpectWrite<Opcode>(MakeSpanU8("\x5d"), Opcode::F32Lt);
  ExpectWrite<Opcode>(MakeSpanU8("\x5e"), Opcode::F32Gt);
  ExpectWrite<Opcode>(MakeSpanU8("\x5f"), Opcode::F32Le);
  ExpectWrite<Opcode>(MakeSpanU8("\x60"), Opcode::F32Ge);
  ExpectWrite<Opcode>(MakeSpanU8("\x61"), Opcode::F64Eq);
  ExpectWrite<Opcode>(MakeSpanU8("\x62"), Opcode::F64Ne);
  ExpectWrite<Opcode>(MakeSpanU8("\x63"), Opcode::F64Lt);
  ExpectWrite<Opcode>(MakeSpanU8("\x64"), Opcode::F64Gt);
  ExpectWrite<Opcode>(MakeSpanU8("\x65"), Opcode::F64Le);
  ExpectWrite<Opcode>(MakeSpanU8("\x66"), Opcode::F64Ge);
  ExpectWrite<Opcode>(MakeSpanU8("\x67"), Opcode::I32Clz);
  ExpectWrite<Opcode>(MakeSpanU8("\x68"), Opcode::I32Ctz);
  ExpectWrite<Opcode>(MakeSpanU8("\x69"), Opcode::I32Popcnt);
  ExpectWrite<Opcode>(MakeSpanU8("\x6a"), Opcode::I32Add);
  ExpectWrite<Opcode>(MakeSpanU8("\x6b"), Opcode::I32Sub);
  ExpectWrite<Opcode>(MakeSpanU8("\x6c"), Opcode::I32Mul);
  ExpectWrite<Opcode>(MakeSpanU8("\x6d"), Opcode::I32DivS);
  ExpectWrite<Opcode>(MakeSpanU8("\x6e"), Opcode::I32DivU);
  ExpectWrite<Opcode>(MakeSpanU8("\x6f"), Opcode::I32RemS);
  ExpectWrite<Opcode>(MakeSpanU8("\x70"), Opcode::I32RemU);
  ExpectWrite<Opcode>(MakeSpanU8("\x71"), Opcode::I32And);
  ExpectWrite<Opcode>(MakeSpanU8("\x72"), Opcode::I32Or);
  ExpectWrite<Opcode>(MakeSpanU8("\x73"), Opcode::I32Xor);
  ExpectWrite<Opcode>(MakeSpanU8("\x74"), Opcode::I32Shl);
  ExpectWrite<Opcode>(MakeSpanU8("\x75"), Opcode::I32ShrS);
  ExpectWrite<Opcode>(MakeSpanU8("\x76"), Opcode::I32ShrU);
  ExpectWrite<Opcode>(MakeSpanU8("\x77"), Opcode::I32Rotl);
  ExpectWrite<Opcode>(MakeSpanU8("\x78"), Opcode::I32Rotr);
  ExpectWrite<Opcode>(MakeSpanU8("\x79"), Opcode::I64Clz);
  ExpectWrite<Opcode>(MakeSpanU8("\x7a"), Opcode::I64Ctz);
  ExpectWrite<Opcode>(MakeSpanU8("\x7b"), Opcode::I64Popcnt);
  ExpectWrite<Opcode>(MakeSpanU8("\x7c"), Opcode::I64Add);
  ExpectWrite<Opcode>(MakeSpanU8("\x7d"), Opcode::I64Sub);
  ExpectWrite<Opcode>(MakeSpanU8("\x7e"), Opcode::I64Mul);
  ExpectWrite<Opcode>(MakeSpanU8("\x7f"), Opcode::I64DivS);
  ExpectWrite<Opcode>(MakeSpanU8("\x80"), Opcode::I64DivU);
  ExpectWrite<Opcode>(MakeSpanU8("\x81"), Opcode::I64RemS);
  ExpectWrite<Opcode>(MakeSpanU8("\x82"), Opcode::I64RemU);
  ExpectWrite<Opcode>(MakeSpanU8("\x83"), Opcode::I64And);
  ExpectWrite<Opcode>(MakeSpanU8("\x84"), Opcode::I64Or);
  ExpectWrite<Opcode>(MakeSpanU8("\x85"), Opcode::I64Xor);
  ExpectWrite<Opcode>(MakeSpanU8("\x86"), Opcode::I64Shl);
  ExpectWrite<Opcode>(MakeSpanU8("\x87"), Opcode::I64ShrS);
  ExpectWrite<Opcode>(MakeSpanU8("\x88"), Opcode::I64ShrU);
  ExpectWrite<Opcode>(MakeSpanU8("\x89"), Opcode::I64Rotl);
  ExpectWrite<Opcode>(MakeSpanU8("\x8a"), Opcode::I64Rotr);
  ExpectWrite<Opcode>(MakeSpanU8("\x8b"), Opcode::F32Abs);
  ExpectWrite<Opcode>(MakeSpanU8("\x8c"), Opcode::F32Neg);
  ExpectWrite<Opcode>(MakeSpanU8("\x8d"), Opcode::F32Ceil);
  ExpectWrite<Opcode>(MakeSpanU8("\x8e"), Opcode::F32Floor);
  ExpectWrite<Opcode>(MakeSpanU8("\x8f"), Opcode::F32Trunc);
  ExpectWrite<Opcode>(MakeSpanU8("\x90"), Opcode::F32Nearest);
  ExpectWrite<Opcode>(MakeSpanU8("\x91"), Opcode::F32Sqrt);
  ExpectWrite<Opcode>(MakeSpanU8("\x92"), Opcode::F32Add);
  ExpectWrite<Opcode>(MakeSpanU8("\x93"), Opcode::F32Sub);
  ExpectWrite<Opcode>(MakeSpanU8("\x94"), Opcode::F32Mul);
  ExpectWrite<Opcode>(MakeSpanU8("\x95"), Opcode::F32Div);
  ExpectWrite<Opcode>(MakeSpanU8("\x96"), Opcode::F32Min);
  ExpectWrite<Opcode>(MakeSpanU8("\x97"), Opcode::F32Max);
  ExpectWrite<Opcode>(MakeSpanU8("\x98"), Opcode::F32Copysign);
  ExpectWrite<Opcode>(MakeSpanU8("\x99"), Opcode::F64Abs);
  ExpectWrite<Opcode>(MakeSpanU8("\x9a"), Opcode::F64Neg);
  ExpectWrite<Opcode>(MakeSpanU8("\x9b"), Opcode::F64Ceil);
  ExpectWrite<Opcode>(MakeSpanU8("\x9c"), Opcode::F64Floor);
  ExpectWrite<Opcode>(MakeSpanU8("\x9d"), Opcode::F64Trunc);
  ExpectWrite<Opcode>(MakeSpanU8("\x9e"), Opcode::F64Nearest);
  ExpectWrite<Opcode>(MakeSpanU8("\x9f"), Opcode::F64Sqrt);
  ExpectWrite<Opcode>(MakeSpanU8("\xa0"), Opcode::F64Add);
  ExpectWrite<Opcode>(MakeSpanU8("\xa1"), Opcode::F64Sub);
  ExpectWrite<Opcode>(MakeSpanU8("\xa2"), Opcode::F64Mul);
  ExpectWrite<Opcode>(MakeSpanU8("\xa3"), Opcode::F64Div);
  ExpectWrite<Opcode>(MakeSpanU8("\xa4"), Opcode::F64Min);
  ExpectWrite<Opcode>(MakeSpanU8("\xa5"), Opcode::F64Max);
  ExpectWrite<Opcode>(MakeSpanU8("\xa6"), Opcode::F64Copysign);
  ExpectWrite<Opcode>(MakeSpanU8("\xa7"), Opcode::I32WrapI64);
  ExpectWrite<Opcode>(MakeSpanU8("\xa8"), Opcode::I32TruncF32S);
  ExpectWrite<Opcode>(MakeSpanU8("\xa9"), Opcode::I32TruncF32U);
  ExpectWrite<Opcode>(MakeSpanU8("\xaa"), Opcode::I32TruncF64S);
  ExpectWrite<Opcode>(MakeSpanU8("\xab"), Opcode::I32TruncF64U);
  ExpectWrite<Opcode>(MakeSpanU8("\xac"), Opcode::I64ExtendI32S);
  ExpectWrite<Opcode>(MakeSpanU8("\xad"), Opcode::I64ExtendI32U);
  ExpectWrite<Opcode>(MakeSpanU8("\xae"), Opcode::I64TruncF32S);
  ExpectWrite<Opcode>(MakeSpanU8("\xaf"), Opcode::I64TruncF32U);
  ExpectWrite<Opcode>(MakeSpanU8("\xb0"), Opcode::I64TruncF64S);
  ExpectWrite<Opcode>(MakeSpanU8("\xb1"), Opcode::I64TruncF64U);
  ExpectWrite<Opcode>(MakeSpanU8("\xb2"), Opcode::F32ConvertI32S);
  ExpectWrite<Opcode>(MakeSpanU8("\xb3"), Opcode::F32ConvertI32U);
  ExpectWrite<Opcode>(MakeSpanU8("\xb4"), Opcode::F32ConvertI64S);
  ExpectWrite<Opcode>(MakeSpanU8("\xb5"), Opcode::F32ConvertI64U);
  ExpectWrite<Opcode>(MakeSpanU8("\xb6"), Opcode::F32DemoteF64);
  ExpectWrite<Opcode>(MakeSpanU8("\xb7"), Opcode::F64ConvertI32S);
  ExpectWrite<Opcode>(MakeSpanU8("\xb8"), Opcode::F64ConvertI32U);
  ExpectWrite<Opcode>(MakeSpanU8("\xb9"), Opcode::F64ConvertI64S);
  ExpectWrite<Opcode>(MakeSpanU8("\xba"), Opcode::F64ConvertI64U);
  ExpectWrite<Opcode>(MakeSpanU8("\xbb"), Opcode::F64PromoteF32);
  ExpectWrite<Opcode>(MakeSpanU8("\xbc"), Opcode::I32ReinterpretF32);
  ExpectWrite<Opcode>(MakeSpanU8("\xbd"), Opcode::I64ReinterpretF64);
  ExpectWrite<Opcode>(MakeSpanU8("\xbe"), Opcode::F32ReinterpretI32);
  ExpectWrite<Opcode>(MakeSpanU8("\xbf"), Opcode::F64ReinterpretI64);
}

TEST(WriteTest, Opcode_tail_call) {
  ExpectWrite<Opcode>(MakeSpanU8("\x12"), Opcode::ReturnCall);
  ExpectWrite<Opcode>(MakeSpanU8("\x13"), Opcode::ReturnCallIndirect);
}

TEST(WriteTest, Opcode_sign_extension) {
  ExpectWrite<Opcode>(MakeSpanU8("\xc0"), Opcode::I32Extend8S);
  ExpectWrite<Opcode>(MakeSpanU8("\xc1"), Opcode::I32Extend16S);
  ExpectWrite<Opcode>(MakeSpanU8("\xc2"), Opcode::I64Extend8S);
  ExpectWrite<Opcode>(MakeSpanU8("\xc3"), Opcode::I64Extend16S);
  ExpectWrite<Opcode>(MakeSpanU8("\xc4"), Opcode::I64Extend32S);
}

TEST(WriteTest, Opcode_reference_types) {
  ExpectWrite<Opcode>(MakeSpanU8("\xd0"), Opcode::RefNull);
  ExpectWrite<Opcode>(MakeSpanU8("\xd1"), Opcode::RefIsNull);
}

TEST(WriteTest, Opcode_function_references) {
  ExpectWrite<Opcode>(MakeSpanU8("\xd2"), Opcode::RefFunc);
}

TEST(WriteTest, Opcode_saturating_float_to_int) {
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x00"), Opcode::I32TruncSatF32S);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x01"), Opcode::I32TruncSatF32U);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x02"), Opcode::I32TruncSatF64S);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x03"), Opcode::I32TruncSatF64U);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x04"), Opcode::I64TruncSatF32S);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x05"), Opcode::I64TruncSatF32U);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x06"), Opcode::I64TruncSatF64S);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x07"), Opcode::I64TruncSatF64U);
}

TEST(WriteTest, Opcode_bulk_memory) {
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x08"), Opcode::MemoryInit);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x09"), Opcode::DataDrop);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x0a"), Opcode::MemoryCopy);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x0b"), Opcode::MemoryFill);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x0c"), Opcode::TableInit);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x0d"), Opcode::ElemDrop);
  ExpectWrite<Opcode>(MakeSpanU8("\xfc\x0e"), Opcode::TableCopy);
}

TEST(WriteTest, Opcode_simd) {
  using O = Opcode;

  ExpectWrite<O>(MakeSpanU8("\xfd\x00"), O::V128Load);
  ExpectWrite<O>(MakeSpanU8("\xfd\x01"), O::V128Store);
  ExpectWrite<O>(MakeSpanU8("\xfd\x02"), O::V128Const);
  ExpectWrite<O>(MakeSpanU8("\xfd\x03"), O::V8X16Shuffle);
  ExpectWrite<O>(MakeSpanU8("\xfd\x04"), O::I8X16Splat);
  ExpectWrite<O>(MakeSpanU8("\xfd\x05"), O::I8X16ExtractLaneS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x06"), O::I8X16ExtractLaneU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x07"), O::I8X16ReplaceLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x08"), O::I16X8Splat);
  ExpectWrite<O>(MakeSpanU8("\xfd\x09"), O::I16X8ExtractLaneS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x0a"), O::I16X8ExtractLaneU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x0b"), O::I16X8ReplaceLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x0c"), O::I32X4Splat);
  ExpectWrite<O>(MakeSpanU8("\xfd\x0d"), O::I32X4ExtractLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x0e"), O::I32X4ReplaceLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x0f"), O::I64X2Splat);
  ExpectWrite<O>(MakeSpanU8("\xfd\x10"), O::I64X2ExtractLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x11"), O::I64X2ReplaceLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x12"), O::F32X4Splat);
  ExpectWrite<O>(MakeSpanU8("\xfd\x13"), O::F32X4ExtractLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x14"), O::F32X4ReplaceLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x15"), O::F64X2Splat);
  ExpectWrite<O>(MakeSpanU8("\xfd\x16"), O::F64X2ExtractLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x17"), O::F64X2ReplaceLane);
  ExpectWrite<O>(MakeSpanU8("\xfd\x18"), O::I8X16Eq);
  ExpectWrite<O>(MakeSpanU8("\xfd\x19"), O::I8X16Ne);
  ExpectWrite<O>(MakeSpanU8("\xfd\x1a"), O::I8X16LtS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x1b"), O::I8X16LtU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x1c"), O::I8X16GtS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x1d"), O::I8X16GtU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x1e"), O::I8X16LeS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x1f"), O::I8X16LeU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x20"), O::I8X16GeS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x21"), O::I8X16GeU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x22"), O::I16X8Eq);
  ExpectWrite<O>(MakeSpanU8("\xfd\x23"), O::I16X8Ne);
  ExpectWrite<O>(MakeSpanU8("\xfd\x24"), O::I16X8LtS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x25"), O::I16X8LtU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x26"), O::I16X8GtS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x27"), O::I16X8GtU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x28"), O::I16X8LeS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x29"), O::I16X8LeU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x2a"), O::I16X8GeS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x2b"), O::I16X8GeU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x2c"), O::I32X4Eq);
  ExpectWrite<O>(MakeSpanU8("\xfd\x2d"), O::I32X4Ne);
  ExpectWrite<O>(MakeSpanU8("\xfd\x2e"), O::I32X4LtS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x2f"), O::I32X4LtU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x30"), O::I32X4GtS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x31"), O::I32X4GtU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x32"), O::I32X4LeS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x33"), O::I32X4LeU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x34"), O::I32X4GeS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x35"), O::I32X4GeU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x40"), O::F32X4Eq);
  ExpectWrite<O>(MakeSpanU8("\xfd\x41"), O::F32X4Ne);
  ExpectWrite<O>(MakeSpanU8("\xfd\x42"), O::F32X4Lt);
  ExpectWrite<O>(MakeSpanU8("\xfd\x43"), O::F32X4Gt);
  ExpectWrite<O>(MakeSpanU8("\xfd\x44"), O::F32X4Le);
  ExpectWrite<O>(MakeSpanU8("\xfd\x45"), O::F32X4Ge);
  ExpectWrite<O>(MakeSpanU8("\xfd\x46"), O::F64X2Eq);
  ExpectWrite<O>(MakeSpanU8("\xfd\x47"), O::F64X2Ne);
  ExpectWrite<O>(MakeSpanU8("\xfd\x48"), O::F64X2Lt);
  ExpectWrite<O>(MakeSpanU8("\xfd\x49"), O::F64X2Gt);
  ExpectWrite<O>(MakeSpanU8("\xfd\x4a"), O::F64X2Le);
  ExpectWrite<O>(MakeSpanU8("\xfd\x4b"), O::F64X2Ge);
  ExpectWrite<O>(MakeSpanU8("\xfd\x4c"), O::V128Not);
  ExpectWrite<O>(MakeSpanU8("\xfd\x4d"), O::V128And);
  ExpectWrite<O>(MakeSpanU8("\xfd\x4e"), O::V128Or);
  ExpectWrite<O>(MakeSpanU8("\xfd\x4f"), O::V128Xor);
  ExpectWrite<O>(MakeSpanU8("\xfd\x50"), O::V128BitSelect);
  ExpectWrite<O>(MakeSpanU8("\xfd\x51"), O::I8X16Neg);
  ExpectWrite<O>(MakeSpanU8("\xfd\x52"), O::I8X16AnyTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x53"), O::I8X16AllTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x54"), O::I8X16Shl);
  ExpectWrite<O>(MakeSpanU8("\xfd\x55"), O::I8X16ShrS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x56"), O::I8X16ShrU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x57"), O::I8X16Add);
  ExpectWrite<O>(MakeSpanU8("\xfd\x58"), O::I8X16AddSaturateS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x59"), O::I8X16AddSaturateU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x5a"), O::I8X16Sub);
  ExpectWrite<O>(MakeSpanU8("\xfd\x5b"), O::I8X16SubSaturateS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x5c"), O::I8X16SubSaturateU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x5d"), O::I8X16Mul);
  ExpectWrite<O>(MakeSpanU8("\xfd\x62"), O::I16X8Neg);
  ExpectWrite<O>(MakeSpanU8("\xfd\x63"), O::I16X8AnyTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x64"), O::I16X8AllTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x65"), O::I16X8Shl);
  ExpectWrite<O>(MakeSpanU8("\xfd\x66"), O::I16X8ShrS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x67"), O::I16X8ShrU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x68"), O::I16X8Add);
  ExpectWrite<O>(MakeSpanU8("\xfd\x69"), O::I16X8AddSaturateS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x6a"), O::I16X8AddSaturateU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x6b"), O::I16X8Sub);
  ExpectWrite<O>(MakeSpanU8("\xfd\x6c"), O::I16X8SubSaturateS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x6d"), O::I16X8SubSaturateU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x6e"), O::I16X8Mul);
  ExpectWrite<O>(MakeSpanU8("\xfd\x73"), O::I32X4Neg);
  ExpectWrite<O>(MakeSpanU8("\xfd\x74"), O::I32X4AnyTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x75"), O::I32X4AllTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x76"), O::I32X4Shl);
  ExpectWrite<O>(MakeSpanU8("\xfd\x77"), O::I32X4ShrS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x78"), O::I32X4ShrU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x79"), O::I32X4Add);
  ExpectWrite<O>(MakeSpanU8("\xfd\x7c"), O::I32X4Sub);
  ExpectWrite<O>(MakeSpanU8("\xfd\x7f"), O::I32X4Mul);
  ExpectWrite<O>(MakeSpanU8("\xfd\x84\x01"), O::I64X2Neg);
  ExpectWrite<O>(MakeSpanU8("\xfd\x85\x01"), O::I64X2AnyTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x86\x01"), O::I64X2AllTrue);
  ExpectWrite<O>(MakeSpanU8("\xfd\x87\x01"), O::I64X2Shl);
  ExpectWrite<O>(MakeSpanU8("\xfd\x88\x01"), O::I64X2ShrS);
  ExpectWrite<O>(MakeSpanU8("\xfd\x89\x01"), O::I64X2ShrU);
  ExpectWrite<O>(MakeSpanU8("\xfd\x8a\x01"), O::I64X2Add);
  ExpectWrite<O>(MakeSpanU8("\xfd\x8d\x01"), O::I64X2Sub);
  ExpectWrite<O>(MakeSpanU8("\xfd\x95\x01"), O::F32X4Abs);
  ExpectWrite<O>(MakeSpanU8("\xfd\x96\x01"), O::F32X4Neg);
  ExpectWrite<O>(MakeSpanU8("\xfd\x97\x01"), O::F32X4Sqrt);
  ExpectWrite<O>(MakeSpanU8("\xfd\x9a\x01"), O::F32X4Add);
  ExpectWrite<O>(MakeSpanU8("\xfd\x9b\x01"), O::F32X4Sub);
  ExpectWrite<O>(MakeSpanU8("\xfd\x9c\x01"), O::F32X4Mul);
  ExpectWrite<O>(MakeSpanU8("\xfd\x9d\x01"), O::F32X4Div);
  ExpectWrite<O>(MakeSpanU8("\xfd\x9e\x01"), O::F32X4Min);
  ExpectWrite<O>(MakeSpanU8("\xfd\x9f\x01"), O::F32X4Max);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa0\x01"), O::F64X2Abs);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa1\x01"), O::F64X2Neg);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa2\x01"), O::F64X2Sqrt);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa5\x01"), O::F64X2Add);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa6\x01"), O::F64X2Sub);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa7\x01"), O::F64X2Mul);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa8\x01"), O::F64X2Div);
  ExpectWrite<O>(MakeSpanU8("\xfd\xa9\x01"), O::F64X2Min);
  ExpectWrite<O>(MakeSpanU8("\xfd\xaa\x01"), O::F64X2Max);
  ExpectWrite<O>(MakeSpanU8("\xfd\xab\x01"), O::I32X4TruncSatF32X4S);
  ExpectWrite<O>(MakeSpanU8("\xfd\xac\x01"), O::I32X4TruncSatF32X4U);
  ExpectWrite<O>(MakeSpanU8("\xfd\xad\x01"), O::I64X2TruncSatF64X2S);
  ExpectWrite<O>(MakeSpanU8("\xfd\xae\x01"), O::I64X2TruncSatF64X2U);
  ExpectWrite<O>(MakeSpanU8("\xfd\xaf\x01"), O::F32X4ConvertI32X4S);
  ExpectWrite<O>(MakeSpanU8("\xfd\xb0\x01"), O::F32X4ConvertI32X4U);
  ExpectWrite<O>(MakeSpanU8("\xfd\xb1\x01"), O::F64X2ConvertI64X2S);
  ExpectWrite<O>(MakeSpanU8("\xfd\xb2\x01"), O::F64X2ConvertI64X2U);
}

TEST(WriteTest, Opcode_threads) {
  using O = Opcode;

  ExpectWrite<O>(MakeSpanU8("\xfe\x00"), O::AtomicNotify);
  ExpectWrite<O>(MakeSpanU8("\xfe\x01"), O::I32AtomicWait);
  ExpectWrite<O>(MakeSpanU8("\xfe\x02"), O::I64AtomicWait);
  ExpectWrite<O>(MakeSpanU8("\xfe\x10"), O::I32AtomicLoad);
  ExpectWrite<O>(MakeSpanU8("\xfe\x11"), O::I64AtomicLoad);
  ExpectWrite<O>(MakeSpanU8("\xfe\x12"), O::I32AtomicLoad8U);
  ExpectWrite<O>(MakeSpanU8("\xfe\x13"), O::I32AtomicLoad16U);
  ExpectWrite<O>(MakeSpanU8("\xfe\x14"), O::I64AtomicLoad8U);
  ExpectWrite<O>(MakeSpanU8("\xfe\x15"), O::I64AtomicLoad16U);
  ExpectWrite<O>(MakeSpanU8("\xfe\x16"), O::I64AtomicLoad32U);
  ExpectWrite<O>(MakeSpanU8("\xfe\x17"), O::I32AtomicStore);
  ExpectWrite<O>(MakeSpanU8("\xfe\x18"), O::I64AtomicStore);
  ExpectWrite<O>(MakeSpanU8("\xfe\x19"), O::I32AtomicStore8);
  ExpectWrite<O>(MakeSpanU8("\xfe\x1a"), O::I32AtomicStore16);
  ExpectWrite<O>(MakeSpanU8("\xfe\x1b"), O::I64AtomicStore8);
  ExpectWrite<O>(MakeSpanU8("\xfe\x1c"), O::I64AtomicStore16);
  ExpectWrite<O>(MakeSpanU8("\xfe\x1d"), O::I64AtomicStore32);
  ExpectWrite<O>(MakeSpanU8("\xfe\x1e"), O::I32AtomicRmwAdd);
  ExpectWrite<O>(MakeSpanU8("\xfe\x1f"), O::I64AtomicRmwAdd);
  ExpectWrite<O>(MakeSpanU8("\xfe\x20"), O::I32AtomicRmw8AddU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x21"), O::I32AtomicRmw16AddU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x22"), O::I64AtomicRmw8AddU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x23"), O::I64AtomicRmw16AddU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x24"), O::I64AtomicRmw32AddU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x25"), O::I32AtomicRmwSub);
  ExpectWrite<O>(MakeSpanU8("\xfe\x26"), O::I64AtomicRmwSub);
  ExpectWrite<O>(MakeSpanU8("\xfe\x27"), O::I32AtomicRmw8SubU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x28"), O::I32AtomicRmw16SubU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x29"), O::I64AtomicRmw8SubU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x2a"), O::I64AtomicRmw16SubU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x2b"), O::I64AtomicRmw32SubU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x2c"), O::I32AtomicRmwAnd);
  ExpectWrite<O>(MakeSpanU8("\xfe\x2d"), O::I64AtomicRmwAnd);
  ExpectWrite<O>(MakeSpanU8("\xfe\x2e"), O::I32AtomicRmw8AndU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x2f"), O::I32AtomicRmw16AndU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x30"), O::I64AtomicRmw8AndU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x31"), O::I64AtomicRmw16AndU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x32"), O::I64AtomicRmw32AndU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x33"), O::I32AtomicRmwOr);
  ExpectWrite<O>(MakeSpanU8("\xfe\x34"), O::I64AtomicRmwOr);
  ExpectWrite<O>(MakeSpanU8("\xfe\x35"), O::I32AtomicRmw8OrU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x36"), O::I32AtomicRmw16OrU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x37"), O::I64AtomicRmw8OrU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x38"), O::I64AtomicRmw16OrU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x39"), O::I64AtomicRmw32OrU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x3a"), O::I32AtomicRmwXor);
  ExpectWrite<O>(MakeSpanU8("\xfe\x3b"), O::I64AtomicRmwXor);
  ExpectWrite<O>(MakeSpanU8("\xfe\x3c"), O::I32AtomicRmw8XorU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x3d"), O::I32AtomicRmw16XorU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x3e"), O::I64AtomicRmw8XorU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x3f"), O::I64AtomicRmw16XorU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x40"), O::I64AtomicRmw32XorU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x41"), O::I32AtomicRmwXchg);
  ExpectWrite<O>(MakeSpanU8("\xfe\x42"), O::I64AtomicRmwXchg);
  ExpectWrite<O>(MakeSpanU8("\xfe\x43"), O::I32AtomicRmw8XchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x44"), O::I32AtomicRmw16XchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x45"), O::I64AtomicRmw8XchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x46"), O::I64AtomicRmw16XchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x47"), O::I64AtomicRmw32XchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x48"), O::I32AtomicRmwCmpxchg);
  ExpectWrite<O>(MakeSpanU8("\xfe\x49"), O::I64AtomicRmwCmpxchg);
  ExpectWrite<O>(MakeSpanU8("\xfe\x4a"), O::I32AtomicRmw8CmpxchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x4b"), O::I32AtomicRmw16CmpxchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x4c"), O::I64AtomicRmw8CmpxchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x4d"), O::I64AtomicRmw16CmpxchgU);
  ExpectWrite<O>(MakeSpanU8("\xfe\x4e"), O::I64AtomicRmw32CmpxchgU);
}

TEST(WriteTest, S32) {
  ExpectWrite<s32>(MakeSpanU8("\x20"), 32);
  ExpectWrite<s32>(MakeSpanU8("\x70"), -16);
  ExpectWrite<s32>(MakeSpanU8("\xc0\x03"), 448);
  ExpectWrite<s32>(MakeSpanU8("\xc0\x63"), -3648);
  ExpectWrite<s32>(MakeSpanU8("\xd0\x84\x02"), 33360);
  ExpectWrite<s32>(MakeSpanU8("\xd0\x84\x52"), -753072);
  ExpectWrite<s32>(MakeSpanU8("\xa0\xb0\xc0\x30"), 101718048);
  ExpectWrite<s32>(MakeSpanU8("\xa0\xb0\xc0\x70"), -32499680);
  ExpectWrite<s32>(MakeSpanU8("\xf0\xf0\xf0\xf0\x03"), 1042036848);
  ExpectWrite<s32>(MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"), -837011344);
}

TEST(WriteTest, S64) {
  ExpectWrite<s64>(MakeSpanU8("\x20"), 32);
  ExpectWrite<s64>(MakeSpanU8("\x70"), -16);
  ExpectWrite<s64>(MakeSpanU8("\xc0\x03"), 448);
  ExpectWrite<s64>(MakeSpanU8("\xc0\x63"), -3648);
  ExpectWrite<s64>(MakeSpanU8("\xd0\x84\x02"), 33360);
  ExpectWrite<s64>(MakeSpanU8("\xd0\x84\x52"), -753072);
  ExpectWrite<s64>(MakeSpanU8("\xa0\xb0\xc0\x30"), 101718048);
  ExpectWrite<s64>(MakeSpanU8("\xa0\xb0\xc0\x70"), -32499680);
  ExpectWrite<s64>(MakeSpanU8("\xf0\xf0\xf0\xf0\x03"), 1042036848);
  ExpectWrite<s64>(MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"), -837011344);
  ExpectWrite<s64>(MakeSpanU8("\xe0\xe0\xe0\xe0\x33"), 13893120096);
  ExpectWrite<s64>(MakeSpanU8("\xe0\xe0\xe0\xe0\x51"), -12413554592);
  ExpectWrite<s64>(MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x2c"), 1533472417872);
  ExpectWrite<s64>(MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x77"), -287593715632);
  ExpectWrite<s64>(MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"), 139105536057408);
  ExpectWrite<s64>(MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x63"),
                   -124777254608832);
  ExpectWrite<s64>(MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"),
                   1338117014066474);
  ExpectWrite<s64>(MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"),
                   -12172681868045014);
  ExpectWrite<s64>(MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"),
                   1070725794579330814);
  ExpectWrite<s64>(MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"),
                   -3540960223848057090);
}

TEST(WriteTest, SectionId) {
  ExpectWrite<SectionId>(MakeSpanU8("\x00"), SectionId::Custom);
  ExpectWrite<SectionId>(MakeSpanU8("\x01"), SectionId::Type);
  ExpectWrite<SectionId>(MakeSpanU8("\x02"), SectionId::Import);
  ExpectWrite<SectionId>(MakeSpanU8("\x03"), SectionId::Function);
  ExpectWrite<SectionId>(MakeSpanU8("\x04"), SectionId::Table);
  ExpectWrite<SectionId>(MakeSpanU8("\x05"), SectionId::Memory);
  ExpectWrite<SectionId>(MakeSpanU8("\x06"), SectionId::Global);
  ExpectWrite<SectionId>(MakeSpanU8("\x07"), SectionId::Export);
  ExpectWrite<SectionId>(MakeSpanU8("\x08"), SectionId::Start);
  ExpectWrite<SectionId>(MakeSpanU8("\x09"), SectionId::Element);
  ExpectWrite<SectionId>(MakeSpanU8("\x0a"), SectionId::Code);
  ExpectWrite<SectionId>(MakeSpanU8("\x0b"), SectionId::Data);
  ExpectWrite<SectionId>(MakeSpanU8("\x0c"), SectionId::DataCount);
}

TEST(WriteTest, ShuffleImmediate) {
  ExpectWrite<ShuffleImmediate>(
      MakeSpanU8(
          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"),
      ShuffleImmediate{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});
}

TEST(WriteTest, Start) {
  ExpectWrite<Start>(MakeSpanU8("\x80\x02"), Start{256});
}

TEST(WriteTest, String) {
  ExpectWrite<string_view>(MakeSpanU8("\x05hello"), "hello");
  ExpectWrite<string_view>(MakeSpanU8("\x02hi"), std::string{"hi"});
}

TEST(WriteTest, Table) {
  ExpectWrite<Table>(MakeSpanU8("\x70\x00\x01"),
                     Table{TableType{Limits{1}, ElementType::Funcref}});
}

TEST(WriteTest, TableType) {
  ExpectWrite<TableType>(MakeSpanU8("\x70\x00\x01"),
                         TableType{Limits{1}, ElementType::Funcref});
  ExpectWrite<TableType>(MakeSpanU8("\x70\x01\x01\x02"),
                         TableType{Limits{1, 2}, ElementType::Funcref});
}

TEST(WriteTest, TypeEntry) {
  ExpectWrite<TypeEntry>(MakeSpanU8("\x60\x00\x01\x7f"),
                         TypeEntry{FunctionType{{}, {ValueType::I32}}});
}

TEST(WriteTest, U8) {
  ExpectWrite<u8>(MakeSpanU8("\x2a"), 42);
}

TEST(WriteTest, U32) {
  ExpectWrite<u32>(MakeSpanU8("\x20"), 32u);
  ExpectWrite<u32>(MakeSpanU8("\xc0\x03"), 448u);
  ExpectWrite<u32>(MakeSpanU8("\xd0\x84\x02"), 33360u);
  ExpectWrite<u32>(MakeSpanU8("\xa0\xb0\xc0\x30"), 101718048u);
  ExpectWrite<u32>(MakeSpanU8("\xf0\xf0\xf0\xf0\x03"), 1042036848u);
}

TEST(WriteTest, ValueType) {
  ExpectWrite<ValueType>(MakeSpanU8("\x7f"), ValueType::I32);
  ExpectWrite<ValueType>(MakeSpanU8("\x7e"), ValueType::I64);
  ExpectWrite<ValueType>(MakeSpanU8("\x7d"), ValueType::F32);
  ExpectWrite<ValueType>(MakeSpanU8("\x7c"), ValueType::F64);
  ExpectWrite<ValueType>(MakeSpanU8("\x7b"), ValueType::V128);
}

TEST(WriteTest, WriteVector_u8) {
  const auto expected = MakeSpanU8("\x05hello");
  const std::vector<u8> input{{'h', 'e', 'l', 'l', 'o'}};
  std::vector<u8> output(expected.size());
  auto iter = WriteVector(input.begin(), input.end(),
                          MakeClampedIterator(output.begin(), output.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), output.end());
  EXPECT_EQ(expected, wasp::SpanU8{output});
}

TEST(WriteTest, WriteVector_u32) {
  const auto expected = MakeSpanU8(
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c");
  const std::vector<u32> input{5, 128, 206412};
  std::vector<u8> output(expected.size());
  auto iter = WriteVector(input.begin(), input.end(),
                          MakeClampedIterator(output.begin(), output.end()));
  EXPECT_FALSE(iter.overflow());
  EXPECT_EQ(iter.base(), output.end());
  EXPECT_EQ(expected, wasp::SpanU8{output});
}
