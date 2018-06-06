#include "GoogleTest/GoogleTestRunner.h"

#include "GoogleTest/GoogleTest_Test.h"
#include "Mangler.h"

#include "Toolchain/Resolvers/InstrumentationResolver.h"
#include "Toolchain/Resolvers/NativeResolver.h"

#include <llvm/IR/Function.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>

using namespace mull;
using namespace llvm;
using namespace llvm::orc;

namespace {
  class UnitTest;
}

static
llvm::orc::RTDyldObjectLinkingLayer::MemoryManagerGetter getMemoryManager() {
  llvm::orc::RTDyldObjectLinkingLayer::MemoryManagerGetter GetMemMgr =
  []() {
    return std::make_shared<SectionMemoryManager>();
  };
  return GetMemMgr;
}

static llvm::orc::RTDyldObjectLinkingLayer::ObjHandleT MullGTEstDummyHandle;

GoogleTestRunner::GoogleTestRunner(llvm::TargetMachine &machine) :
  TestRunner(machine),
  ObjectLayer(getMemoryManager()),
  mangler(Mangler(machine.createDataLayout())),
  overrides([this](const char *name) {
    return this->mangler.getNameWithPrefix(name);
  }),
  fGoogleTestInit(mangler.getNameWithPrefix("_ZN7testing14InitGoogleTestEPiPPc")),
  fGoogleTestInstance(mangler.getNameWithPrefix("_ZN7testing8UnitTest11GetInstanceEv")),
  fGoogleTestRun(mangler.getNameWithPrefix("_ZN7testing8UnitTest3RunEv")),
  handle(MullGTEstDummyHandle),
  trampoline(new InstrumentationInfo*)
{
}

GoogleTestRunner::~GoogleTestRunner() {
  delete trampoline;
}

void *GoogleTestRunner::GetCtorPointer(const llvm::Function &Function) {
  return
    getFunctionPointer(mangler.getNameWithPrefix(Function.getName().str()));
}

void *GoogleTestRunner::getFunctionPointer(const std::string &functionName) {
  JITSymbol symbol = ObjectLayer.findSymbol(functionName, false);

  void *fpointer =
    reinterpret_cast<void *>(static_cast<uintptr_t>(symbol.getAddress().get()));

  if (fpointer == nullptr) {
    errs() << "GoogleTestRunner> Can't find pointer to function: "
           << functionName << "\n";
    exit(1);
  }

  return fpointer;
}

void GoogleTestRunner::runStaticCtor(llvm::Function *Ctor) {
//  printf("Init: %s\n", Ctor->getName().str().c_str());

  void *CtorPointer = GetCtorPointer(*Ctor);

  auto ctor = ((int (*)())(intptr_t)CtorPointer);
  ctor();
}

void GoogleTestRunner::loadInstrumentedProgram(ObjectFiles &objectFiles,
                                               Instrumentation &instrumentation) {
  if (handle != MullGTEstDummyHandle) {
    // TODO
    ObjectLayer.removeObject(handle);
  }

//  handle = ObjectLayer.addObjectSet(objectFiles,
//                                    make_unique<SectionMemoryManager>(),
//                                    make_unique<InstrumentationResolver>(overrides,
//                                                                         instrumentation,
//                                                                         mangler,
//                                                                         trampoline));
//  ObjectLayer.emitAndFinalize(handle);
}

void GoogleTestRunner::loadProgram(ObjectFiles &objectFiles) {
  if (handle != MullGTEstDummyHandle) {
//    ObjectLayer.removeObjectSet(handle);
  }

  std::shared_ptr<NativeResolver> objcResolver = std::make_shared<NativeResolver>(overrides);

  std::unique_ptr<object::ObjectFile> uniqobj = std::unique_ptr<object::ObjectFile>(objectFiles[0]);

  // TODO
  llvm::object::OwningBinary<object::ObjectFile> bocj = llvm::object::OwningBinary<object::ObjectFile>(std::move(uniqobj), std::move(nullptr));

  std::shared_ptr<llvm::object::OwningBinary<object::ObjectFile>> firstObj = std::shared_ptr<llvm::object::OwningBinary<object::ObjectFile>>(&bocj);
//
  handle = ObjectLayer.addObject(firstObj,
                                 objcResolver).get();

  ObjectLayer.emitAndFinalize(handle);
}

ExecutionStatus GoogleTestRunner::runTest(Test *test) {
  *trampoline = &test->getInstrumentationInfo();

  GoogleTest_Test *GTest = dyn_cast<GoogleTest_Test>(test);

  for (auto &Ctor: GTest->GetGlobalCtors()) {
    runStaticCtor(Ctor);
  }

  std::string filter = "--gtest_filter=" + GTest->getTestName();
  const char *argv[] = { "mull", filter.c_str(), NULL };
  int argc = 2;

  /// Normally Google Test Driver looks like this:
  ///
  ///   int main(int argc, char **argv) {
  ///     InitGoogleTest(&argc, argv);
  ///     return UnitTest.GetInstance()->Run();
  ///   }
  ///
  /// Technically we can just call `main` function, but there is a problem:
  /// Among all the files that are being processed may be more than one
  /// `main` function, therefore we can call wrong driver.
  ///
  /// To avoid this from happening we implement the driver function on our own.
  /// We must keep in mind that each project can have its own, extended
  /// version of the driver (LLVM itself has one).
  ///

  void *initGTestPtr = getFunctionPointer(fGoogleTestInit);

  auto initGTest = ((void (*)(int *, const char**))(intptr_t)initGTestPtr);
  initGTest(&argc, argv);

  void *getInstancePtr = getFunctionPointer(fGoogleTestInstance);

  auto getInstance = ((UnitTest *(*)())(intptr_t)getInstancePtr);
  UnitTest *unitTest = getInstance();

  void *runAllTestsPtr = getFunctionPointer(fGoogleTestRun);

  auto runAllTests = ((int (*)(UnitTest *))(intptr_t)runAllTestsPtr);
  uint64_t result = runAllTests(unitTest);

  overrides.runDestructors();

  if (result == 0) {
    return ExecutionStatus::Passed;
  }
  return ExecutionStatus::Failed;
}
