

	.section .text..SHmedia32,"ax"
	.little

	.balign 8
	.global clear_page
clear_page:
	pta/l 1f, tr1
	pta/l 2f, tr2
	ptabs/l r18, tr0

	movi 4096, r7
	add  r2, r7, r7
	add  r2, r63, r6
1:
	alloco r6, 0
	synco	! TAKum03020
	addi	r6, 32, r6
	bgt/l	r7, r6, tr1

	add  r2, r63, r6
2:
	st.q  r6,   0, r63
	st.q  r6,   8, r63
	st.q  r6,  16, r63
	st.q  r6,  24, r63
	addi r6, 32, r6
	bgt/l r7, r6, tr2

	blink tr0, r63


