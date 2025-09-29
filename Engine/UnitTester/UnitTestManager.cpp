#include "pch.hpp"
#include "UnitTestManager.hpp"
#include <stdexcept>

UnitTestManager& UnitTestManager::GetInstance() {
    static UnitTestManager instance;
    return instance;
}

void UnitTestManager::AddTest(const std::string& name, TestFunc func) {
    tests.emplace_back(name, func);
}

// Runs all test does not matter if it fails or pass first, it runs all, and then if there is more than 2 fail test, at the end of this function it will throw exception and tell you whats the function that is broken
void UnitTestManager::RunAllTests() {
    static std::ofstream logFile("unittests.log", std::ios::app); // open once, append mode
    logFile << "Running " << tests.size() << " unit tests..." << std::endl;

    bool allPassed = true;

    for (auto& [name, func] : tests) {
        try {
            if (!func()) {
                logFile << "Test failed: " << name << std::endl;
                allPassed = false;
            }
            else {
                logFile << "Test passed: " << name << std::endl;
            }
        }
        catch (const std::exception& e) {
            logFile << "Exception in test " << name << ": " << e.what() << std::endl;
            allPassed = false;
        }
    }

    if (!allPassed) {
        throw std::runtime_error("One or more unit tests failed. Check unittests.log.");
    }
}


