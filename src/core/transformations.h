#define _USE_MATH_DEFINES
#include <math.h>

struct Vec3
{
    float x, y, z;

    Vec3( float x = 0, float y = 0, float z = 0 ) : x( x ), y( y ), z( z ) {}
};

struct Mat4
{
    float m[ 4 ][ 4 ] = {}; // m[column][row]

    // Identity matrix initialization
    Mat4()
    {
        for( int i = 0; i < 4; ++i )
        {
            m[ i ][ i ] = 1.0f;
        }
    }

    // Translation constructor
    Mat4( const Vec3& translate ) : Mat4()
    {
        m[ 3 ][ 0 ] = translate.x;
        m[ 3 ][ 1 ] = translate.y;
        m[ 3 ][ 2 ] = translate.z;
    }

    // Matrix multiplication
    Mat4 operator*( const Mat4& other ) const
    {
        Mat4 result;
        for( int col = 0; col < 4; ++col )
        {
            for( int row = 0; row < 4; ++row )
            {
                result.m[ col ][ row ] = 0.0f;
                for( int k = 0; k < 4; ++k )
                {
                    result.m[ col ][ row ] += m[ k ][ row ] * other.m[ col ][ k ];
                }
            }
        }
        return result;
    }

};

class Transformations
{
private:
    static float degToRadians( float degrees )
    {
        return degrees * ( M_PI / 180.0f );
    }

public:
    enum class Axis
    {
        X = 0,
        Y = 1,
        Z = 2
    };

    static Mat4 Rotate( float angleInDegrees, Axis axis )
    {
        Mat4 rotationMatrix;
        float angleInRadians = degToRadians( angleInDegrees );

        switch( axis )
        {
        case Axis::X:
            // Y axis
            rotationMatrix.m[ 1 ][ 1 ] = cosf( angleInRadians );
            rotationMatrix.m[ 2 ][ 1 ] = -sinf( angleInRadians );

            // Z axis
            rotationMatrix.m[ 1 ][ 2 ] = sinf( angleInRadians );
            rotationMatrix.m[ 2 ][ 2 ] = cosf( angleInRadians );
            break;

        case Axis::Y:
            // X axis
            rotationMatrix.m[ 0 ][ 0 ] = cosf( angleInRadians );
            rotationMatrix.m[ 2 ][ 0 ] = sinf( angleInRadians );

            // Z axis
            rotationMatrix.m[ 0 ][ 2 ] = -sinf( angleInRadians );
            rotationMatrix.m[ 2 ][ 2 ] = cosf( angleInRadians );
            break;

        case Axis::Z:
            // X axis
            rotationMatrix.m[ 0 ][ 0 ] = cosf( angleInRadians );
            rotationMatrix.m[ 1 ][ 0 ] = -sinf( angleInRadians );

            // Y axis
            rotationMatrix.m[ 0 ][ 1 ] = sinf( angleInRadians );
            rotationMatrix.m[ 1 ][ 1 ] = cosf( angleInRadians );
            break;

        default:
            break;
        }

        return rotationMatrix;
    }

    static Mat4 Perspective( float fov, float aspect, float nearPlane, float farPlane )
    {
        Mat4 result;
        float fovInRadians = degToRadians( fov );
        float scaleFactor = 1.0f / tanf( fovInRadians / 2.0f );

        result.m[ 0 ][ 0 ] = scaleFactor / aspect;                                      // Horizontal scaling
        result.m[ 1 ][ 1 ] = scaleFactor;                                               // Vertical scaling
        result.m[ 2 ][ 2 ] = ( farPlane + nearPlane ) / ( nearPlane - farPlane );
        result.m[ 2 ][ 3 ] = -1.0f;
        result.m[ 3 ][ 2 ] = ( 2.0f * farPlane * nearPlane ) / ( nearPlane - farPlane );
        result.m[ 3 ][ 3 ] = 0.0f;

        return result;
    }
};