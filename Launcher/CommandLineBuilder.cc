#include "CommandLineBuilder.h"

namespace {
    void AppendBackslashes(std::wstring& output, size_t count) {
        output.append(count, L'\\');
    }

    void AppendQuotedArgument(std::wstring& output, std::wstring_view argument) {
        size_t backslashCount = 0;

        output.push_back(L'"');

        for (wchar_t ch : argument) {
            if (ch == L'\\') {
                ++backslashCount;
                continue;
            }

            if (ch == L'"') {
                AppendBackslashes(output, backslashCount);
                output.push_back(L'\\');
                output.push_back(L'"');
                backslashCount = 0;
                continue;
            }

            AppendBackslashes(output, backslashCount);
            backslashCount = 0;
            output.push_back(ch);
        }

        AppendBackslashes(output, backslashCount * 2);
        output.push_back(L'"');
    }
}

namespace CommandLineBuilder {
    std::wstring Build(int argc, wchar_t** argv) {
        std::wstring commandLine;

        for (int i = 1; i < argc; ++i) {
            if (i > 1) {
                commandLine.push_back(L' ');
            }
            AppendQuotedArgument(commandLine, argv[i]);
        }

        return commandLine;
    }
}
