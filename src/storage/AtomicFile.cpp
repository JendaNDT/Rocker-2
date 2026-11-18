#include "AtomicFile.h"
#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace rocker {
bool commitTemporary(const juce::TemporaryFile& temp) {
#if JUCE_WINDOWS
    // Both names are in the same directory. Replace the directory entry in one rename.
    return MoveFileExW(temp.getFile().getFullPathName().toWideCharPointer(),
                       temp.getTargetFile().getFullPathName().toWideCharPointer(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return temp.overwriteTargetFileWithTemporary();
#endif
}

bool atomicJson(const juce::File& path, const juce::var& data) {
    if (path.getParentDirectory().createDirectory().failed())
        return false;
    juce::TemporaryFile temp(path);
    {
        juce::FileOutputStream output(temp.getFile());
        const auto json = juce::JSON::toString(data);
        if (!output.openedOk() || !output.write(json.toRawUTF8(), json.getNumBytesAsUTF8()))
            return false;
        output.flush();
        if (output.getStatus().failed())
            return false;
    }
    return commitTemporary(temp);
}
} // namespace rocker
