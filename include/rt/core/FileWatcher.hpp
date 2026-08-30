/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** FileWatcher
*/

#ifndef RT_FILEWATCHER_HPP
    #define RT_FILEWATCHER_HPP

    #include <string>
    #include <chrono>
    #include <sys/stat.h>

namespace rt {

class FileWatcher {
public:
    explicit FileWatcher(const std::string &filePath);
    
    bool hasChanged();
    void reset();
    
private:
    std::string filePath_;
    std::time_t lastModTime_;
    
    std::time_t getFileModTime() const;
};

}

#endif // RT_FILEWATCHER_HPP
