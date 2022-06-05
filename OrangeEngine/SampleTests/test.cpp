#include "pch.h"
#include "../LinkerBuildTests/LinkerBuildTest.h"


////https://docs.microsoft.com/en-us/previous-versions/ms182489(v=vs.140)?redirectedfrom=MSDN
/// https://docs.microsoft.com/en-us/visualstudio/test/vstest-console-options?view=vs-2022

TEST(TestCaseName, TestName) {
  EXPECT_EQ(HelloWorld(), "Hello World");
  EXPECT_TRUE(true);
}


