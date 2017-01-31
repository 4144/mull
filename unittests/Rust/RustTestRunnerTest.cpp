
#include "Context.h"
#include "MutationOperators/AddMutationOperator.h"
#include "Rust/RustTestFinder.h"
#include "TestModuleFactory.h"
#include "Toolchain/Compiler.h"
#include "Rust/RustTestRunner.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "gtest/gtest.h"

using namespace mull;
using namespace llvm;

static TestModuleFactory TestModuleFactory;

TEST(RustTestRunner, runTest) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  std::unique_ptr<TargetMachine> targetMachine(
                                EngineBuilder().selectTarget(Triple(), "", "",
                                SmallVector<std::string, 1>()));

  Compiler compiler(*targetMachine.get());
  Context context;

  RustTestRunner Runner(*targetMachine.get());
  RustTestRunner::ObjectFiles ObjectFiles;
  RustTestRunner::OwnedObjectFiles OwnedObjectFiles;

  auto ownedModuleWithTests = TestModuleFactory.rustModule();

  Module *moduleWithTests = ownedModuleWithTests.get();

  auto ownedMullModuleWithTests =
    make_unique<MullModule>(std::move(ownedModuleWithTests), "");

  context.addModule(std::move(ownedMullModuleWithTests));

  std::vector<std::unique_ptr<MutationOperator>> mutationOperators;
  mutationOperators.emplace_back(make_unique<AddMutationOperator>());

  RustTestFinder testFinder;

  auto Tests = testFinder.findTests(context);

  ASSERT_EQ(4U, Tests.size());

  auto &Test = *(Tests.begin());

  {
    auto Obj = compiler.compileModule(moduleWithTests);
    ObjectFiles.push_back(Obj.getBinary());
    OwnedObjectFiles.push_back(std::move(Obj));
  }

  /// Here we run test with original testee function
  ASSERT_EQ(ExecutionStatus::Passed, Runner.runTest(Test.get(), ObjectFiles).Status);

  ObjectFiles.erase(ObjectFiles.begin(), ObjectFiles.end());

  /// afterwards we apply single mutation and run test again
  /// expecting it to fail

  std::vector<std::unique_ptr<Testee>> testees =
    testFinder.findTestees(Test.get(), context, 4);

  ASSERT_EQ(2U, testees.size());
  Function *testeeFunction = testees[1]->getTesteeFunction();

  AddMutationOperator MutOp;
  std::vector<MutationOperator *> MutOps({&MutOp});
  std::vector<MutationPoint *> mutationPoints =
    testFinder.findMutationPoints(context, *testeeFunction);

  ASSERT_EQ(1U, mutationPoints.size());

  auto &mutationPoint = *(mutationPoints.begin());

  {
    auto owningObject = mutationPoint->applyMutation(compiler);
    auto mutant = owningObject.getBinary();

    ObjectFiles.push_back(mutant);
    OwnedObjectFiles.push_back(std::move(owningObject));
  }

  ASSERT_EQ(ExecutionStatus::RustTestsFailed, Runner.runTest(Test.get(), ObjectFiles).Status);

  ObjectFiles.erase(ObjectFiles.begin(), ObjectFiles.end());
}
