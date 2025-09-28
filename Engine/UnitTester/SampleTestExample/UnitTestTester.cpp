#include "pch.hpp"
#include "UnitTestManager.hpp"
#include "UnitTestTester.hpp"

// Right here contain a sample of how you would create your unit test function to test your stuff

bool SampleUnitTest() {
	// Example test logic
	int a = 5;
	int b = 10;
	int sum = a + b;
	return sum == 15; // Test passes if sum is 15
}

// Uncomment the following 2 functions to test if UnitTestManager correctly identifies a failing test
//bool TestAlwaysFalse() { 
//	return false;
//}

//bool TestMultipleFalse() {
//	// Example test logic
//	int ming = 5;
//	int sheng = 10;
//	int foo = ming + sheng;
//	return foo == 14; // Test passes if sum is 15
//}

// This struct is used to register the tests with the UnitTestManager when the program starts **********
struct RegisterDummyTests {
	RegisterDummyTests() {
		UnitTestManager::GetInstance().AddTest("SampleUnitTest", &SampleUnitTest);
		//UnitTestManager::GetInstance().AddTest("TestAlwaysFalse", &TestAlwaysFalse);
		//UnitTestManager::GetInstance().AddTest("TestMultipleFalse", &TestMultipleFalse);
	}
};

// This static line is very important as it ensures all test are added to the test array before main() is called
// So you need to have this for your own test files
static RegisterDummyTests register_dummy_tests;