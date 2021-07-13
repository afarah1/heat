/*
 * Don't repeat yourself - Go-like parameters for C
 *
 * Before DRY:
 *
 * foo(int i, double x, double y, struct planet const *p1, struct planet const
 *     *p2, struct planet const *p3, int j)
 *
 * After DRY:
 *
 * foo(int i, DRY(double, x, y), DRY(struct planet const *, p1, p2, p3), int j)
 *
 * This version supports up to three parameters of the same type. It's simple
 * to expand it to whatever amount of parameters you need.
 */
#define DRY1(type, e) type e
#define DRY2(type, e, ...) type e, DRY1(type, __VA_ARGS__)
#define DRY3(type, e, ...) type e, DRY2(type, __VA_ARGS__)
#define DRY4(type, e, ...) type e, DRY3(type, __VA_ARGS__)
#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1, _2, _3, _4, N, ...) N
#define PP_RSEQ_N() 4, 3, 2, 1, 0
#define CAT2(a, b) a ## b
#define CAT(a, b) CAT2(a, b)
#define DRY(type, ...) CAT(DRY, PP_NARG(__VA_ARGS__))(type, __VA_ARGS__)
