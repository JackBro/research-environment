#ifndef _VECTOR3_H
#define _VECTOR3_H
#include <cmath>
#include <iosfwd>

template<typename T>
class Vector3 {
  T _x, _y, _z;
public:
  Vector3() : _x{}, _y{}, _z{} {}
  Vector3( T x_, T y_, T z_ ) : _x{x_}, _y{y_}, _z{z_} {}
  inline T x() const { return _x; }
  inline T y() const { return _y; }
  inline T z() const { return _z; }
  T norm() const;
  T normsq() const;
  friend std::istream& operator>>( std::istream &is, Vector3<T> &vec ) {
    is >> vec._x >> vec._y >> vec._z;
    return is;
  }
};

template<typename T>
inline T Vector3<T>::normsq() const {
  return _x * _x + _y * _y + _z * _z;
}

template<typename T>
inline T Vector3<T>::norm() const {
  return sqrt( normsq() );
}

template<>
inline float Vector3<float>::norm() const {
  return sqrtf( normsq() );
}

template<typename T>
inline const Vector3<T> operator+( const Vector3<T> &a, const Vector3<T> &b ) {
  return Vector3<T>{ a.x() + b.x(), a.y() + b.y(), a.z() + b.z() };
}

template<typename T>
inline const Vector3<T> operator-( const Vector3<T> &a, const Vector3<T> &b ) {
  return Vector3<T>{ a.x() - b.x(), a.y() - b.y(), a.z() - b.z() };
}

// Vector * scalar
template<typename T>
inline const Vector3<T> operator*( const Vector3<T> &a, T b ) {
  return Vector3<T>{ a.x() * b, a.y() * b, a.z() * b };
}

// scalar * Vector
template<typename T>
inline const Vector3<T> operator*( T a, const Vector3<T> &b ) {
  return Vector3<T>{ a * b.x(), a * b.y(), a * b.z() };
}

// Vector / scalar
template<typename T>
inline const Vector3<T> operator/( const Vector3<T> &a, T b ) {
  return Vector3<T>{ a.x() / b, a.y() / b, a.z() / b };
}

// Other useful math
template<typename T>
inline T dot( const Vector3<T> &a, const Vector3<T> &b ) {
  return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

template<typename T>
inline Vector3<T> cross( const Vector3<T> &a, const Vector3<T> &b ) {
  return Vector3<T>{ a.y() * b.z() - a.z() * b.y(),
                     a.z() * b.x() - a.x() * b.z(),
                     a.x() * b.y() - a.y() * b.x() };
}

template<typename T>
inline T cube( T x ) {
  return x * x * x;
}

template<typename T>
std::ostream& operator<<( std::ostream &os, const Vector3<T> &vec ) {
  os << vec.x() << " " << vec.y() << " " << vec.z();
  return os;
}

typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;
typedef Vector3<int> Vector3i;

#endif // _VECTOR3_H
