/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** FileWatcher
*/

#include "rt/core/FileWatcher.hpp"

namespace rt {

FileWatcher::FileWatcher(const std::string &filePath)
    : filePath_(filePath), lastModTime_(getFileModTime())
{
}

std::time_t FileWatcher::getFileModTime() const
{
    struct stat fileStat;
    if (stat(filePath_.c_str(), &fileStat) == 0) {
        return fileStat.st_mtime;
    }
    return 0;
}

bool FileWatcher::hasChanged()
{
    std::time_t currentModTime = getFileModTime();
    return currentModTime != lastModTime_ && currentModTime != 0;
}

void FileWatcher::reset()
{
    lastModTime_ = getFileModTime();
}

}
