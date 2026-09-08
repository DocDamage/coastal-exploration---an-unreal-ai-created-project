#pragma once
#include <iostream>
#include <string>
struct TestRun
{
    int checks = 0, failures = 0;
    void Expect(bool value, const std::string& name)
    {
        ++checks;
        if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
    }
    int Finish(const char* suite) const
    {
        std::cout << suite << ": " << checks << " checks; " << failures << " failures\n";
        return failures == 0 ? 0 : 1;
    }
};
