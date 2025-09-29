class UnitTestManager {
public:
    using TestFunc = std::function<bool()>;

    static UnitTestManager& GetInstance();

    void AddTest(const std::string& name, TestFunc func);
    void RunAllTests();

private:
    UnitTestManager() = default;
    std::vector<std::pair<std::string, TestFunc>> tests;
};