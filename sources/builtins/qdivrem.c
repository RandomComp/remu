#include "builtins/quad.h"

#define	B ((int32)1 << HALF_BITS)

#define	COMBINE(a, b) (((uint32)(a) << HALF_BITS) | (b))

static void shl(uint32 *p, int32 len, int32 sh) {
	int32 i;

	for (i = 0; i < len; i++) {
		p[i] = (uint32)(LHALF((uint32)p[i] << sh) |
			   ((uint32)p[i + 1] >> (HALF_BITS - sh)));
	}

	p[i] = (uint32)(LHALF((uint32)p[i] << sh));
}

uint64 __qdivrem(uint64 uq, uint64 vq, uint64 *arq) {
	union uu tmp;

	uint32 *u, *v, *q;

	uint32 v1, v2;

	uint32 qhat, rhat, t;

	int32 m, n, d, j, i;

	uint32 rbj;

	uint32 q1, q2, q3, q4;

	uint32 uspace[5], vspace[5], qspace[5];
	
	if (vq == 0) {
		static volatile const uint32 zero = 0;

		tmp.ul[H] = tmp.ul[L] = 1 / zero;

		if (arq) *arq = uq;

		return tmp.q;
	}

	if (uq < vq) {
		if (arq) *arq = uq;

		return 0;
	}

	u = &uspace[0];

	v = &vspace[0];

	q = &qspace[0];

	tmp.uq = uq;

	u[0] = 0;

	u[1] = (uint32)HHALF(tmp.ul[H]);

	u[2] = (uint32)LHALF(tmp.ul[H]);

	u[3] = (uint32)HHALF(tmp.ul[L]);

	u[4] = (uint32)LHALF(tmp.ul[L]);

	tmp.uq = vq;

	v[1] = (uint32)HHALF(tmp.ul[H]);

	v[2] = (uint32)LHALF(tmp.ul[H]);

	v[3] = (uint32)HHALF(tmp.ul[L]);

	v[4] = (uint32)LHALF(tmp.ul[L]);

	for (n = 4; v[1] == 0; v++) {
		if (--n == 1) {
			t = v[2];

			q1 = (uint32)(u[1] / t);

			rbj = COMBINE(u[1] % t, u[2]);

			q2 = (uint32)(rbj / t);

			rbj = COMBINE(rbj % t, u[3]);

			q3 = (uint32)(rbj / t);

			rbj = COMBINE(rbj % t, u[4]);

			q4 = (uint32)(rbj / t);

			if (arq) *arq = rbj % t;

			tmp.ul[H] = COMBINE(q1, q2);

			tmp.ul[L] = COMBINE(q3, q4);

			return tmp.q;
		}
	}
	
	for (m = 4 - n; u[1] == 0; u++)
		m--;

	for (i = 4 - m; --i >= 0;)
		q[i] = 0;

	q += 4 - m;
	
	d = 0;

	for (t = v[1]; t < B / 2; t <<= 1)
		d++;

	if (d > 0) {
		shl(&u[0], m + n, d);

		shl(&v[1], n - 1, d);
	}
	
	j = 0;

	v1 = v[1];

	v2 = v[2];

	do {
		uint32 uj0, uj1, uj2;
		
		uj0 = u[j + 0];

		uj1 = u[j + 1];

		uj2 = u[j + 2];

		if (uj0 == v1) {
			qhat = B;

			rhat = uj1;

			goto qhat_too_big;
		} 

		else {
			uint32 nn = COMBINE(uj0, uj1);
			qhat = nn / v1;
			rhat = nn % v1;
		}

		while (v2 * qhat > COMBINE(rhat, uj2)) {
			qhat_too_big:
				qhat--;

				if ((rhat += v1) >= B)
					break;
		}
		
		for (t = 0, i = n; i > 0; i--) {
			t = u[i + j] - v[i] * qhat - t;

			u[i + j] = (uint32)LHALF(t);

			t = (B - HHALF(t)) & (B - 1);
		}

		t = u[j] - t;

		u[j] = (uint32)LHALF(t);
		
		if (HHALF(t)) {
			qhat--;

			for (t = 0, i = n; i > 0; i--) {
				t += u[i + j] + v[i];

				u[i + j] = (uint32)LHALF(t);

				t = HHALF(t);
			}

			u[j] = (uint32)LHALF(u[j] + t);
		}

		q[j] = (uint32)qhat;
	} while (++j <= m);

	
	if (arq) {
		if (d) {
			for (i = m + n; i > m; --i) {
				u[i] = (uint32)(((uint32)u[i] >> d) |
					   LHALF((uint32)u[i - 1] << (HALF_BITS - d)));
			}

			u[i] = 0;
		}

		tmp.ul[H] = COMBINE(uspace[1], uspace[2]);

		tmp.ul[L] = COMBINE(uspace[3], uspace[4]);

		*arq = tmp.q;
	}

	tmp.ul[H] = COMBINE(qspace[1], qspace[2]);

	tmp.ul[L] = COMBINE(qspace[3], qspace[4]);

	return tmp.q;
}
