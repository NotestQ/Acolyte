#pragma once
#include <vector>
#include <string>

// Definitions from SFML. Someone smarter than me can likely include SFML in the project? Maybe?
typedef unsigned int uint;
typedef unsigned __int64 ulong64;
typedef unsigned short ushort;
typedef unsigned char uchar;

namespace sf {
    enum PrimitiveType {
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
    };

    enum Style {
        Regular = 0,
        Bold = 1,
        Italic = 2,
        Underlined = 4,
        StrikeThrough = 8,
    };

    template <typename T>
    struct Vector2 {
        T x;
        T y;
    };

    template <typename T>
    struct Vector3 {
        T x;
        T y;
        T z;
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
        char padding[172];
        Vertex m_vertices[4];
        Texture* m_texture;
        Rect<int> m_textureRect;
    };

    struct Transform {
        float m_matrix[16];
    };

    struct Transformable {
        void* vtable;
        Vector2<float> m_origin;
        Vector2<float> m_position;
        float m_rotation;
        Vector2<float> m_scale;
        Transform m_transform;
        bool m_transformNeedUpdate;
        Transform m_inverseTransform;
        bool m_inverseTransformNeedUpdate;
    };

    struct View {
        Vector2<float> m_center;
        Vector2<float> m_size;
        float m_rotation;
        Rect<float> m_viewport;
        Transform m_transform;
        Transform m_inverseTransform;
        bool m_transformUpdated;
        bool m_invTransformUpdated;
    };

    struct String {
        std::string m_string;
    };

    struct alignas(8) RenderTexture { char padding[0x208]; };

    struct RenderStates { char padding[0x60]; };
}