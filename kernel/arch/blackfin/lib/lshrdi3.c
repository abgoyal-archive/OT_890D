

#define BITS_PER_UNIT 8

typedef int SItype __attribute__ ((mode(SI)));
typedef unsigned int USItype __attribute__ ((mode(SI)));
typedef int DItype __attribute__ ((mode(DI)));
typedef int word_type __attribute__ ((mode(__word__)));

struct DIstruct {
	SItype high, low;
};

typedef union {
	struct DIstruct s;
	DItype ll;
} DIunion;

#ifdef CONFIG_ARITHMETIC_OPS_L1
DItype __lshrdi3(DItype u, word_type b)__attribute__((l1_text));
#endif

DItype __lshrdi3(DItype u, word_type b)
{
	DIunion w;
	word_type bm;
	DIunion uu;

	if (b == 0)
		return u;

	uu.ll = u;

	bm = (sizeof(SItype) * BITS_PER_UNIT) - b;
	if (bm <= 0) {
		w.s.high = 0;
		w.s.low = (USItype) uu.s.high >> -bm;
	} else {
		USItype carries = (USItype) uu.s.high << bm;
		w.s.high = (USItype) uu.s.high >> b;
		w.s.low = ((USItype) uu.s.low >> b) | carries;
	}

	return w.ll;
}
