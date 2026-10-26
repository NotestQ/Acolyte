#pragma once
#include <vector>
#include <string>

// Definitions from SFML. Someone smarter than me can likely include SFML in the project? Maybe?
typedef unsigned int uint;
typedef unsigned __int64 ulong64;
typedef unsigned short ushort;
typedef unsigned char uchar;

namespace sf {
    struct RenderTarget;
    struct RenderWindow;
    struct Window;
    struct Font;
    struct TcpSocket;

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
        std::u32string m_string;
    };

    struct alignas(8) RenderTexture { char padding[0x208]; };

    struct RenderStates { char padding[0x60]; };
    namespace Keyboard {
        enum Key {
            Unknown = -1,
            A = 0,
            B = 1,
            C = 2,
            D = 3,
            E = 4,
            F = 5,
            G = 6,
            H = 7,
            I = 8,
            J = 9,
            K = 10,
            L = 11,
            M = 12,
            N = 13,
            O = 14,
            P = 15,
            Q = 16,
            R = 17,
            S = 18,
            T = 19,
            U = 20,
            V = 21,
            W = 22,
            X = 23,
            Y = 24,
            Z = 25,
            Num0 = 26,
            Num1 = 27,
            Num2 = 28,
            Num3 = 29,
            Num4 = 30,
            Num5 = 31,
            Num6 = 32,
            Num7 = 33,
            Num8 = 34,
            Num9 = 35,
            Escape = 36,
            LControl = 37,
            LShift = 38,
            LAlt = 39,
            LSystem = 40,
            RControl = 41,
            RShift = 42,
            RAlt = 43,
            RSystem = 44,
            Menu = 45,
            LBracket = 46,
            RBracket = 47,
            SemiColon = 48,
            Comma = 49,
            Period = 50,
            Quote = 51,
            Slash = 52,
            BackSlash = 53,
            Tilde = 54,
            Equal = 55,
            Dash = 56,
            Space = 57,
            Return = 58,
            BackSpace = 59,
            Tab = 60,
            PageUp = 61,
            PageDown = 62,
            End = 63,
            Home = 64,
            Insert = 65,
            Delete = 66,
            Add = 67,
            Subtract = 68,
            Multiply = 69,
            Divide = 70,
            Left = 71,
            Right = 72,
            Up = 73,
            Down = 74,
            Numpad0 = 75,
            Numpad1 = 76,
            Numpad2 = 77,
            Numpad3 = 78,
            Numpad4 = 79,
            Numpad5 = 80,
            Numpad6 = 81,
            Numpad7 = 82,
            Numpad8 = 83,
            Numpad9 = 84,
            F1 = 85,
            F2 = 86,
            F3 = 87,
            F4 = 88,
            F5 = 89,
            F6 = 90,
            F7 = 91,
            F8 = 92,
            F9 = 93,
            F10 = 94,
            F11 = 95,
            F12 = 96,
            F13 = 97,
            F14 = 98,
            F15 = 99,
            Pause = 100,
            KeyCount = 101,
        };
    }

    namespace Joystick {
        enum Axis {
            X = 0,
            Y = 1,
            Z = 2,
            R = 3,
            U = 4,
            V = 5,
            PovX = 6,
            PovY = 7,
        };
    }

    struct Drawable {
        void* vtable;
    };

    struct Text: public sf::Drawable, public sf::Transformable {
        sf::String m_string;
        sf::Font* m_font;
        uint m_characterSize;
        uint m_style;
        sf::Color m_fillColor;
        sf::Color m_outlineColor;
        float m_outlineThickness;
        sf::VertexArray m_vertices;
        sf::VertexArray m_outlineVertices;
        sf::Rect<float> m_bounds;
        bool m_geometryNeedUpdate;
    };

    struct Socket {
        enum Type { Tcp, Udp };
        void* vtable;
        Type m_type;
        uint m_socket;
        bool m_isBlocking;
    };

    struct UdpSocket: public sf::Socket {
        std::vector<char> m_buffer;
    };

    struct TcpListener: public sf::Socket {
    };

    struct IpAddress {
        uint m_address;
        bool m_valid;
    };

    struct Time {
        __int64 m_microseconds;
    };
}
