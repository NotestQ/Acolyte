// Definitions from SFML. Someone smarter than me can likely include SFML in the project? Maybe?
typedef unsigned int uint;
typedef unsigned __int64 ulong64;
typedef unsigned short ushort;
typedef unsigned char uchar;

namespace sf {
    typedef enum PrimitiveType {
        Points = 0,
        Lines = 1,
        LineStrip = 2,
        LinesStrip = 2,
        Triangles = 3,
        TriangleStrip = 4,
        TrianglesStrip = 4,
        TriangleFan = 5,
        TrianglesFan = 5,
        Quads = 6
    } PrimitiveType;

    template <typename T>
    struct Vector2 {
        T x;
        T y;
    };

    template <typename T>
    struct Rect {
        T* left;
        T* top;
        T* width;
        T* height;
    };

    struct Color {
        uchar r;
        uchar g;
        uchar b;
        uchar a;
    };

    struct Vertex {
        Vector2<float> position;
        Color color;
        Vector2<float> texCoords;
    };

    struct VertexArray {
        int _padding;
        std::vector<Vertex> m_vertices;
        PrimitiveType m_primitivetype;
    };

    struct Texture {
        Vector2<uint> m_size;
        Vector2<uint> m_actualSize;
        uint m_texture;
        bool m_isSmooth;
        bool m_sRgb;
        bool m_isRepeated;
        bool m_pixelsFlipped;
        bool m_fboAttachment;
        bool m_hasMipmap;
        ulong64 m_cacheId;
    };

    struct Image {
        Vector2<uint> m_size;
        std::vector<unsigned char> m_pixels;
    };

    struct Sprite {
        char padding[172]; // Likely vtable stuff
        Vertex m_vertices[4];
        Texture* m_texture;
        Rect<int> m_textureRect;
    };
}