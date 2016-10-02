#include "Driver.h"
#include "Config.h"
#include "Context.h"
#include "ModuleLoader.h"
#include "SimpleTest/SimpleTestFinder.h"
#include "SimpleTest/SimpleTestRunner.h"
#include "XCTest/XCTestFinder.h"
#include "XCTest/XCTestRunner.h"
#include "TestModuleFactory.h"
#include "TestResult.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"

#include "gtest/gtest.h"

using namespace Mutang;
using namespace llvm;

static LLVMContext GlobalCtx;

static TestModuleFactory TestModuleFactory;

class FakeModuleLoader : public ModuleLoader {
public:
  FakeModuleLoader() : ModuleLoader(GlobalCtx) {}

  std::unique_ptr<llvm::Module> loadModuleAtPath(const std::string &path) override {
    if (path == "foo") {
      return TestModuleFactory.createTesterModule();
    }

    else if (path == "bar") {
      return TestModuleFactory.createTesteeModule();
    }

    else if (path == "xctest_tests") {
      return TestModuleFactory.createXCTest_TesterModule();
    }

//    else if (path == "bar") {
//      return TestModuleFactory.createTesteeModule();
//    }

    return nullptr;
  }
};

TEST(Driver, SimpleTest) {
  /// Create Config with fake BitcodePaths
  /// Create Fake Module Loader
  /// Initialize Driver using ModuleLoader and Config
  /// Driver should initialize (make them injectable? DI?)
  /// TestRunner and TestFinder based on the Config
  /// Then Run all the tests using driver

  std::vector<std::string> ModulePaths({ "foo", "bar" });
  bool doFork = false;
  Config Cfg(ModulePaths, doFork);

  FakeModuleLoader Loader;
  SimpleTestFinder TestFinder;
  SimpleTestRunner Runner;

  Driver Driver(Cfg, Loader, TestFinder, Runner);

  /// Given the modules we use here we expect:
  ///
  /// 1 original test, which has Passed state
  /// 1 mutant test, which has Failed state
  auto Results = Driver.Run();
  ASSERT_EQ(1u, Results.size());

  auto FirstResult = Results.begin()->get();
  ASSERT_EQ(ExecutionStatus::Passed, FirstResult->getOriginalTestResult().Status);
  ASSERT_EQ("test_count_letters", FirstResult->getTestName());

  auto &Mutants = FirstResult->getMutationResults();
  ASSERT_EQ(1u, Mutants.size());

  auto FirstMutant = Mutants.begin()->get();
  ASSERT_EQ(ExecutionStatus::Failed, FirstMutant->getExecutionResult().Status);

  ASSERT_NE(nullptr, FirstMutant->getMutationPoint());
}

TEST(Driver, XCTest) {
  std::vector<std::string> ModulePaths({ "xctest_tests" });
  bool doFork = false;

  Config Cfg(ModulePaths, doFork);

  FakeModuleLoader Loader;
  XCTestFinder TestFinder;
  XCTestRunner Runner;

  Driver Driver(Cfg, Loader, TestFinder, Runner);

  /// Given the modules we use here we expect:
  ///
  /// 1 original test, which has Passed state
  /// 1 mutant test, which has Failed state
  auto Results = Driver.Run();
  ASSERT_EQ(1u, Results.size());

//  auto FirstResult = Results.begin()->get();
//  ASSERT_EQ(ExecutionStatus::Passed, FirstResult->getOriginalTestResult().Status);
//  ASSERT_EQ("test_count_letters", FirstResult->getTestName());
//
//  auto &Mutants = FirstResult->getMutationResults();
//  ASSERT_EQ(1u, Mutants.size());
//
//  auto FirstMutant = Mutants.begin()->get();
//  ASSERT_EQ(ExecutionStatus::Failed, FirstMutant->getExecutionResult().Status);
//
//  ASSERT_NE(nullptr, FirstMutant->getMutationPoint());
}
