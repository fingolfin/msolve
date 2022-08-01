/* This file is part of msolve.
 *
 * msolve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * msolve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with msolve.  If not, see <https://www.gnu.org/licenses/>
 *
 * Authors:
 * Jérémy Berthomieu
 * Christian Eder
 * Mohab Safey El Din */


#include "sba.h"

static inline crit_t *initialize_signature_criteria(
        const stat_t * const st
        )
{
    crit_t *crit    =   calloc((unsigned long)st->ngens, sizeof(crit_t));

    return crit;
}

static inline void free_signature_criteria_data(
        crit_t *crit,
        const stat_t * const st
        )
{
    for (len_t i = 0; i < st->ngens; ++i) {
        free(crit[i].sdm);
        crit[i].sdm =   NULL;
        free(crit[i].hm);
        crit[i].hm  =   NULL;
        crit[i].ld  =   0;
    }
}

static inline void free_signature_criteria(
        crit_t **critp,
        const stat_t * const st
        )
{
    crit_t *crit    =   *critp;
    free_signature_criteria_data(crit, st);
    free(crit);
    crit    =   NULL;

    *critp  =   crit;
}

static len_t sba_add_new_elements_to_basis(
        const smat_t * const smat,
        const ht_t * const ht,
        bs_t *bs,
        const stat_t * const st
        )
{
    len_t i, j, k, ne;

    const len_t nr  = smat->ld;
    const len_t bld = bs->ld;

    bs->lo = bld;

    /* row indices of new elements are getting precached */
    len_t *rine = calloc((unsigned long)smat->ld, sizeof(len_t));

    k = 0;
    i = 0;
next:
    for (; i < ld; ++i) {
        const hm_t lm = smat->cols[i][SM_OFFSET];
        for (j = 0; j < bld; ++j) {
            if (check_monomial_division(bs->hm[j][SM_OFFSET], lm, ht) == 1) {
                goto next;
            }
        }
        rine[k++] = i;
    }
    check_enlarge_basis(bs, k, st);

    /* now enter elements to basis */
    for (i = 0; i < k; ++i) {
        bs->hm[bs->ld] = (hm_t *)malloc(
                (unsigned long)(smat->cols[rine[i]i][SM_LEN])+SM_OFFSET *
                sizeof(hm_t));
        memcpy(bs->hm[bs->ld], sat->cols[rine[i]],
                (unsigned long)(smat->cols[rine[i]][SM_LEN])+SM_OFFSET *
                sizeof(hm_t));
        bs->cf_32[bs->ld] = (cf32_t *)malloc(
                (unsigned long)(smat->cols[rine[i]][SM_LEN]) *
                sizeof(cf32_t));
        memcpy(bs->cf_32[bs->ld], sat->cur_cf32[rine[i]],
                (unsigned long)(smat->cols[rine[i]][SM_LEN]) * sizeof(cf32_t));
        bs->ld++;
    }

    free(rine);
    rine = NULL;

    /* check number of new elements here, later on bs->ld might change */
    ne = k;

    /* If the input is NOT homogeneous, check if the new elements in
     * the basis makes older elements redundant. */
    if (st->homogeneous != 1) {
        /* Note: Now, bs->ld is the new load of the basis after adding
         * new elements, bld was set before thus it represents the old
         * load of the basis. */
        const len_t bln = bs->ld;
        k = 0;
        for (i = 0; i < bld; ++i) {
            for (j = blo; j < bln; ++j) {
                const hm_t lm = bs->hm[i][SM_OFFSET];
                if (check_monomial_division(bs->hm[j][SM_OFFSET], lm, ht) == 1) {
                    free(bs->hm[i]);
                    bs->hm[i] = NULL;
                    free(bs->cf_32[i]);
                    bs->cf_32[i] = NULL;
                }
                bs->hm[k]    = bs->hm[i];
                bs->cf_32[k] = bs->cf_32[i];
                k++;
            }
        }
        /* now add new elements correctly in minimized basis */
        for (i = blo; i < bln; ++i) {
            bs->hm[k]    = bs->hm[i];
            bs->cf_32[k] = bs->cf_32[i];
            k++;
        }
        bs->ld = k;
    }
    /* TODO: check divisibility and set lml, etc. correctly in here */

    return ne;
}

static int is_signature_needed(
        const smat_t * const smat,
        const crit_t * const syz,
        const crit_t * const rew,
        const smat_t *psmat,
        const len_t idx,
        const len_t var_idx,
        ht_t *ht
        )
{
    /* get exponent vector and increment entry for var_idx */
    exp_t *ev   =   ht->ev[0];
    ev          =   ht->ev[psmat->cols[idx][SM_SMON]];
    /* Note: ht->ebl = #elimination variables + 1 */
    len_t shift =   var_idx < ht->ebl - 1 ? 1: 2;
    ev[var_idx+shift]++;

    const len_t sig_idx =   psmat->cols[idx][SM_SIDX];
    const hm_t hm       =   insert_in_hash_table(ev, ht);
    const sdm_t nsdm    =   ~ht->hd[hm].sdm;

    const len_t evl     =   ht->evl;

    /* syzygy criterion */
    const crit_t syz_idx =   syz[sig_idx];
    for (len_t i = 0; i < syz_idx.ld; ++i) {
        if (nsdm & syz_idx.sdm[i]) {
            continue;
        }
        const exp_t *sev    =   ht->ev[syz_idx.hm[i]];
        for (len_t j = 0; j < evl; ++j) {
            if (sev[j] > ev[j]) {
                continue;
            }
        }
        return 0;
    }
    /* rewrite criterion */
    const crit_t rew_idx =   rew[sig_idx];
    for (len_t i = 0; i < rew_idx.ld; ++i) {
        if (nsdm & rew_idx.sdm[i]) {
            continue;
        }
        const exp_t *sev    =   ht->ev[rew_idx.hm[i]];
        for (len_t j = 0; j < evl; ++j) {
            if (sev[j] > ev[j]) {
                continue;
            }
        }
        return 0;
    }
    return 1;
}

static inline void enlarge_signature_matrix(
        smat_t *smat
        )
{
        smat->sz *= 2;
        smat->cols = realloc(
                smat->cols, (unsigned long)smat->sz * sizeof(hm_t *));
        smat->curr_cf32 = realloc(
                smat->curr_cf32, (unsigned long)smat->sz * sizeof(cf32_t *));
        smat->prev_cf32 = realloc(
                smat->prev_cf32, (unsigned long)smat->sz * sizeof(cf32_t *));
}

static inline void check_enlarge_rewrite_rule_array(
        crit_t *rew,
        const len_t sidx
        )
{
    if (rew[sidx].ld >= rew[sidx].sz) {
        rew[sidx].sz    *=  2;
        rew[sidx].sdm   =   realloc(rew[sidx].sdm,
                (unsigned long)rew[sidx].sz * sizeof(sdm_t));
        rew[sidx].hm    =   realloc(rew[sidx].hm,
                (unsigned long)rew[sidx].sz * sizeof(hm_t));
    }
}

static inline void add_rewrite_rule(
        crit_t *rew,
        const smat_t * const smat,
        const ht_t * const ht
        )
{
    const len_t sidx    =   smat->cols[smat->ld-1][SM_SIDX];
    check_enlarge_rewrite_rule_array(rw, sidx);
    rew[sidx].hm[rew[sidx].ld]  =   smat->cols[smat->ld-1][SM_SMON];
    rew[sidx].sdm[rew[sidx].ld] =   ht->hd[smat->cols[smat->ld-1][SM_SMON]].sdm;
    rew[sidx].ld++;
}

static void add_row_to_signature_matrix(
        smat_t *smat,
        const smat_t * const psmat,
        const len_t idx,
        const len_t var_idx,
        ht_t *ht
        )
{
    if (smat->ld >= smat->sz) {
        enlarge_signature_matrix(smat);
    }
    const len_t ld      =   smat->ld;
    smat->prev_cf32[ld] =   psmat->curr_cf32[idx];
    smat->curr_cf32[ld] =   NULL;
    /* copy monomial entries in row */
    smat->cols[ld]  =   malloc(
            ((unsigned long)psmat->cols[idx][SM_LEN]+SM_OFFSET) * sizeof(hm_t));
    memcpy(smat->cols[ld], psmat->cols[idx],
            ((unsigned long)psmat->cols[idx][SM_LEN]+SM_OFFSET) * sizeof(hm_t));

    /* now multiply each column entry with the corresponding variable */
    hm_t *cols          =   smat->cols[ld];
    exp_t *ev           =   ht->ev[0];
    const len_t shift   =   var_idx > ht->ebl ? 2 : 1;

    /* multiply signature */
    ev              =   ht->ev[cols[SM_SMON]];
    ev[var_idx+shift]++;
    cols[SM_SMON]   =   insert_in_hash_table(ev, ht);

    /* multiply monomials in corresp. polnoymial */
    const len_t len =   cols[SM_LEN] + SM_OFFSET;
    for (len_t i = SM_OFFSET; i < len; ++i) {
        ev      =   ht->ev[cols[i]];
        ev[var_idx+shift]++;
        cols[i] =   insert_in_hash_table(ev, ht);
    }
    smat->ld++;
}

static void add_multiples_of_previous_degree_row(
        smat_t *smat,
        smat_t *psmat,
        const len_t idx,
        const crit_t * const syz,
        crit_t *rew,
        ht_t *ht,
        stat_t *st)
{
    const len_t nv  =   ht->nv;

    free_signature_criteria_data(rew, st);
    for (len_t i = 0; i < nv; ++i) {
        /* check syzygy and rewrite criterion */
        if (is_signature_needed(smat, syz, rew, psmat, idx, i, ht) == 1) {
            add_row_to_signature_matrix(smat, psmat, idx, i, ht);
            /* add rewrite rule */
            add_rewrite_rule(rew, smat, ht);
        }
    }
}

static inline void add_row_with_signature(
        smat_t *smat,
        const bs_t * const bs,
        const len_t pos
        )
{
    const len_t ld          =   smat->ld;
    const unsigned long len =   bs->hm[pos][LENGTH];
    smat->cols[ld]           =   (hm_t *)malloc(
            (len + SM_OFFSET) * sizeof(hm_t));
    /* copy polynomial data */
    memcpy(smat->cols[ld]+SM_CFS,bs->hm[pos]+COEFFS,
            len + OFFSET - COEFFS);
    smat->prev_cf32[ld]      =   bs->cf_32[bs->hm[pos][COEFFS]];
    smat->cols[ld][SM_CFS]   =   ld;
    /* store also signature data */
    smat->cols[ld][SM_SMON]  =   bs->sm[pos];
    smat->cols[ld][SM_SIDX]  =   bs->si[pos];
    smat->ld++;
}

inline void add_syzygy_schreyer(
        crit_t *syz,
        const hm_t sm,
        const len_t si,
        const ht_t * const ht
        )
{
    while (syz[si].ld >= syz[si].sz) {
        syz[si].sz *= 2;
        syz[si].hm = realloc(syz[si].hm,
                (unsigned long)syz[si].sz * sizeof(hm_t));
        syz[si].sdm = realloc(syz[si].sdm,
                (unsigned long)syz[si].sz * sizeof(sdm_t));
    }
    syz[si].hm[syz[si].ld]  = sm;
    syz[si].sdm[syz[si].ld] = ht->hd[sm].sdm;
    syz[si].ld++;
}

static inline crit_t *initialize_syzygies_schreyer(
        const bs_t * const bs,
        ht_t *ht
        )
{
    /* when initializing syzygies we assume that bs->ld == st->ngens */
    crit_t *syz =   calloc((unsigned long)bs->ld, sizeof(crit_t));
    syz[0].ld   =   0;
    syz[0].sz   =   0;
    for (len_t i = 1; i < bs->ld; ++i) {
        syz[i].hm   =   calloc((unsigned long)i, sizeof(hm_t));
        syz[i].sdm  =   calloc((unsigned long)i, sizeof(sdm_t));
        syz[i].ld   =   0;
        syz[i].sz   =   i;
        for (len_t j = 0; j < i; ++j) {
            syz[i].hm[j]    = insert_multiplied_signature_in_hash_table(
                    bs->hm[j][OFFSET], bs->sm[i], ht);
            syz[i].sdm[j]   = ht->hd[syz[i].hm[j]].sdm;
        }
    }
    return syz;
}

static inline void initialize_signatures_schreyer(
        bs_t *bs
        )
{
    for (len_t i = 0; i < bs->ld; ++i) {
        bs->si[i]   =   i;
        bs->sm[i]   =   bs->hm[i][OFFSET];
    }
}

static inline void initialize_signatures_not_schreyer(
        bs_t *bs
        )
{
    for (len_t i = 0; i < bs->ld; ++i) {
        bs->si[i]   =   i;
        bs->sm[i]   =   0;
    }
}

static void sba_prepare_next_degree(
        smat **psmatp,
        smat_t **smatp,
        const stat_t * const st)
{
    smat_t *psmat = *psmatp;
    smat_t *smat  = *smatp;
    psmat = smat;

    /* reset smat data */
    smat->cols      = NULL;
    smat->prev_cf32 = NULL;
    smat->curr_cf32 = NULL;
    smat->ld = smat->sz = smat->nz = smat->nc = 0;

    smat->sz        = psmat->ld * st->nvars + in->ld;
    smat->cols      = (hm_t **)calloc(
            (unsigned long)smat->sz,sizeof(hm_t *));
    smat->prev_cf32 = (cf32_t **)calloc(
            (unsigned long)psmat->ld,sizeof(cf32_t *));

    *psmatp = psmat;
    *smatp  = smat;
}

static void check_initial_generators(
        smat_t *smat,
        bs_t *in
        )
{
    while (in->ld > 0 && in->hm[in->ld-1][DEG] == next_degree) {
        add_row_with_signature(smat, in, in->ld-1);
        free(in->hm[in->ld]);
        in->hm[in->ld] = NULL;
        free(in->cf_32[in->ld]);
        in->cf_32[in->ld] = NULL;
        in->ld--;
    }
}

static void generate_next_degree_matrix(
        smat_t *smat,
        smat_t *psmat,
        const crit_t * const syz,
        crit_t *rew,
        ht_t *ht,
        stat_t *st
        )
{
    for (len_t i = psmat->ld; i > 0 ; --i) {
        add_multiples_of_previous_degree_row(
                smat, psmat, i-1, syz, rew, ht, st);
        free(psmat->cols[i-1]);
        psmat->cols[i-1]  =   NULL;
    }
}

int core_sba_schreyer(
        bs_t **bsp,
        ht_t **htp,
        stat_t **stp
        )
{
    bs_t *in    = *bsp;
    ht_t *ht    = *htp;
    stat_t *st  = *stp;

    /* timings for one round */
    double rrt0, rrt1;

    int try_termination =   0;
    /* hashes-to-columns map, initialized with length 1, is reallocated
     * in each call when generating matrices for linear algebra */
    hi_t *hcm = (hi_t *)malloc(sizeof(hi_t));

    /* signature matrix and previous degree signature matrix */
    smat_t *smat = NULL, *psmat = NULL;

    /* initialize signature related information */
    initialize_signatures_schreyer(in);
    crit_t *syz =   initialize_syzygies_schreyer(in, ht);
    crit_t *rew =   initialize_signature_criteria(st);

    /* initialize an empty basis for keeping the real basis elements */
    bs_t *bs    =   initialize_basis(st);

    /* sort initial elements, highest lead term first */
    sort_r(in->hm, (unsigned long)in->ld, sizeof(hm_t *),
            initial_input_cmp_sig, ht);

    int32_t next_degree =   in->hm[in->ld-1][DEG];
    if (st->info_level > 1) {
        printf("\ndeg     sel   pairs        mat          density \
                new data             time(rd)\n");
        printf("-------------------------------------------------\
                ----------------------------------------\n");
    }
    st->current_rd  =   0;

    while (!try_termination) {
        rrt0  = realtime();
        st->max_bht_size    = st->max_bht_size > ht->esz ?
            st->max_bht_size : ht->esz;
        st->current_rd++;

        /* prepare signature matrix for next degree */
        sba_prepare_next_degree(&psmat, &smat, st);

        /* check if we have initial generators not handled in lower degree
         * until now */
        check_initial_generators(smat, in);

        /* generate rows from previous degree matrix, start with the highest
         * signatures in order to get an efficient rewrite criterion test */
        generate_next_degree_matrix(smat, psmat, syz, rew, ht, st);

        /* sort matrix rows by increasing signature */
        sort_matrix_rows_by_increasing_signature(smat, ht);

        /* map hashes to columns */
        sba_convert_hashes_to_columns(&hcm, smat, st, ht);

        /* s-reduce matrix and add syzygies when rows s-reduce to zero */
        sba_linear_algebra(smat, syz, st, ht);

        /* maps columns to hashes */
        sba_convert_columns_to_hashes(smat, hcm);

        /* NOTE: Reset hash table indices to zero in here! */
        ht->elo = ht->eld;

        /* add new elements to basis */
        ne = sba_add_new_elements_to_basis(smat, ht, bs, st);
        if (st->info_level > 1) {
            printf("%7d new %7d zero", ne, smat->nz);
            fflush(stdout);
        }

        /* free psmat, all entries should be already freed */
        free(psmat);
        psmat = NULL;

        /* if we found a constant we are done, if we have added no new elements
         * we assume we are done*/
        if (bs->constant  == 1 || ne == 0) {
            try_termination =   1;
        }
        rrt1 = realtime();
        if (st->info_level > 1) {
            printf("%13.2f sec\n", rrt1-rrt0);
        }
    }
    if (st->info_level > 1) {
        printf("-------------------------------------------------\
                ----------------------------------------\n");
    }
    /* fully reduce elements in basis. */
    if (st->reduce_gb == 1) {
        /* prepare basis data to apply final reduction process */
        for (len_t i = 0; i < bs->ld; ++i) {
            bs->lm[i]   = ht->hd[bs->hm[i][SM_OFFSET]].sdm;
            bs->lmps[i] = i;
        }
        bs->lml = bs->ld;

        ht_t *sht = initialize_secondary_hash_table(bht, st);
        /* note: bht will become sht, and sht will become NULL,
         * thus we need pointers */
        reduce_basis(bs, mat, &hcm, &bht, &sht, st);
    }


    *bsp    = bs;
    *htp    = ht;
    *stp    = st;

    /* free and clean up */
    free(hcm);
    free_signature_criteria(&syz, st);
    free_signature_criteria(&rew, st);

    /* TODO: free signature matrices! */


    return 1;
}

