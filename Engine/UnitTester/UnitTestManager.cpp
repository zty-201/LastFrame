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
    std::cout << "Running " << tests.size() << " unit tests...\n";
    bool allPassed = true;

    for (auto& [name, func] : tests) {
        try {
            if (!func()) {
                std::cerr << "Test failed: " << name << "\n";
                allPassed = false;
            }
            else {
                std::cout << "Test passed: " << name << "\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in test " << name << ": " << e.what() << "\n";
            allPassed = false;
        }
    }

    if (!allPassed) {
        throw std::runtime_error("One or more unit tests failed. Check logs.");
    }
}

