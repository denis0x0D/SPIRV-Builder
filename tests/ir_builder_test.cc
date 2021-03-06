#include "../include/ir/ir_builder.h"
#include <memory>

int main() {
  using namespace spirv;
  using namespace std;
  unique_ptr<Module> module = make_unique<Module>("Test module");
  unique_ptr<IRPrinter> printer = make_unique<IRPrinter>();
  printer->AddMetaInfo();
  module->InitHeader();

  // Types.
  spv::Id int_32_t =
      module->CreateCustomType(spv::Op::OpTypeInt, 0, {"32", "1"});
  spv::Id uint_32_t =
      module->CreateCustomType(spv::Op::OpTypeInt, 0, {"32", "0"});
  spv::Id void_t = module->CreateCustomType(spv::Op::OpTypeVoid, 0);
  spv::Id bool_t = module->CreateCustomType(spv::Op::OpTypeBool, 0);
  spv::Id func_type = module->CreateCustomType(spv::Op::OpTypeFunction, void_t);
  spv::Id v3_int =
      module->CreateCustomType(spv::Op::OpTypeVector, int_32_t, {"3"});
  spv::Id ptr_input_v3int =
      module->CreateCustomType(spv::Op::OpTypePointer, v3_int, {"Input"});
  spv::Id int_ptr_type =
      module->CreateCustomType(spv::Op::OpTypePointer, int_32_t, {"Function"});
  spv::Id runtime_arr =
      module->CreateCustomType(spv::Op::OpTypeRuntimeArray, int_32_t);
  spv::Id struct_10 =
      module->CreateCustomType(spv::Op::OpTypeStruct, runtime_arr);
  spv::Id ptr_uniform_struct_10 =
      module->CreateCustomType(spv::Op::OpTypePointer, struct_10, {"Uniform"});

  // Constants
  spv::Id int_0 = module->CreateGlobalVariable(int_32_t, true, {"0"});
  spv::Id int_1 = module->CreateGlobalVariable(int_32_t, true, {"1"});
  spv::Id int_128 = module->CreateGlobalVariable(int_32_t, true, {"128"});

  // Variables
  // Tensor A
  spv::Id input_array1 =
      module->CreateGlobalVariable(ptr_uniform_struct_10, false, {"Uniform"});
  // Tensor B
  spv::Id input_array2 =
      module->CreateGlobalVariable(ptr_uniform_struct_10, false, {"Uniform"});
  // Tensor C 
  spv::Id input_array3 =
      module->CreateGlobalVariable(ptr_uniform_struct_10, false, {"Uniform"});

  spv::Id global_invoc_id =
      module->CreateGlobalVariable(ptr_input_v3int, false, {"Input"});
  spv::Id ptr_uniform_int =
      module->CreateCustomType(spv::Op::OpTypePointer, int_32_t, {"Uniform"});
  spv::Id ptr_input_int =
      module->CreateCustomType(spv::Op::OpTypePointer, int_32_t, {"Input"});

  // Decorate
  module->Decorate(runtime_arr, {"ArrayStride", "4"});
  module->MemberDecorate(struct_10, {"0", "Offset", "0"});
  module->Decorate(struct_10, {"BufferBlock"});
  module->Decorate(global_invoc_id, {"BuiltIn", "GlobalInvocationId"});

  module->Decorate(input_array1, {"DescriptorSet", "0"});
  module->Decorate(input_array1, {"Binding", "0"});
  module->Decorate(input_array2, {"DescriptorSet", "0"});
  module->Decorate(input_array2, {"Binding", "1"});
  module->Decorate(input_array3, {"DescriptorSet", "0"});
  module->Decorate(input_array3, {"Binding", "2"});

  Function *function =
      module->GetOrCreateFunction("compute_kernel", void_t, func_type, "None");

  BasicBlock *entry = new BasicBlock("entry");
  BasicBlock *next_block1 = new BasicBlock("next1");
  BasicBlock *next_block2 = new BasicBlock("next2");
  BasicBlock *next_block3 = new BasicBlock("next3");
  BasicBlock *ret = new BasicBlock("ret");

  function->AddEntryBlock(entry);
  function->AddBasicBlock(next_block1);
  function->AddBasicBlock(next_block2);
  function->AddBasicBlock(next_block3);
  function->AddRetBlock(ret);

  unique_ptr<IRBuilder> builder = make_unique<IRBuilder>(entry, module.get());
  builder->SetInsertPoint(entry);
  builder->CreateBr(next_block1);

  builder->SetInsertPoint(next_block1);
  spv::Id phi_id = builder->CreatePhi(int_32_t);
  builder->AddIncoming(next_block1, phi_id, int_0, entry);
  spv::Id cmp =
      builder->CreateBinOp(spv::Op::OpSLessThan, bool_t, phi_id, int_128);
  builder->CreateLoopMerge(ret, next_block3, {"None"});
  builder->CreateCondBr(cmp, next_block2, ret);

  builder->SetInsertPoint(next_block2);
  spv::Id C_ptr = builder->CreateAccessChain(spv::Op::OpInBoundsAccessChain,
                                             ptr_uniform_int, input_array3,
                                             {int_0, phi_id});
  spv::Id C_value = builder->CreateLoad(int_32_t, C_ptr, {"None"});

  spv::Id B_ptr = builder->CreateAccessChain(spv::Op::OpInBoundsAccessChain,
                                             ptr_uniform_int, input_array2,
                                             {int_0, phi_id});
  spv::Id B_value = builder->CreateLoad(int_32_t, B_ptr, {"None"});

  spv::Id A_value =
      builder->CreateBinOp(spv::Op::OpIAdd, int_32_t, C_value, B_value);

  spv::Id A_ptr = builder->CreateAccessChain(spv::Op::OpInBoundsAccessChain,
                                             ptr_uniform_int, input_array1,
                                             {int_0, phi_id});
  builder->CreateStore(A_ptr, A_value, {"None"});
  builder->CreateBr(next_block3);

  builder->SetInsertPoint(next_block3);
  // Increment index
  spv::Id index =
      builder->CreateBinOp(spv::Op::OpIAdd, int_32_t, phi_id, int_1);
  builder->CreateBr(next_block1);

  // Add incoming value and branch to phi node
  builder->AddIncoming(next_block1, phi_id, index, next_block3);

  module->CreateEntryPoint(function, global_invoc_id);
  module->CreateExecutionMode(function, {"LocalSize", "1", "1", "1"});

  module->Accept(printer.get());
  printer->Dump();
  return 0;
}
