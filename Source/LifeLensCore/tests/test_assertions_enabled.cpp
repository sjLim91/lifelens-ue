// A Release test build must never silently compile away assert expressions.
#ifdef NDEBUG
#error "LifeLens Core tests require assertions, including in Release builds"
#endif

#include <cassert>

int main()
{
    bool evaluated=false;
    assert((evaluated=true));
    return evaluated ? 0 : 1;
}
