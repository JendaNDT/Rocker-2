#include "AtomicFile.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif JUCE_MAC
#include <stdio.h>
#else
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#include <unistd.h>
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

bool moveNewDirectory(const juce::File& from, const juce::File& to) {
    const auto& source = from.getFullPathName();
    const auto& target = to.getFullPathName();
#if JUCE_WINDOWS
    return MoveFileExW(source.toWideCharPointer(), target.toWideCharPointer(), MOVEFILE_WRITE_THROUGH) !=
           0;
#elif JUCE_MAC
    return renamex_np(source.toRawUTF8(), target.toRawUTF8(), RENAME_EXCL) == 0;
#else
    return syscall(SYS_renameat2, AT_FDCWD, source.toRawUTF8(), AT_FDCWD, target.toRawUTF8(),
                   RENAME_NOREPLACE) == 0;
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
