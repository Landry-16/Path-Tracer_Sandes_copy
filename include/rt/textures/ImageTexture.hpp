#ifndef RT_IMAGETEXTURE_HPP
    #define RT_IMAGETEXTURE_HPP

    #include "rt/interfaces/ITexture.hpp"
    #include "rt/math/Color.hpp"
    #include <vector>
    #include <string>

class ImageTexture : public ITexture {
public:
    ImageTexture(const std::string &filename);
    ~ImageTexture() = default;
    
    Color sample(double u, double v) const override;
    bool isLoaded() const { return loaded; }

private:
    std::vector<unsigned char> data;
    int width;
    int height;
    int channels;
    bool loaded;
};

#endif // RT_IMAGETEXTURE_HPP
