#include <juce_core/juce_core.h>

inline juce::String getDllName()
{
#if JUCE_MAC
    return "libDLL.dylib";
#elif JUCE_WINDOWS
    return "dll.dll";
#else
    return "dll.so";
#endif
}

int main()
{
    using juce::File;

    auto exeDir = File::getSpecialLocation(File::currentExecutableFile);
    auto dllFile = exeDir.getParentDirectory().getChildFile(getDllName());

    juce::DynamicLibrary lib {dllFile.getFullPathName()};
    auto func = (void(*)())lib.getFunction("dllFunction");
    func();

    return 0;
}
