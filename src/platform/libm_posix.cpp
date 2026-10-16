// The static i386 libm of glibc 2.39 exports __ieee754_fmod but not fmod, which FFmpeg's avutil references.
extern "C" double __ieee754_fmod(double x, double y);

extern "C" double fmod(double x, double y)
{
    return __ieee754_fmod(x, y);
}
