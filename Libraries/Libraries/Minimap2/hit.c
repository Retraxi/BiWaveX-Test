#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mmpriv.h"
#include "kalloc.h"
#include "khash.h"

static inline void mm_cal_fuzzy_len(mm_reg1_t *r, const mm128_t *a)
{
	int i;
	r->mlen = r->blen = 0;
	if (r->cnt <= 0) return;
	r->mlen = r->blen = a[r->as].y>>32&0xff;
	for (i = r->as + 1; i < r->as + r->cnt; ++i) {
		int span = a[i].y>>32&0xff;
		int tl = (int32_t)a[i].x - (int32_t)a[i-1].x;
		int ql = (int32_t)a[i].y - (int32_t)a[i-1].y;
		r->blen += tl > ql? tl : ql;
		r->mlen += tl > span && ql > span? span : tl < ql? tl : ql;
	}
}

static inline void mm_reg_set_coor(mm_reg1_t *r, int32_t qlen, const mm128_t *a, int is_qstrand)
{ // NB: r->as and r->cnt MUST BE set correctly for this function to work
	int32_t k = r->as, q_span = (int32_t)(a[k].y>>32&0xff);
	r->rev = a[k].x>>63;
	r->rid = a[k].x<<1>>33;
	r->rs = (int32_t)a[k].x + 1 > q_span? (int32_t)a[k].x + 1 - q_span : 0; // NB: target span may be shorter, so this test is necessary
	r->re = (int32_t)a[k + r->cnt - 1].x + 1;
	if (!r->rev || is_qstrand) {
		r->qs = (int32_t)a[k].y + 1 - q_span;
		r->qe = (int32_t)a[k + r->cnt - 1].y + 1;
	} else {
		r->qs = qlen - ((int32_t)a[k + r->cnt - 1].y + 1);
		r->qe = qlen - ((int32_t)a[k].y + 1 - q_span);
	}
	mm_cal_fuzzy_len(r, a);
}

static inline uint64_t hash64(uint64_t key)
{
	key = (~key + (key << 21));
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8));
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4));
	key = key ^ key >> 28;
	key = (key + (key << 31));
	return key;
}

mm_reg1_t *mm_gen_regs(void *km, uint32_t hash, int qlen, int n_u, uint64_t *u, mm128_t *a, int is_qstrand) // convert chains to hits
{
	mm128_t *z, tmp;
	mm_reg1_t *r;
	int i, k;

	if (n_u <= 0) return 0;

	// sort by score
	z = (mm128_t*)kmalloc(km, n_u * 16);
	for (i = k = 0; i < n_u; ++i) {
		uint32_t h;
		h = (uint32_t)hash64((hash64(a[k].x) + hash64(a[k].y)) ^ hash);
		z[i].x = u[i] ^ h; // u[i] -- higher 32 bits: chain score; lower 32 bits: number of seeds in the chain
		z[i].y = (uint64_t)k << 32 | (int32_t)u[i];
		k += (int32_t)u[i];
	}
	radix_sort_128x(z, z + n_u);
	for (i = 0; i < n_u>>1; ++i) // reverse, s.t. larger score first
		tmp = z[i], z[i] = z[n_u-1-i], z[n_u-1-i] = tmp;

	// populate r[]
	r = (mm_reg1_t*)calloc(n_u, sizeof(mm_reg1_t));
	for (i = 0; i < n_u; ++i) {
		mm_reg1_t *ri = &r[i];
		ri->id = i;
		ri->parent = MM_PARENT_UNSET;
		ri->score = ri->score0 = z[i].x >> 32;
		ri->hash = (uint32_t)z[i].x;
		ri->cnt = (int32_t)z[i].y;
		ri->as = z[i].y >> 32;
		ri->div = -1.0f;
		mm_reg_set_coor(ri, qlen, a, is_qstrand);
	}
	kfree(km, z);
	return r;
}

void mm_mark_alt(const mm_idx_t *mi, int n, mm_reg1_t *r)
{
	int i;
	if (mi->n_alt == 0) return;
	for (i = 0; i < n; ++i)
		if (mi->seq[r[i].rid].is_alt)
			r[i].is_alt = 1;
}

static inline int mm_alt_score(int score, float alt_diff_frac)
{
	if (score < 0) return score;
	score = (int)(score * (1.0 - alt_diff_frac) + .499);
	return score > 0? score : 1;
}

void mm_split_reg(mm_reg1_t *r, mm_reg1_t *r2, int n, int qlen, mm128_t *a, int is_qstrand)
{
	if (n <= 0 || n >= r->cnt) return;
	*r2 = *r;
	r2->id = -1;
	r2->sam_pri = 0;
	r2->p = 0;
	r2->split_inv = 0;
	r2->cnt = r->cnt - n;
	r2->score = (int32_t)(r->score * ((float)r2->cnt / r->cnt) + .499);
	r2->as = r->as + n;
	if (r->parent == r->id) r2->parent = MM_PARENT_TMP_PRI;
	mm_reg_set_coor(r2, qlen, a, is_qstrand);
	r->cnt -= r2->cnt;
	r->score -= r2->score;
	mm_reg_set_coor(r, qlen, a, is_qstrand);
	r->split |= 1, r2->split |= 2;
}

void mm_set_parent(void *km, float mask_level, int mask_len, int n, mm_reg1_t *r, int sub_diff, int hard_mask_level, float alt_diff_frac) // and compute mm_reg1_t::subsc
{
	int i, j, k, *w;
	uint64_t *cov;
	if (n <= 0) return;
	for (i = 0; i < n; ++i) r[i].id = i;
	cov = (uint64_t*)kmalloc(km, n * sizeof(uint64_t));
	w = (int*)kmalloc(km, n * sizeof(int));
	w[0] = 0, r[0].parent = 0;
	for (i = 1, k = 1; i < n; ++i) {
		mm_reg1_t *ri = &r[i];
		int si = ri->qs, ei = ri->qe, n_cov = 0, uncov_len = 0;
		if (hard_mask_level) goto skip_uncov;
		for (j = 0; j < k; ++j) { // traverse existing primary hits to find overlapping hits
			mm_reg1_t *rp = &r[w[j]];
			int sj = rp->qs, ej = rp->qe;
			if (ej <= si || sj >= ei) continue;
			if (sj < si) sj = si;
			if (ej > ei) ej = ei;
			cov[n_cov++] = (uint64_t)sj<<32 | ej;
		}
		if (n_cov == 0) {
			goto set_parent_test; // no overlapping primary hits; then i is a new primary hit
		} else if (n_cov > 0) { // there are overlapping primary hits; find the length not covered by existing primary hits
			int j, x = si;
			radix_sort_64(cov, cov + n_cov);
			for (j = 0; j < n_cov; ++j) {
				if ((int)(cov[j]>>32) > x) uncov_len += (cov[j]>>32) - x;
				x = (int32_t)cov[j] > x? (int32_t)cov[j] : x;
			}
			if (ei > x) uncov_len += ei - x;
		}
skip_uncov:
		for (j = 0; j < k; ++j) { // traverse existing primary hits again
			mm_reg1_t *rp = &r[w[j]];
			int sj = rp->qs, ej = rp->qe, min, max, ol;
			if (ej <= si || sj >= ei) continue; // no overlap
			min = ej - sj < ei - si? ej - sj : ei - si;
			max = ej - sj > ei - si? ej - sj : ei - si;
			ol = si < sj? (ei < sj? 0 : ei < ej? ei - sj : ej - sj) : (ej < si? 0 : ej < ei? ej - si : ei - si); // overlap length; TODO: this can be simplified
			if ((float)ol / min - (float)uncov_len / max > mask_level && uncov_len <= mask_len) { // then this is a secondary hit
				int cnt_sub = 0, sci = ri->score;
				ri->parent = rp->parent;
				if (!rp->is_alt && ri->is_alt) sci = mm_alt_score(sci, alt_diff_frac);
				rp->subsc = rp->subsc > sci? rp->subsc : sci;
				if (ri->cnt >= rp->cnt) cnt_sub = 1;
				if (rp->p && ri->p && (rp->rid != ri->rid || rp->rs != ri->rs || rp->re != ri->re || ol != min)) { // the last condition excludes identical hits after DP
					sci = ri->p->dp_max;
					if (!rp->is_alt && ri->is_alt) sci = mm_alt_score(sci, alt_diff_frac);
					rp->p->dp_max2 = rp->p->dp_max2 > sci? rp->p->dp_max2 : sci;
					if (rp->p->dp_max - ri->p->dp_max <= sub_diff) cnt_sub = 1;
				}
				if (cnt_sub) ++rp->n_sub;
				break;
			}
		}
set_parent_test:
		if (j == k) w[k++] = i, ri->parent = i, ri->n_sub = 0;
	}
	kfree(km, cov);
	kfree(km, w);
}

void mm_hit_sort(void *km, int *n_regs, mm_reg1_t *r, float alt_diff_frac)
{
	int32_t i, n_aux, n = *n_regs, has_cigar = 0, no_cigar = 0;
	mm128_t *aux;
	mm_reg1_t *t;

	if (n <= 1) return;
	aux = (mm128_t*)kmalloc(km, n * 16);
	t = (mm_reg1_t*)kmalloc(km, n * sizeof(mm_reg1_t));
	for (i = n_aux = 0; i < n; ++i) {
		if (r[i].inv || r[i].cnt > 0) { // squeeze out elements with cnt==0 (soft deleted)
			int score;
			if (r[i].p) score = r[i].p->dp_max, has_cigar = 1;
			else score = r[i].score, no_cigar = 1;
			if (r[i].is_alt) score = mm_alt_score(score, alt_diff_frac);
			aux[n_aux].x = (uint64_t)score << 32 | r[i].hash;
			aux[n_aux++].y = i;
		} else if (r[i].p) {
			free(r[i].p);
			r[i].p = 0;
		}
	}
	assert(has_cigar + no_cigar == 1);
	radix_sort_128x(aux, aux + n_aux);
	for (i = n_aux - 1; i >= 0; --i)
		t[n_aux - 1 - i] = r[aux[i].y];
	memcpy(r, t, sizeof(mm_reg1_t) * n_aux);
	*n_regs = n_aux;
	kfree(km, aux);
	kfree(km, t);
}

int mm_set_sam_pri(int n, mm_reg1_t *r)
{
	int i, n_pri = 0;
	for (i = 0; i < n; ++i)
		if (r[i].id == r[i].parent) {
			++n_pri;
			r[i].sam_pri = (n_pri == 1);
		} else r[i].sam_pri = 0;
	return n_pri;
}

void mm_sync_regs(void *km, int n_regs, mm_reg1_t *regs) // keep mm_reg1_t::{id,parent} in sync; also reset id
{
	int *tmp, i, max_id = -1, n_tmp;
	if (n_regs <= 0) return;
	for (i = 0; i < n_regs; ++i) // NB: doesn't work if mm_reg1_t::id is negative
		max_id = max_id > regs[i].id? max_id : regs[i].id;
	n_tmp = max_id + 1;
	tmp = (int*)kmalloc(km, n_tmp * sizeof(int));
	for (i = 0; i < n_tmp; ++i) tmp[i] = -1;
	for (i = 0; i < n_regs; ++i)
		if (regs[i].id >= 0) tmp[regs[i].id] = i;
	for (i = 0; i < n_regs; ++i) {
		mm_reg1_t *r = &regs[i];
		r->id = i;
		if (r->parent == MM_PARENT_TMP_PRI)
			r->parent = i;
		else if (r->parent >= 0 && tmp[r->parent] >= 0)
			r->parent = tmp[r->parent];
		else r->parent = MM_PARENT_UNSET;
	}
	kfree(km, tmp);
	mm_set_sam_pri(n_regs, regs);
}

void mm_select_sub(void *km, float pri_ratio, int min_diff, int best_n, int check_strand, int min_strand_sc, int *n_, mm_reg1_t *r)
{
	if (pri_ratio > 0.0f && *n_ > 0) {
		int i, k, n = *n_, n_2nd = 0;
		for (i = k = 0; i < n; ++i) {
			int p = r[i].parent;
			if (p == i || r[i].inv) { // primary or inversion
				r[k++] = r[i];
			} else if ((r[i].score >= r[p].score * pri_ratio || r[i].score + min_diff >= r[p].score) && n_2nd < best_n) {
				if (!(r[i].qs == r[p].qs && r[i].qe == r[p].qe && r[i].rid == r[p].rid && r[i].rs == r[p].rs && r[i].re == r[p].re)) // not identical hits
					r[k++] = r[i], ++n_2nd;
				else if (r[i].p) free(r[i].p);
			} else if (check_strand && n_2nd < best_n && r[i].score > min_strand_sc && r[i].rev != r[p].rev) {
				r[i].strand_retained = 1;
				r[k++] = r[i], ++n_2nd;
			} else if (r[i].p) free(r[i].p);
		}
		if (k != n) mm_sync_regs(km, k, r); // removing hits requires sync()
		*n_ = k;
	}
}

int mm_filter_strand_retained(int n_regs, mm_reg1_t *r)
{
	int i, k;
	for (i = k = 0; i < n_regs; ++i) {
		int p = r[i].parent;
		if (!r[i].strand_retained || r[i].div < r[p].div * 5.0f || r[i].div < 0.01f) {
			if (k < i) r[k++] = r[i];
			else ++k;
		}
	}
	return k;
}

void mm_filter_regs(const mm_mapopt_t *opt, int qlen, int *n_regs, mm_reg1_t *regs)
{ // NB: after this call, mm_reg1_t::parent can be -1 if its parent filtered out
	int i, k;
	for (i = k = 0; i < *n_regs; ++i) {
		mm_reg1_t *r = &regs[i];
		int flt = 0;
		if (!r->inv && !r->seg_split && r->cnt < opt->min_cnt) flt = 1;
		if (r->p) { // these filters are only applied when base-alignment is available
			if (r->mlen < opt->min_chain_score) flt = 1;
			else if (r->p->dp_max < opt->min_dp_max) flt = 1;
			else if (r->qs > qlen * opt->max_clip_ratio && qlen - r->qe > qlen * opt->max_clip_ratio) flt = 1;
			if (flt) free(r->p);
		}
		if (!flt) {
			if (k < i) regs[k++] = regs[i];
			else ++k;
		}
	}
	*n_regs = k;
}

int mm_squeeze_a(void *km, int n_regs, mm_reg1_t *regs, mm128_t *a)
{ // squeeze out regions in a[] that are not referenced by regs[]
	int i, as = 0;
	uint64_t *aux;
	aux = (uint64_t*)kmalloc(km, n_regs * 8);
	for (i = 0; i < n_regs; ++i)
		aux[i] = (uint64_t)regs[i].as << 32 | i;
	radix_sort_64(aux, aux + n_regs);
	for (i = 0; i < n_regs; ++i) {
		mm_reg1_t *r = &regs[(int32_t)aux[i]];
		if (r->as != as) {
			memmove(&a[as], &a[r->as], r->cnt * 16);
			r->as = as;
		}
		as += r->cnt;
	}
	kfree(km, aux);
	return as;
}

mm_seg_t *mm_seg_gen(void *km, uint32_t hash, int n_segs, const int *qlens, int n_regs0, const mm_reg1_t *regs0, int *n_regs, mm_reg1_t **regs, const mm128_t *a)
{
	int s, i, j, acc_qlen[MM_MAX_SEG+1], qlen_sum = 0;
	mm_seg_t *seg;

	assert(n_segs <= MM_MAX_SEG);
	for (s = 1, acc_qlen[0] = 0; s < n_segs; ++s)
		acc_qlen[s] = acc_qlen[s-1] + qlens[s-1];
	qlen_sum = acc_qlen[n_segs - 1] + qlens[n_segs - 1];

	seg = (mm_seg_t*)kcalloc(km, n_segs, sizeof(mm_seg_t));
	for (s = 0; s < n_segs; ++s) {
		seg[s].u = (uint64_t*)kmalloc(km, n_regs0 * 8);
		for (i = 0; i < n_regs0; ++i)
			seg[s].u[i] = (uint64_t)regs0[i].score << 32;
	}
	for (i = 0; i < n_regs0; ++i) {
		const mm_reg1_t *r = &regs0[i];
		for (j = 0; j < r->cnt; ++j) {
			int sid = (a[r->as + j].y&MM_SEED_SEG_MASK)>>MM_SEED_SEG_SHIFT;
			++seg[sid].u[i];
			++seg[sid].n_a;
		}
	}
	for (s = 0; s < n_segs; ++s) {
		mm_seg_t *sr = &seg[s];
		for (i = 0, sr->n_u = 0; i < n_regs0; ++i) // squeeze out zero-length per-segment chains
			if ((int32_t)sr->u[i] != 0)
				sr->u[sr->n_u++] = sr->u[i];
		sr->a = (mm128_t*)kmalloc(km, sr->n_a * sizeof(mm128_t));
		sr->n_a = 0;
	}

	for (i = 0; i < n_regs0; ++i) {
		const mm_reg1_t *r = &regs0[i];
		for (j = 0; j < r->cnt; ++j) {
			int sid = (a[r->as + j].y&MM_SEED_SEG_MASK)>>MM_SEED_SEG_SHIFT;
			mm128_t a1 = a[r->as + j];
			// on reverse strand, the segment position is:
			//   x_for_cat = qlen_sum - 1 - (int32_t)a1.y - 1 + q_span
			//   (int32_t)new_a1.y = qlens[sid] - (x_for_cat - acc_qlen[sid] + 1 - q_span) - 1 = (int32_t)a1.y - (qlen_sum - (qlens[sid] + acc_qlen[sid]))
			a1.y -= a1.x>>63? qlen_sum - (qlens[sid] + acc_qlen[sid]) : acc_qlen[sid];
			seg[sid].a[seg[sid].n_a++] = a1;
		}
	}
	for (s = 0; s < n_segs; ++s) {
		regs[s] = mm_gen_regs(km, hash, qlens[s], seg[s].n_u, seg[s].u, seg[s].a, 0);
		n_regs[s] = seg[s].n_u;
		for (i = 0; i < n_regs[s]; ++i) {
			regs[s][i].seg_split = 1;
			regs[s][i].seg_id = s;
		}
	}
	return seg;
}

void mm_seg_free(void *km, int n_segs, mm_seg_t *segs)
{
	int i;
	for (i = 0; i < n_segs; ++i) kfree(km, segs[i].u);
	for (i = 0; i < n_segs; ++i) kfree(km, segs[i].a);
	kfree(km, segs);
}

static void mm_set_inv_mapq(void *km, int n_regs, mm_reg1_t *regs)
{
	int i, n_aux;
	mm128_t *aux;
	if (n_regs < 3) return;
	for (i = 0; i < n_regs; ++i)
		if (regs[i].inv) break;
	if (i == n_regs) return; // no inversion hits

	aux = (mm128_t*)kmalloc(km, n_regs * 16);
	for (i = n_aux = 0; i < n_regs; ++i)
		if (regs[i].parent == i || regs[i].parent < 0)
			aux[n_aux].y = i, aux[n_aux++].x = (uint64_t)regs[i].rid << 32 | regs[i].rs;
	radix_sort_128x(aux, aux + n_aux);

	for (i = 1; i < n_aux - 1; ++i) {
		mm_reg1_t *inv = &regs[aux[i].y];
		if (inv->inv) {
			mm_reg1_t *l = &regs[aux[i-1].y];
			mm_reg1_t *r = &regs[aux[i+1].y];
			inv->mapq = l->mapq < r->mapq? l->mapq : r->mapq;
		}
	}
	kfree(km, aux);
}

void mm_set_mapq2(void *km, int n_regs, mm_reg1_t *regs, int min_chain_sc, int match_sc, int rep_len, int is_sr, int is_splice)
{
	static const float q_coef = 40.0f;
	int64_t sum_sc = 0;
	float uniq_ratio;
	int i, n_2nd_splice = 0;
	if (n_regs == 0) return;
	for (i = 0; i < n_regs; ++i) {
		if (regs[i].parent == regs[i].id)
			sum_sc += regs[i].score;
		else if (regs[i].is_spliced)
			++n_2nd_splice;
	}
	uniq_ratio = (float)sum_sc / (sum_sc + rep_len);
	for (i = 0; i < n_regs; ++i) {
		mm_reg1_t *r = &regs[i];
		if (r->inv) {
			r->mapq = 0;
		} else if (r->parent == r->id) {
			int mapq, subsc;
			float pen_s1 = (r->score > 100? 1.0f : 0.01f * r->score) * uniq_ratio;
			float pen_cm = r->cnt > 10? 1.0f : 0.1f * r->cnt;
			pen_cm = pen_s1 < pen_cm? pen_s1 : pen_cm;
			subsc = r->subsc > min_chain_sc? r->subsc : min_chain_sc;
			if (r->p && r->p->dp_max2 > 0 && r->p->dp_max > 0) {
				float x, identity = (float)r->mlen / r->blen;
				if (is_sr && is_splice)
					x = (float)r->p->dp_max2 / r->p->dp_max; // ignore chaining score; for short RNA-seq reads, unspliced chaining score tends to be higher
				else
					x = (float)r->p->dp_max2 * subsc / r->p->dp_max / r->score0;
				mapq = (int)(identity * pen_cm * q_coef * (1.0f - x * x) * logf((float)r->p->dp_max / match_sc));
				if (!is_sr) {
					int mapq_alt = (int)(6.02f * identity * identity * (r->p->dp_max - r->p->dp_max2) / match_sc + .499f); // BWA-MEM like mapQ, mostly for short reads
					mapq = mapq < mapq_alt? mapq : mapq_alt; // in case the long-read heuristic fails
				}
				if (is_splice && is_sr && r->is_spliced && n_2nd_splice == 0)
					mapq += 10;
			} else {
				float x = (float)subsc / r->score0;
				if (r->p) {
					float identity = (float)r->mlen / r->blen;
					mapq = (int)(identity * pen_cm * q_coef * (1.0f - x) * logf((float)r->p->dp_max / match_sc));
				} else {
					mapq = (int)(pen_cm * q_coef * (1.0f - x) * logf(r->score));
				}
			}
			mapq -= (int)(4.343f * logf(r->n_sub + 1) + .499f);
			mapq = mapq > 0? mapq : 0;
			r->mapq = mapq < 60? mapq : 60;
			if (r->p && r->p->dp_max > r->p->dp_max2 && r->mapq == 0) r->mapq = 1;
		} else r->mapq = 0;
	}
	mm_set_inv_mapq(km, n_regs, regs);
}
