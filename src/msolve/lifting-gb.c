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


typedef struct{
  uint32_t len; /* length of the encoded polynomial */
  uint32_t **cf_32; /* array of arrays of coefficients
                       * modulo several primes
                       */
  mpz_t *cf_zz; /* array which stores CRT lifting of
                    the coefficiels */
  mpz_t *cf_qq; /* array which stores rational coefficients
                  being lifted, numerators and denominators
                  are given given as mpz_t
               */
} modpolys_struct;

typedef modpolys_struct modpolys_t[1];

typedef struct {
  uint32_t alloc; /* alloc -> max number of primes */
  uint32_t nprimes; /* number of primes */
  uint64_t *primes; /* array of prime numbers encoded with uint64_t to ensure
                       compatibility with flint */
  uint64_t *cf_64; /* array of length equal to number of primes which will be used
                    to copy coefficients (hence ensuring compatibility with
                    flint) */
  uint32_t ld; /* number of polynomials */
  modpolys_t *modpolys; /* array of polynomials modulo primes */
} gb_modpoly_array_struct;

typedef gb_modpoly_array_struct gb_modpoly_t[1];

#define NEWGBLIFT 1

#ifdef NEWGBLIFT
typedef struct{
  int32_t npol; /* number of polynomials to be lifted */
  int32_t rr; /* number of primes before activating rational reconstruction */
  int32_t nsteps; /* number of steps for lifting GB (per degree) */
  int32_t *steps; /* array of length nsteps ; the sum of the entries should
                     equal npol */
  int32_t cstep; /* current step lifting GB */
  /* liftings are performed on ranges of polynomials (depending on their degrees) */
  int32_t lstart; /* index of first polynomial to be lifted */
  int32_t lend; /* index of last polynomial to be lifted */

  int crt_mult; /* indicates if multi-mod flint structures need to be
                initialized */
  mpz_t *crt; /* stores current CRT */
  int recon; /* equals 1 when some rational number can be lifted, else 0 */
  int32_t *coef; /* array of indices to lift */
  mpz_t *num; /* lifted numerator */
  mpz_t *den; /* lifted denominator */
  mpz_t gden; /* guessed denominator */
  int32_t start; /* indicates smallest index of poly whose coef has been lifted but not checked*/
  int32_t end; /* indicates largest index of poly whose coef has been lifted but not checked*/
  int *check1; /* tells whether lifted data are ok with one more prime */
  int *check2; /* tells whether lifted data are ok with two more primes */

} data_lift_struct;

typedef data_lift_struct data_lift_t[1];
#else
typedef struct {
  int32_t lstart; /* index of polynomial being lifted */
  int32_t lend; /* not used */
  int32_t nsteps; /* number of steps for lifting GB (per degree) */
  int32_t *steps; /* array of length nsteps ; the sum of the entries should
                     equal the total number of polynomials to be lifted */
  uint32_t *coef; /*  */
  int crt_mult; /* indicates if multi-mod flint structures need to be
                initialized */
  mpz_t *crt; /* current crt */
  int recon; /* equals 1 when some rational number has been lifted, else 0 */
  mpz_t num; /* lifted numerator */
  mpz_t den; /* lifted denominator */
  int check1; /* tells whether lifted data are ok with one more prime */
  int check2; /* tells whether lifted data are ok with two more primes */
} data_lift_struct;

typedef data_lift_struct data_lift_t[1];
#endif

#ifdef NEWGBLIFT
static inline void data_lift_init(data_lift_t dlift,
                                  int32_t npol,
                                  int32_t *steps, int32_t nsteps){
  dlift->npol = npol;
  dlift->rr = 1;
  dlift->lstart = 0;
  dlift->nsteps = nsteps;

  int32_t i;

  dlift->steps = calloc(nsteps, sizeof(int32_t));
  for(i = 0; i < nsteps; i++){
    dlift->steps[i] = steps[i];
  }

  dlift->cstep = 0;
  dlift->lend = steps[0] - 1;

  dlift->crt_mult = 0;
  dlift->crt = malloc(sizeof(mpz_t) * dlift->npol);
  for(int32_t i = 0; i < dlift->npol; i++){
    mpz_init(dlift->crt[i]);
  }
  dlift->recon = 0;
  dlift->coef = calloc(npol, sizeof(mpz_t) );

  dlift->num = malloc(sizeof(mpz_t) * npol);
  for(i = 0; i < npol; i++){
    mpz_init(dlift->num[i]);
  }
  dlift->den = malloc(sizeof(mpz_t) * npol);
  for(i = 0; i < npol; i++){
    mpz_init(dlift->den[i]);
  }
  mpz_init_set_ui(dlift->gden, 1);

  dlift->start = 0;
  dlift->end = 0;
  dlift->check1 = calloc(npol, sizeof(int));
  dlift->check2 = calloc(npol, sizeof(int));

}
#else
static inline void data_lift_init(data_lift_t dlift, int npol,
                                  int32_t *steps, int32_t nsteps){
  dlift->lstart = -1;
  dlift->lend = -1;

  dlift->steps = calloc(nsteps, sizeof(int32_t));
  for(int32_t i = 0; i < nsteps; i++){
    dlift->steps[i] = steps[i];
  }

  dlift->crt_mult = 0;
  dlift->coef = calloc(npol, sizeof(mpz_t) );

  dlift->crt = malloc(sizeof(mpz_t));
  mpz_init(dlift->crt[0]);

  mpz_init(dlift->num);
  mpz_init(dlift->den);
  dlift->check1 = 0;
  dlift->check2 = 0;
  dlift->recon = 0;
}
#endif

#ifdef NEWGBLIFT
static inline void data_lift_clear(data_lift_t dlift){
  for(int32_t i = 0; i < dlift->npol; i++){
    mpz_clear(dlift->crt[i]);
  }
  free(dlift->crt);

  free(dlift->steps);
  free(dlift->coef);

  for(int32_t i = 0; i < dlift->npol; i++){
    mpz_clear(dlift->num[i]);
  }
  free(dlift->num);

  for(int32_t i = 0; i < dlift->npol; i++){
    mpz_clear(dlift->den[i]);
  }
  free(dlift->den);

  mpz_clear(dlift->gden);
  free(dlift->check1);
  free(dlift->check2);

}
#else
static inline void data_lift_clear(data_lift_t dlift){

  mpz_clear(dlift->crt[0]);
  free(dlift->crt);

  mpz_clear(dlift->num);
  mpz_clear(dlift->den);
  free(dlift->coef);

}
#endif

static inline void gb_modpoly_init(gb_modpoly_t modgbs,
                                   uint32_t alloc, int32_t *lens,
                                   uint32_t ld){
  modgbs->alloc = alloc;
  modgbs->nprimes = 0;
  modgbs->primes = calloc(sizeof(uint64_t), alloc);
  modgbs->cf_64 = calloc(sizeof(uint64_t), alloc);
  modgbs->ld = ld;
  modgbs->modpolys = malloc(sizeof(modpolys_struct) * ld);

  for(uint32_t i = 0; i < ld; i++){
    modgbs->modpolys[i]->len = lens[i];
    modgbs->modpolys[i]->cf_32 = malloc(sizeof(uint32_t **)*lens[i]);

    modgbs->modpolys[i]->cf_zz = malloc(sizeof(mpz_t)*lens[i]);
    modgbs->modpolys[i]->cf_qq = malloc(sizeof(mpz_t)*2*lens[i]);
    for(uint32_t j = 0; j < lens[i]; j++){
      modgbs->modpolys[i]->cf_32[j] = calloc(sizeof(uint32_t), alloc);
      mpz_init(modgbs->modpolys[i]->cf_zz[j]);
    }
    for(uint32_t j = 0; j < 2 * lens[i]; j++){
      mpz_init(modgbs->modpolys[i]->cf_qq[j]);
    }
  }
}

static inline void gb_modpoly_realloc(gb_modpoly_t modgbs,
                                      uint32_t newalloc){
  uint32_t oldalloc = modgbs->alloc;
  modgbs->alloc += newalloc;

  uint64_t *newprimes = (uint64_t *)realloc(modgbs->primes,
                                            modgbs->alloc * sizeof(uint64_t));
  if(newprimes == NULL){
    fprintf(stderr, "Problem when reallocating modgbs (primes)\n");
    exit(1);
  }
  modgbs->primes = newprimes;
  for(uint32_t i = oldalloc; i < modgbs->alloc; i++){
    modgbs->primes[i] = 0;
  }

  uint64_t *ncf_64 = (uint64_t *)realloc(modgbs->cf_64,
                                         modgbs->alloc * sizeof(uint64_t));
  if(ncf_64 == NULL){
    fprintf(stderr, "Problem when reallocating modgbs (cfs)\n");
    exit(1);
  }
  modgbs->cf_64 = ncf_64;
  for(uint32_t i = oldalloc; i < modgbs->alloc; i++){
    modgbs->cf_64[i] = 0;
  }

  for(uint32_t i = 0; i < modgbs->ld; i++){
    for(uint32_t j = 0; j < modgbs->modpolys[i]->len; j++){
      uint32_t *newcfs_pol = (uint32_t *)realloc(modgbs->modpolys[i]->cf_32[j],
                                                 modgbs->alloc * sizeof(uint32_t));
      if(newcfs_pol == NULL){
        fprintf(stderr, "Problem when reallocating modgbs (cfs_pol)\n");
      }
      modgbs->modpolys[i]->cf_32[j] = newcfs_pol;
      for(uint32_t k = oldalloc; k < modgbs->alloc; k++){
        modgbs->modpolys[i]->cf_32[j][k] = 0;
      }
    }
  }
}


static inline void display_gbmodpoly_cf_32(FILE *file,
                                     gb_modpoly_t modgbs){
  fprintf(file, "alloc = %d\n", modgbs->alloc);
  fprintf(file, "nprimes = %d\n", modgbs->nprimes);
  fprintf(stderr, "primes = [");
  for(uint32_t i = 0; i < modgbs->alloc-1; i++){
    fprintf(file, "%lu, ", modgbs->primes[i]);
  }
  fprintf(file, "%lu]\n", modgbs->primes[modgbs->alloc -1]);
  fprintf(file, "numpolys = %d\n", modgbs->ld);
  fprintf(file, "[\n");
  for(uint32_t i = 0; i < modgbs->ld; i++){
    uint32_t len = modgbs->modpolys[i]->len;
    fprintf(file, "[%d, ", len);
    for(uint32_t j = 0; j < len; j++){
      fprintf(stderr, "[");
      for(uint32_t k = 0; k < modgbs->alloc-1; k++){
        fprintf(file, "%d, ", modgbs->modpolys[i]->cf_32[j][k]);
      }
      if(j < len - 1){
        fprintf(file, "%d], ", modgbs->modpolys[i]->cf_32[j][modgbs->alloc-1]);
      }
      else{
        fprintf(file, "%d]\n", modgbs->modpolys[i]->cf_32[j][modgbs->alloc-1]);
      }
    }
    fprintf(file, "],\n");
  }
  fprintf(file, "]\n");
}

static inline void display_gbmodpoly_cf_qq(FILE *file,
                                           gb_modpoly_t modgbs){
  modpolys_t *pols = modgbs->modpolys;
  int32_t p = modgbs->ld ;
  fprintf(file, "[");
  for(uint32_t i = 0; i < p - 1; i++){
    fprintf(file, "[");
    for(uint32_t l = pols[i]->len - 1; l > 0; l--){
      if(mpz_cmp_ui(pols[i]->cf_qq[2*l + 1], 1)){
        mpz_out_str(file, 10, pols[i]->cf_qq[2*l]);
        fprintf(file, "/");
        mpz_out_str(file, 10, pols[i]->cf_qq[2*l + 1]);
        fprintf(file, ", ");
      }
      else{
        mpz_out_str(file, 10, pols[i]->cf_qq[2*l]);
        fprintf(file, ", ");
      }
    }
    if(mpz_cmp_ui(pols[i]->cf_qq[1], 1)){
      mpz_out_str(file, 10, pols[i]->cf_qq[0]);
      fprintf(file, "/");
      mpz_out_str(file, 10, pols[i]->cf_qq[1]);
      fprintf(file, "],\n");
    }
    else{
      mpz_out_str(file, 10, pols[i]->cf_qq[0]);
      fprintf(file, "],\n");
    }
  }
  fprintf(file, "[");
  for(uint32_t l = pols[p-1]->len - 1; l > 0; l--){
    if(mpz_cmp_ui(pols[p-1]->cf_qq[2*l + 1], 1)){
      mpz_out_str(file, 10, pols[p-1]->cf_qq[2*l]);
      fprintf(file, "/");
      mpz_out_str(file, 10, pols[p-1]->cf_qq[2*l + 1]);
      fprintf(file, ", ");
    }
    else{
      mpz_out_str(file, 10, pols[p-1]->cf_qq[2*l]);
      fprintf(file, ", ");
    }
  }
  if(mpz_cmp_ui(pols[p-1]->cf_qq[1], 1)){
    mpz_out_str(file, 10, pols[p-1]->cf_qq[0]);
    fprintf(file, "/");
    mpz_out_str(file, 10, pols[p-1]->cf_qq[1]);
    fprintf(file, "]\n");
  }
  else{
    mpz_out_str(file, 10, pols[p-1]->cf_qq[0]);
    fprintf(file, "]\n");
  }
  fprintf(file, "]:");
}

static inline void gb_modpoly_clear(gb_modpoly_t modgbs){
  free(modgbs->primes);
  for(uint32_t i = 0; i < modgbs->ld; i++){
    for(uint32_t j = 0; j < modgbs->modpolys[i]->len; j++){
      free(modgbs->modpolys[i]->cf_32[j]);
      mpz_clear(modgbs->modpolys[i]->cf_zz[j]);
    }
    for(uint32_t j = 0; j < 2 * modgbs->modpolys[i]->len; j++){
      mpz_clear(modgbs->modpolys[i]->cf_qq[j]);
    }
    free(modgbs->modpolys[i]->cf_32);
    free(modgbs->modpolys[i]->cf_zz);
    free(modgbs->modpolys[i]->cf_qq);
  }
  free(modgbs->modpolys);
}

static inline int32_t degree(int32_t *mon, int nv){
  int32_t deg = 0;
  for(int i = 0; i < nv; i++){
    deg += mon[i];
  }
  return deg;
}

/*
  bexp_lm is a list of monomials involving nv variables of increasing degrees
  return an array of length *nb containing at the i-th position the number of
  monomials of the i-th degree (hence nb is the number of different degrees).
 */
static inline int32_t *array_nbdegrees(int32_t *bexp_lm, int len,
                                     int nv, int * nb){
  *nb = 1;
  int32_t deg = degree(bexp_lm, nv);

  for(int32_t i = 1; i < len; i++){
    int32_t newdeg = degree(bexp_lm + i * nv, nv);

    if(deg != newdeg){
      (*nb)++;
      deg = newdeg;
    }
  }
  int32_t *ldeg = calloc(sizeof(int32_t), *nb);
  deg = degree(bexp_lm, nv);
  ldeg[0] = 1;
  int32_t i = 0, j = 1;
  while(j < len){
    int32_t newdeg = degree(bexp_lm + j * nv, nv);
    if(deg == newdeg){
      ldeg[i]++;
    }
    else{
      i++;
      ldeg[i] = 1;
      deg = newdeg;
    }
    j++;
  }
  return ldeg;
}

static inline int grevlex_is_less_than(int nv, int32_t* m1, int32_t *m2){
  int32_t deg1 = 0, deg2 = 0;
  for(int i = 0; i < nv; i++){
    deg1 += m1[i];
  }
  for(int i = 0; i < nv; i++){
    deg2 += m2[i];
  }
  if(deg1 < deg2){
    return 1;
  }
  if(deg1 > deg2){
    return 0;
  }
  for(int i = 0; i < nv; i++){
    if(m1[i] < m2[i]){
      return 0;
    }
  }
  return 1;
}

static inline int32_t compute_length(int32_t *mon, int nv,
                                     int32_t *basis, int dquot){

  for(int i = dquot - 1; i >= 0; i--){
    if(!grevlex_is_less_than(nv, mon, basis + i * nv)){
      return i + 1;
    }
  }
  return -1;
}

/*
 * bexp_lm is an array of len  monomials with nv variables
 * basis is an array of dquot monomials with nv variables
 * (basis of some quotient defined with bexp_lm, all with grevlex order)
 * basis is sorted increasingly, as for bexp_lm
 *
 * returns an array of length len containing the maximum possible lengths
 * of polynomials with leading terms in bexp_lm (length excluding the
 * leading term).
 */
static inline int32_t *array_of_lengths(int32_t *bexp_lm, int len,
                                        int32_t *basis, int dquot, int nv){
  int32_t *lens = calloc(sizeof(int32_t), len);
  for(int i = 0; i < len; i++){
    lens[i] = compute_length(bexp_lm + (i * nv), nv, basis, dquot);
  }
  return lens;
}

/* returns 0 in case of failure else returns 1 */
static inline int modpgbs_set(gb_modpoly_t modgbs,
                               const bs_t *bs, const ht_t * const ht,
                               const int32_t fc,
                               int32_t *basis, const int dquot,
                               int *mgb){
  if(modgbs->nprimes >= modgbs->alloc-1){
    fprintf(stderr, "Not enough space in modgbs\n");
    return 0;
  }
  modgbs->primes[modgbs->nprimes] = fc;

  len_t i, j, k, idx;

  len_t len   = 0;
  hm_t *hm    = NULL;

  const len_t nv  = ht->nv;
  const len_t ebl = ht->ebl;
  const len_t evl = ht->evl;

  int *evi    =   (int *)malloc((unsigned long)ht->nv * sizeof(int));
  if (ebl == 0) {
    for (i = 1; i < evl; ++i) {
      evi[i-1]    =   i;
    }
  } else {
    for (i = 1; i < ebl; ++i) {
      evi[i-1]    =   i;
    }
    for (i = ebl+1; i < evl; ++i) {
      evi[i-2]    =   i;
    }
  }

  for(i = 0; i < modgbs->ld; i++){
    idx = bs->lmps[i];
    if (bs->hm[idx] == NULL) {
      fprintf(stderr, " poly is 0\n");
      exit(1);
    } else {
      hm  = bs->hm[idx]+OFFSET;
      len = bs->hm[idx][LENGTH];
    }
    int bc = modgbs->modpolys[i]->len - 1;
    for (j = 1; j < len; ++j) {
      uint32_t c = bs->cf_32[bs->hm[idx][COEFFS]][j];
      for (k = 0; k < nv; ++k) {
          mgb[k] = ht->ev[hm[j]][evi[k]];
      }
      while(!is_equal_exponent(mgb, basis + (bc * nv), nv)){
        bc--;
      }
      modgbs->modpolys[i]->cf_32[bc][modgbs->nprimes] = c;
      bc--;
    }
  }

  modgbs->nprimes++;

  free(evi);
  return 1;
}

static inline int32_t maxbitsize_gens(data_gens_ff_t *gens, len_t ngens){
  if(gens->field_char != 0){
    return -1;
  }
  const int32_t *lens = gens->lens;
  mpz_t **cfs = gens->mpz_cfs;
  int32_t off = 0;
  int32_t mbs = 0;
  for(int32_t i = 0; i < ngens; i++){
    for(int32_t j = off; j < off + lens[i]; ++j){
      mbs = MAX(mbs,
                mpz_sizeinbase(*(cfs[2*j]), 2) + mpz_sizeinbase(*(cfs[2*j+1]), 2));
    }
    off += lens[i];
  }
  return mbs;
}

static int32_t * gb_modular_trace_learning(gb_modpoly_t modgbs,
                                           int32_t *mgb,
                                           int32_t *num_gb,
                                           int32_t **leadmons,
                                           trace_t *trace,
                                           ht_t *tht,
                                           bs_t *bs_qq,
                                           ht_t *bht,
                                           stat_t *st,
                                           const int32_t fc,
                                           int info_level,
                                           int print_gb,
                                           int *dim,
                                           long *dquot_ori,
                                           data_gens_ff_t *gens,
                                           int32_t maxbitsize,
                                           files_gb *files,
                                           int *success)
{
    double ca0, rt;
    ca0 = realtime();

    bs_t *bs = NULL;
    if(gens->field_char){
      bs = bs_qq;
      int boo = core_gba(&bs, &bht, &st);
      if (!boo) {
        printf("Problem with F4, stopped computation.\n");
        exit(1);
      }
      free_shared_hash_data(bht);
    }
    else{
      if(st->laopt > 40){
        bs = modular_f4(bs_qq, bht, st, fc);
      }
      else{
        bs = gba_trace_learning_phase(trace, tht, bs_qq, bht, st, fc);
      }
    }
    rt = realtime()-ca0;

    if(info_level > 1){
        fprintf(stderr, "Learning phase %.2f Gops/sec\n",
                (st->trace_nr_add+st->trace_nr_mult)/1000.0/1000.0/rt);
    }
    if(info_level > 2){
        fprintf(stderr, "------------------------------------------\n");
        fprintf(stderr, "#ADDITIONS       %13lu\n", (unsigned long)st->trace_nr_add * 1000);
        fprintf(stderr, "#MULTIPLICATIONS %13lu\n", (unsigned long)st->trace_nr_mult * 1000);
        fprintf(stderr, "#REDUCTIONS      %13lu\n", (unsigned long)st->trace_nr_red);
        fprintf(stderr, "------------------------------------------\n");
    }

    /* Leading monomials from Grobner basis */
    int32_t *bexp_lm = get_lm_from_bs(bs, bht);
    leadmons[0] = bexp_lm;
    num_gb[0] = bs->lml;

    if(bs->lml == 1){
        if(info_level){
            fprintf(stderr, "Grobner basis has a single element\n");
        }
        int is_empty = 1;
        for(int i = 0; i < bht->nv; i++){
            if(bexp_lm[i]!=0){
                is_empty = 0;
            }
        }
        if(is_empty){
            *dquot_ori = 0;
            *dim = 0;
            if(info_level){
              fprintf(stderr, "No solution\n");
            }
            print_ff_basis_data(
                                files->out_file, "a", bs, bht, st, gens, print_gb);
            return NULL;
        }
    }

    /**************************************************/
    long dquot = 0;
    int32_t *lmb = monomial_basis_enlarged(bs->lml, bht->nv,
                                           bexp_lm, &dquot);

    /************************************************/
    /************************************************/

    int32_t *lens = array_of_lengths(bexp_lm, bs->lml, lmb, dquot, bht->nv);

    gb_modpoly_init(modgbs, maxbitsize, lens, bs->lml);

    modpgbs_set(modgbs, bs, bht, fc, lmb, dquot, mgb);

    free_basis(&(bs));
    return lmb;
}


static void gb_modular_trace_application(gb_modpoly_t modgbs,
                                         int32_t *mgb,
                                         int32_t *num_gb,
                                         int32_t **leadmons_ori,
                                         int32_t **leadmons_current,

                                         trace_t **btrace,
                                         ht_t **btht,
                                         const bs_t *bs_qq,
                                         ht_t **bht,
                                         stat_t *st,
                                         const int32_t fc,
                                         int info_level,
                                         bs_t **bs,
                                         int32_t *lmb_ori,
                                         int32_t dquot_ori,
                                         primes_t *lp,
                                         data_gens_ff_t *gens,
                                         double *stf4,
                                         int *bad_primes){

  st->info_level = 0;

  /* tracing phase */
  len_t i;
  double ca0;

  /* F4 and FGLM are run using a single thread */
  /* st->nthrds is reset to its original value afterwards */
  const int nthrds = st->nthrds;
  st->nthrds = 1 ;
  /*at the moment multi-threading is not supprted here*/
  memset(bad_primes, 0, (unsigned long)st->nprimes * sizeof(int));
  #pragma omp parallel for num_threads(nthrds)  \
    private(i) schedule(static)
  for (i = 0; i < st->nprimes; ++i){
    ca0 = realtime();
    if(st->laopt > 40){
      bs[i] = modular_f4(bs_qq, bht[i], st, lp->p[i]);
    }
    else{
      bs[i] = gba_trace_application_phase(btrace[i], btht[i], bs_qq, bht[i], st, lp->p[i]);
    }
    *stf4 = realtime()-ca0;

    if(bs[i]->lml != num_gb[i]){
      if (bs[i] != NULL) {
        free_basis(&(bs[i]));
      }
      bad_primes[i] = 1;
      /* return; */
    }
    get_lm_from_bs_trace(bs[i], bht[i], leadmons_current[i]);

    if(!equal_staircase(leadmons_current[i], leadmons_ori[i],
                       num_gb[i], num_gb[i], bht[i]->nv)){
      bad_primes[i] = 1;
    }

  }
  for(i = 0; i < st->nprimes; i++){
    if(!bad_primes[i] && bs[i] != NULL){
      /* copy of data for multi-mod computation */
      modpgbs_set(modgbs, bs[i], bht[i], lp->p[i], lmb_ori, dquot_ori, mgb);
    }
    if (bs[i] != NULL) {
      free_basis(&(bs[i]));
    }
  }

  st->nthrds = nthrds;
}

static inline void choose_coef_to_lift(gb_modpoly_t modgbs, data_lift_t dlift){
  uint32_t ld = modgbs->ld;
  for(int32_t i = 0; i < ld; i++){
    uint32_t d = 0;
    while(d < modgbs->modpolys[i]->len){
      if(modgbs->modpolys[i]->cf_32[d][0]){
        dlift->coef[i] = d;
        break;
      }
      else{
        d++;
      }
    }
  }
}

#ifdef NEWGBLIFT
/* uses FLINT's multi CRT when starting to lift one witness coef */
/* TODO: avoid to initialize/clear comb and comb_temp at each call */
/* coef is the array of indices of the coefficients to be lifted */
static inline void start_dlift(gb_modpoly_t modgbs, data_lift_t dlift, uint32_t *coef){
  /* Data needed by multi CRT functions */
  fmpz_comb_t comb;
  fmpz_comb_temp_t comb_temp;

  fmpz_comb_init(comb, modgbs->primes, modgbs->nprimes);
  fmpz_comb_temp_init(comb_temp, comb);
  fmpz_t y;
  fmpz_init(y);

  modpolys_t *polys = modgbs->modpolys;

  for(int32_t k = dlift->lstart; k <= dlift->lend; k++){
    int32_t cl = dlift->coef[k];
    for(uint32_t i = 0; i < modgbs->nprimes; i++){
      /* modgbs->cf_64[i] = polys[k]->cf_32[coef[cl]][i]; */
      modgbs->cf_64[i] = polys[k]->cf_32[cl][i];
    }
    fmpz_multi_CRT_ui(y, modgbs->cf_64,
                      comb, comb_temp, 1);
    fmpz_get_mpz(dlift->crt[k], y);
  }

  /* indicates that CRT started */
  dlift->crt_mult = 1;

  fmpz_clear(y);
  fmpz_comb_temp_clear(comb_temp);
  fmpz_comb_clear(comb);

}
#else
/* uses FLINT's multi CRT when starting to lift one witness coef */
static inline void start_dlift(gb_modpoly_t modgbs, data_lift_t dlift, uint32_t *coef){
  /* Data needed by multi CRT functions */
  fmpz_comb_t comb;
  fmpz_comb_temp_t comb_temp;

  fmpz_comb_init(comb, modgbs->primes, modgbs->nprimes);
  fmpz_comb_temp_init(comb_temp, comb);
  fmpz_t y;
  fmpz_init(y);

  modpolys_t *polys = modgbs->modpolys;

  for(uint32_t i = 0; i < modgbs->nprimes; i++){
    modgbs->cf_64[i] = polys[dlift->lstart]->cf_32[coef[0]][i];
  }
  fmpz_multi_CRT_ui(y, modgbs->cf_64,
                    comb, comb_temp, 1);
  fmpz_get_mpz(dlift->crt[0], y);

  /* indicates that CRT started */
  dlift->crt_mult = 1;

  fmpz_clear(y);
  fmpz_comb_temp_clear(comb_temp);
  fmpz_comb_clear(comb);
}
#endif

#ifdef NEWGBLIFT
/* Incremental CRT (called once FLINT multi_CRT has been called) */
/* mod is the current modulus */
static inline void incremental_dlift_crt(gb_modpoly_t modgbs, data_lift_t dlift,
                                         int32_t *coef, mpz_t *mod_p, mpz_t *prod_p,
                                         int thrds){


  /* all primes are assumed to be good primes */
  mpz_mul_ui(prod_p[0], mod_p[0], modgbs->primes[modgbs->nprimes - 1 ]);
  for(int32_t k = dlift->lstart; k <= dlift->lend; k++){
    uint32_t c = modgbs->modpolys[k]->cf_32[coef[k]][modgbs->nprimes  - 1 ];
    mpz_CRT_ui(dlift->crt[k], dlift->crt[k], mod_p[0],
               c, modgbs->primes[modgbs->nprimes - 1 ],
               prod_p[0], 1);

  }
  mpz_set(mod_p[0], prod_p[0]);


}
#else
/* Incremental CRT (called once FLINT multi_CRT has been called) */
/* mod is the current modulus */
static inline void incremental_dlift_crt(gb_modpoly_t modgbs, data_lift_t dlift,
                                         int32_t *coef, mpz_t *mod_p, mpz_t *prod_p,
                                         int thrds){
  /* all primes are assumed to be good primes */
  for(int i = 0; i < thrds; i++){
    uint32_t c = modgbs->modpolys[dlift->lstart]->cf_32[coef[0]][modgbs->nprimes  - (thrds - i) ];

    mpz_mul_ui(prod_p[0], mod_p[0], modgbs->primes[modgbs->nprimes - (thrds - i) ]);

    mpz_CRT_ui(dlift->crt[0], dlift->crt[0], mod_p[0],
               c, modgbs->primes[modgbs->nprimes - (thrds - i) ],
               prod_p[0], 1);
    mpz_set(mod_p[0], prod_p[0]);
  }

}
#endif


static inline void crt_lift_modgbs(gb_modpoly_t modgbs, data_lift_t dlift,
                                   int32_t start, int32_t end){
  /* Data needed by multi CRT functions */
  fmpz_comb_t comb;
  fmpz_comb_temp_t comb_temp;

  fmpz_comb_init(comb, modgbs->primes, modgbs->nprimes);
  fmpz_comb_temp_init(comb_temp, comb);
  fmpz_t y;
  fmpz_init(y);

  modpolys_t *polys = modgbs->modpolys;

  for(int32_t k = start; k <= end; k++){
    if(dlift->check2[k]){
      for(int32_t l = 0; l < polys[k]->len; l++){
        for(uint32_t i = 0; i < modgbs->nprimes; i++){
          modgbs->cf_64[i] = polys[k]->cf_32[l][i];
        }
        fmpz_multi_CRT_ui(y, modgbs->cf_64,
                          comb, comb_temp, 1);
        fmpz_get_mpz(polys[k]->cf_zz[l], y);
      }
    }
  }

  fmpz_clear(y);
  fmpz_comb_temp_clear(comb_temp);
  fmpz_comb_clear(comb);

}

/*
  returns the index of the poly if some of its coeff could not be lifted
  else returns -1
 */
static inline int ratrecon_lift_modgbs(gb_modpoly_t modgbs, data_lift_t dlift,
                                       int32_t start, int32_t end,
                                       mpz_t *mod_p, rrec_data_t recdata){
 
  mpz_t rnum, rden, lcm;
  mpz_init(rnum);
  mpz_init(rden);
  mpz_init(lcm);


  mpz_t rnum, rden;

  mpz_init(rnum);
  mpz_init(rden);
  mpz_init(lcm);

  modpolys_t *polys = modgbs->modpolys;
  for(int32_t k = start; k <= end; k++){

    if(dlift->check2[k]){
      mpz_fdiv_q_2exp(recdata->N, mod_p[0], 1);

      if(dlift->cstep){
        mpz_root(recdata->D, recdata->N, 3);
        mpz_fdiv_q(recdata->N, recdata->N, recdata->D);
      }
      else{
        mpz_sqrt(recdata->N, recdata->N);
        mpz_set(recdata->D, recdata->N);
        mpz_fdiv_q(recdata->D, recdata->D, dlift->den[k]);
        mpz_mul(recdata->N, recdata->N, dlift->den[k]);

      }

      mpz_set(lcm, dlift->den[k]);

      for(int32_t l = 0; l < polys[k]->len; l++){
        if(ratreconwden(rnum, rden, polys[k]->cf_zz[l], mod_p[0], dlift->den[k], recdata)){
          mpz_set(polys[k]->cf_qq[2*l], rnum);
          mpz_mul(rden, rden, dlift->den[k]);
          mpz_set(polys[k]->cf_qq[2*l + 1], rden);
          mpz_lcm(lcm, lcm, rden);
        }
        else{
          fprintf(stderr, "[%d/%d]", k, modgbs->ld - 1);
          mpz_clear(rnum);
          mpz_clear(rden);
          mpz_clear(lcm);
          return k;
        }
      }
      mpz_lcm(dlift->gden, dlift->gden, lcm);
    }
    else{
      mpz_clear(rnum);
      mpz_clear(rden);
      mpz_clear(lcm);
      return k;

    }
    else{
      return k;
    }
  }
  mpz_clear(rnum);
  mpz_clear(rden);
  mpz_clear(lcm);
  return -1;
}

#ifdef NEWGBLIFT
/* returns the first index between start and end for which the lifted rationals stored
   in dlift are not ok
   else it returns -1
*/
static inline int verif_lifted_rational(gb_modpoly_t modgbs, data_lift_t dlift,
                                        int thrds){
  if(dlift->recon){

    for(int32_t k = dlift->start; k <= dlift->end; k++){

      for(int i = 0; i < thrds; i++){

        uint32_t prime = modgbs->primes[modgbs->nprimes - (thrds - i) ];
        uint32_t lc = mpz_fdiv_ui(dlift->den[k], prime);
        lc = mod_p_inverse_32(lc, prime);

        uint64_t c = mpz_fdiv_ui(dlift->num[k], prime);
        c *= lc;
        c = c % prime;

        uint32_t coef = modgbs->modpolys[k]->cf_32[dlift->coef[k]][modgbs->nprimes  - (thrds - i) ];

        if(c!=coef){
          dlift->check1[k] = 0;
          dlift->check2[k] = 0;
          return k;
        }
      }
      if(!dlift->check1[k]){
        dlift->check1[k] = 1;
      }
      else{
        dlift->check2[k] = 1;
      }
    }

  }
  return -1;
}
#else
/* return 1 if the lifted rational hidden in dlift is ok else return 0 */
static inline int verif_lifted_rational(gb_modpoly_t modgbs, data_lift_t dlift,
                                        int thrds){

  for(int i = 0; i < thrds; i++){

    uint32_t prime = modgbs->primes[modgbs->nprimes - (thrds - i) ];
    uint32_t lc = mpz_fdiv_ui(dlift->den, prime);
    lc = mod_p_inverse_32(lc, prime);

    uint64_t c = mpz_fdiv_ui(dlift->num, prime);
    c *= lc;
    c = c % prime;

    uint32_t coef = modgbs->modpolys[dlift->lstart]->cf_32[dlift->coef[0]][modgbs->nprimes  - (thrds - i) ];

    if(c!=coef){
      return 0;
    }

  }
  return 1;
}
#endif

#ifdef NEWGBLIFT
static inline void update_dlift(gb_modpoly_t modgbs, data_lift_t dlift,
                                mpz_t *mod_p, mpz_t *prod_p, int thrds){

  if(verif_lifted_rational(modgbs, dlift, thrds)){
  }

  return;
}
#else
static inline void update_dlift(gb_modpoly_t modgbs, data_lift_t dlift,
                                mpz_t *mod_p, mpz_t *prod_p, int thrds){
  if(dlift->recon == 1){
    /* at a previous call a rational number could lifted */
    if(verif_lifted_rational(modgbs, dlift, thrds)){
      if(!dlift->check1){
        dlift->check1 = 1;
      }
      if(!dlift->check2){
        dlift->check2 = 1;
      }
    }
    else{
      dlift->check1 = 0;
      dlift->check2 = 0;
    }
  }
}
#endif

static void update_prodprimes(gb_modpoly_t modgbs, data_lift_t dlift,
                              mpz_t *mod_p, mpz_t *prod_p, int thrds){
  if(dlift->crt_mult == 0){
    /* starts lifting witness coefficient */
    start_dlift(modgbs, dlift, dlift->coef);
    /* updates mod_p and ptr_p */
    if(dlift->lstart == 0){

      for(int i = 0; i < modgbs->nprimes; i++){
        uint32_t prime = modgbs->primes[i];
        mpz_mul_ui(mod_p[0], mod_p[0], prime);
      }
      mpz_set(prod_p[0], mod_p[0]);

    }
    else{
      for(int i = 0; i < thrds; i++){
        uint32_t prime = modgbs->primes[modgbs->nprimes - (thrds - i)];
        mpz_mul_ui(mod_p[0], mod_p[i], prime);
      }
      mpz_set(prod_p[0], mod_p[0]);
    }
  }
  else{
    incremental_dlift_crt(modgbs, dlift, dlift->coef,
                          mod_p, prod_p, thrds);
    /* mod_p and prod_p are updated inside incremental_dlift_crt */
  }
}

#ifdef NEWGBLIFT
static void ratrecon_gb(gb_modpoly_t modgbs, data_lift_t dlift,
                        mpz_t *mod_p, mpz_t *prod_p,
                        rrec_data_t recdata,
                        int thrds, double *st_crt, double *st_rrec){
#ifdef DEBUGGBLIFT
  for(int i = 0; i < dlift->nsteps; i++){
    fprintf(stderr, "[%d]", dlift->steps[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "nprimes  = %d [cstep = %d]\n", modgbs->nprimes, dlift->cstep);
#endif

  verif_lifted_rational(modgbs, dlift, thrds);

  /********************************************************/
  /*                     CRT                              */
  /********************************************************/
  /* starts CRT */
  double st = realtime();
  if(dlift->crt_mult == 0){
    /* if(modgbs->nprimes >=  (dlift->lend)/2 + 1){ */
    if(modgbs->nprimes >=  (dlift->steps[dlift->cstep]) / 16){

      start_dlift(modgbs, dlift, dlift->coef);

      if(dlift->lstart == 0){
        mpz_set_ui(mod_p[0], 1);
        for(int i = 0; i < modgbs->nprimes; i++){
          uint32_t prime = modgbs->primes[i];
          mpz_mul_ui(mod_p[0], mod_p[0], prime);
        }
        mpz_set(prod_p[0], mod_p[0]);

      }
      else{
        for(int i = 0; i < thrds; i++){
          uint32_t prime = modgbs->primes[modgbs->nprimes - (thrds - i)];
          mpz_mul_ui(mod_p[0], mod_p[i], prime);
        }
        mpz_set(prod_p[0], mod_p[0]);
      }
      return;
    }
    else{
      /* We do not have enough primes */
      if(dlift->lstart == 0){
        mpz_set_ui(mod_p[0], 1);
        for(int i = 0; i < modgbs->nprimes; i++){
          uint32_t prime = modgbs->primes[i];
          mpz_mul_ui(mod_p[0], mod_p[0], prime);
        }
        mpz_set(prod_p[0], mod_p[0]);

      }
      else{
        for(int i = 0; i < thrds; i++){
          uint32_t prime = modgbs->primes[modgbs->nprimes - (thrds - i)];
          mpz_mul_ui(mod_p[0], mod_p[i], prime);
        }
        mpz_set(prod_p[0], mod_p[0]);
      }
      return;
    }
  }
  else{
    incremental_dlift_crt(modgbs, dlift,
                          dlift->coef, mod_p, prod_p,
                          thrds);
  }

  *st_crt += realtime() - st;

  /********************************************************/
  /********************************************************/

  /********************************************************/
  /*                       RATRECON                       */
  /********************************************************/

  if(dlift->lstart == 0){
    mpz_set_ui(dlift->gden, 1);
  }
  else{
    if(dlift->check2[dlift->lstart-1]){
      mpz_set(dlift->gden, dlift->den[dlift->lstart-1]);
    }
  }

  int32_t start = dlift->lstart;
  dlift->start = start;
  dlift->end = start-1;

  st = realtime();

  mpz_fdiv_q_2exp(recdata->N, mod_p[0], 1);

  if(dlift->cstep){

    long bg = mpz_sizeinbase(dlift->gden, 2);
    long bt = mpz_sizeinbase(recdata->N, 2);

    mpz_root(recdata->D, recdata->N, 3);
    mpz_fdiv_q(recdata->N, recdata->N, recdata->D);

  }
  else{
    mpz_sqrt(recdata->N, recdata->N);
    mpz_set(recdata->D, recdata->N);

  }

  if(modgbs->nprimes % dlift->rr == 0){
    for(int32_t i = dlift->lstart; i <= dlift->lend; i++){

      dlift->recon = ratreconwden(dlift->num[i], dlift->den[i],
                                  dlift->crt[i], mod_p[0], dlift->gden, recdata);

      if(dlift->recon){
        mpz_mul(dlift->den[i], dlift->den[i], dlift->gden);

        dlift->lstart++;
        dlift->end++;

      }
      else{

        dlift->recon = 0;

        break;
      }
    }
  }
  *st_rrec += realtime()-st;

  /********************************************************/
  /********************************************************/

  int b = -1;
  if(dlift->lstart != start){

    /* lifting over all the polynomials in the range */
    st = realtime();
    crt_lift_modgbs(modgbs, dlift, start, dlift->lend);
    *st_crt += realtime() - st;

    st = realtime();
    b = ratrecon_lift_modgbs(modgbs, dlift, start, dlift->lend,
                             mod_p, recdata);
    *st_rrec += realtime() - st;



  /********************************************************/
  /*                       RATRECON                       */
  /********************************************************/
  mpz_fdiv_q_2exp(recdata->N, mod_p[0], 1);
  mpz_sqrt(recdata->N, recdata->N);
  mpz_set(recdata->D, recdata->N);
  int32_t start = dlift->lstart;
  dlift->start = start;
  dlift->end = start-1;

  for(int32_t i = dlift->lstart; i <= dlift->lend; i++){
    fprintf(stderr, "[%d]", i);
    st = realtime();
    dlift->recon = ratrecon(dlift->num[i], dlift->den[i],
                            dlift->crt[i], mod_p[0], recdata);
    *st_rrec += realtime()-st;

    if(dlift->recon){
      dlift->lstart++;
      dlift->end++;
    }
    else{
      break;
    }
  }


  /********************************************************/
  /********************************************************/

  int b = -1;
  if(dlift->lstart != start){
    /* lifting over all the polynomials in the range */
    st = realtime();
    crt_lift_modgbs(modgbs, dlift, start, dlift->lend);
    *st_crt += realtime() - st;

    st = realtime();
    b = ratrecon_lift_modgbs(modgbs, dlift, start, dlift->lend,
                             mod_p, recdata);
    *st_rrec += realtime() - st;

    if(b >= 0){
      dlift->lstart = b;
    }
    else{
      dlift->lstart = dlift->lend + 1;
      dlift->lend += dlift->steps[dlift->cstep + 1] ;
      dlift->cstep++;
      dlift->crt_mult = 0;
    }
  }
  else{
    dlift->recon = 0;
  }

    if(b >= 0){
      dlift->lstart = b;
    }
    else{
      dlift->lstart = dlift->lend + 1;
      dlift->lend += dlift->steps[dlift->cstep + 1] ;
      dlift->cstep++;
      dlift->crt_mult = 0;
    }
  }
  else{
    dlift->recon = 0;
  }

  start_dlift(modgbs, dlift, dlift->coef);

  /* all polynomials have been lifted */
  if(dlift->lstart >= modgbs->ld){
    dlift->lend = dlift->lstart;
    dlift->end = dlift->lstart;
    return;
  }
  return;
}
#else
/* returns 0 when gb is lifted over the rationals */
static void ratrecon_gb(gb_modpoly_t modgbs, data_lift_t dlift,
                        mpz_t *mod_p, mpz_t *prod_p,
                        rrec_data_t recdata, int thrds){

  /* updates dlift->check1 and dlift->check2 */
  update_dlift(modgbs, dlift, mod_p, prod_p, thrds);
  /* the last coef to lift was ok ; we are done */
  if((dlift->check1 && dlift->check2 && dlift->lstart >= modgbs->ld - 1)){
    return;
  }

  /* previous witness coef has been lifted and checked twice */
  /* we then switch to the next polynomial */
  if(dlift->lstart == -1 || (dlift->check1 && dlift->check2 && dlift->lstart < modgbs->ld - 1)){
    /* next pol to lift */
    dlift->lstart++;

    /* updates check flags */
    dlift->check1 = 0;
    dlift->check2 = 0;
    dlift->crt_mult = 0;
  }
  if(dlift->crt_mult == 0){
    /* starts lifting witness coefficient */
    start_dlift(modgbs, dlift, dlift->coef);
    /* updates mod_p and ptr_p */
    if(dlift->lstart == 0){

      for(int i = 0; i < modgbs->nprimes; i++){
        uint32_t prime = modgbs->primes[i];
        mpz_mul_ui(mod_p[0], mod_p[0], prime);
      }
      mpz_set(prod_p[0], mod_p[0]);

    }
    else{
      for(int i = 0; i < thrds; i++){
        uint32_t prime = modgbs->primes[modgbs->nprimes - (thrds - i)];
        mpz_mul_ui(mod_p[0], mod_p[i], prime);
      }
      mpz_set(prod_p[0], mod_p[0]);
    }


  }
  else{
    incremental_dlift_crt(modgbs, dlift, dlift->coef,
                          mod_p, prod_p, thrds);
    /* mod_p and prod_p are updated inside incremental_dlift_crt */
  }

  mpz_fdiv_q_2exp(recdata->N, mod_p[0], 1);
  mpz_sqrt(recdata->N, recdata->N);
  mpz_set(recdata->D, recdata->N);

  dlift->recon = ratrecon(dlift->num, dlift->den, dlift->crt[0], mod_p[0], recdata);

  if(dlift->recon){
    fprintf(stderr, "dlift->lstart = %d\n", dlift->lstart);
    mpz_out_str(stderr, 10, dlift->num);
    fprintf(stderr, " / ");
    mpz_out_str(stderr, 10, dlift->den);
    fprintf(stderr, "\n");
  }

#ifdef DEBUGGBLIFT
  fprintf(stderr, "dlift->lstart = %d\n", dlift->lstart);
  fprintf(stderr, "dlift->coef = %d\n", dlift->coef);
  fprintf(stderr, "dlift->recon = %d\n", dlift->recon);
  fprintf(stderr, "LIFTED = ");
  mpz_out_str(stderr, 10, dlift->num);
  fprintf(stderr, " / ");
  mpz_out_str(stderr, 10, dlift->den);
  fprintf(stderr, "\n");
#endif

}
#endif

/*

  - renvoie 0 si le calcul est ok.
  => GB = [1] dim =0 dquot = 0
  => Positive dimension dim > 0
  => Dimension zero + calcul qui a pu etre fait. dim=0 dquot > 0

  - renvoie 1 si le calcul a echoue
  => Dimension 0 => pas en position generique

  - renvoie 2 si besoin de plus de genericite.
  => (tous les carres ne sont pas sous l'escalier)

  - renvoie -2 si la carac est > 0

  - renvoie -3 si meta data pas bonnes

  - renvoie -4 si bad prime
*/

int msolve_gbtrace_qq(
                      int *dim_ptr,
                      long *dquot_ptr,
                      data_gens_ff_t *gens,
                      int32_t ht_size, //initial_hts,
                      int32_t nr_threads,
                      int32_t max_nr_pairs,
                      int32_t elim_block_len,
                      int32_t reset_ht,
                      int32_t la_option,
                      int32_t use_signatures,
                      int32_t info_level,
                      int32_t print_gb,
                      int32_t pbm_file,
                      files_gb *files){

  uint32_t field_char = gens->field_char;
  const void *cfs = gens->mpz_cfs;
  if(gens->field_char){
    cfs = gens->cfs;
  }
  else{
    cfs = gens->mpz_cfs;
  }
  int mon_order = 0;
  int32_t nr_vars = gens->nvars;
  int32_t nr_gens = gens->ngens;
  int reduce_gb = 1;
  int32_t nr_nf = 0;
  const uint32_t prime_start = pow(2, 30);

  len_t i;

  /* initialize stuff */
  stat_t *st  = initialize_statistics();

  int *invalid_gens   =   NULL;
  int res = validate_input_data(&invalid_gens, cfs, gens->lens, &field_char, &mon_order,
                                &elim_block_len, &nr_vars, &nr_gens, &nr_nf, &ht_size, &nr_threads,
                                &max_nr_pairs, &reset_ht, &la_option, &use_signatures, &reduce_gb,
                                &info_level);

  /* all data is corrupt */
  if (res == -1) {
    fprintf(stderr, "Invalid input generators, msolve now terminates.\n");
    free(invalid_gens);
    return -3;
  }
  /* checks and set all meta data. if a nonzero value is returned then
   * some of the input data is corrupted. */

  if (check_and_set_meta_data_trace(st, gens->lens, gens->exps, cfs, invalid_gens,
                                    field_char, mon_order, elim_block_len, nr_vars, nr_gens,
                                    nr_nf, ht_size, nr_threads, max_nr_pairs, reset_ht, la_option,
                                    use_signatures, reduce_gb, prime_start,
                                    nr_threads /* nr_primes */,
                                    pbm_file, info_level)) {
    free(st);
    return -3;
  }

  mstrace_t msd;
  initialize_mstrace(msd, st);

  /* read in ideal, move coefficients to integers */
  import_input_data(msd->bs_qq, msd->bht, st, gens->lens, gens->exps, cfs, invalid_gens);
  free(invalid_gens);
  invalid_gens  =   NULL;

  if (st->info_level > 0) {
    print_initial_statistics(stderr, st);
  }

  /* for faster divisibility checks, needs to be done after we have
    * read some input data for applying heuristics */
  calculate_divmask(msd->bht);

  /* sort initial elements, smallest lead term first */
  sort_r(msd->bs_qq->hm, (unsigned long)msd->bs_qq->ld, sizeof(hm_t *),
          initial_input_cmp, msd->bht);
  if(gens->field_char == 0){
    remove_content_of_initial_basis(msd->bs_qq);
    /* generate lucky prime numbers */
    generate_lucky_primes(msd->lp, msd->bs_qq, st->prime_start, st->nthrds);
  }
  else{
    msd->lp->old = 0;
    msd->lp->ld = 1;
    msd->lp->p = calloc(1, sizeof(uint32_t));
    normalize_initial_basis(msd->bs_qq, st->fc);
  }

  uint32_t prime = next_prime(1<<30);
  uint32_t primeinit;
  srand(time(0));

  prime = next_prime(rand() % (1303905301 - (1<<30) + 1) + (1<<30));
  while(gens->field_char==0 && is_lucky_prime_ui(prime, msd->bs_qq)){
    prime = next_prime(rand() % (1303905301 - (1<<30) + 1) + (1<<30));
  }

  primeinit = prime;
  msd->lp->p[0] = primeinit;
  if(gens->field_char){
    msd->lp->p[0] = gens->field_char;
    primeinit = gens->field_char;
  }
  prime = next_prime(1<<30);

  int success = 1;
  gb_modpoly_t modgbs;

  int32_t maxbitsize = maxbitsize_gens(gens, st->ngens);
  fprintf(stderr, "MAX BIT SIZE COEFFS = %d\n", maxbitsize);

  int learn = 1, apply = 1, nprimes = 0;
  double stf4 = 0;

  ht_t **btht;



  rrec_data_t recdata;
  initialize_rrec_data(recdata);

  data_lift_t dlift;
  /* indicates that dlift has been already initialized */
  int dlinit = 0;
  double st_crt = 0;
  double st_rrec = 0;

  while(learn){
    int32_t *lmb_ori = gb_modular_trace_learning(modgbs,
                                                 msd->mgb,
                                                 msd->num_gb, msd->leadmons_ori,
                                                 msd->btrace[0],
                                                 msd->tht, msd->bs_qq, msd->bht, st,
                                                 msd->lp->p[0],
                                                 info_level,
                                                 print_gb,
                                                 dim_ptr, dquot_ptr,
                                                 gens, maxbitsize,
                                                 files,
                                                 &success);

    apply = 1;

    gb_modpoly_realloc(modgbs, 2);

#ifdef DEBUGGBLIFT
    display_gbmodpoly(stderr, modgbs);
#endif

    if(!dlinit){
      int nb = 0;
      int32_t *ldeg = array_nbdegrees((*msd->leadmons_ori), msd->num_gb[0],
                                      msd->bht->nv, &nb);
      data_lift_init(dlift, modgbs->ld, ldeg, nb);

      choose_coef_to_lift(modgbs, dlift);
      free(ldeg);
      dlinit = 1;
    }

    if(info_level){
      int s= 0;
      for(int i = 0; i < dlift->nsteps; i++){
        fprintf(stderr, "[%d]", dlift->steps[i]);
        s+=dlift->steps[i];
      }
      fprintf(stderr, "\n");
      if(s > 1){
        fprintf(stderr, "%d polynomials to lift\n", s);
      }
    }

    if(lmb_ori == NULL || success == 0 || gens->field_char) {

      apply = 0;
      if(dlinit){
        data_lift_clear(dlift);
      }

      gb_modpoly_clear(modgbs);

      free_mstrace(msd, st);
      free(st);

    }
    /* duplicate data for multi-threaded multi-mod computation */
    duplicate_data_mthread_gbtrace(st->nthrds, st, msd->num_gb,
                                   msd->leadmons_ori, msd->leadmons_current,
                                   msd->btrace);

    /* copy of hash tables for tracer application */
    msd->blht[0] = msd->bht;
    for(int i = 1; i < st->nthrds; i++){
      ht_t *lht = copy_hash_table(msd->bht, st);
      msd->blht[i] = lht;
    }
    msd->btht[0] = msd->tht;
    for(int i = 1; i < st->nthrds; i++){
      msd->btht[i] = copy_hash_table(msd->tht, st);
    }

    if(info_level){
      fprintf(stderr, "\nStarts trace based multi-modular computations\n");
    }

    learn = 0;
    while(apply){

      /* generate lucky prime numbers */
      msd->lp->p[0] = next_prime(prime);
      while(is_lucky_prime_ui(prime, msd->bs_qq) || prime==primeinit){
        prime = next_prime(prime);
        msd->lp->p[0] = prime;
      }

      for(len_t i = 1; i < st->nthrds; i++){
        prime = next_prime(prime);
        msd->lp->p[i] = prime;
        while(is_lucky_prime_ui(prime, msd->bs_qq) || prime==primeinit){
          prime = next_prime(prime);
          msd->lp->p[i] = prime;
        }
      }
      prime = msd->lp->p[st->nthrds - 1];
      gb_modpoly_realloc(modgbs, st->nthrds);

      gb_modular_trace_application(modgbs, msd->mgb,
                                   msd->num_gb,
                                   msd->leadmons_ori,
                                   msd->leadmons_current,
                                   msd->btrace,
                                   msd->btht, msd->bs_qq, msd->blht, st,
                                   field_char, 0, /* info_level, */
                                   msd->bs, lmb_ori, *dquot_ptr, msd->lp,
                                   gens, &stf4, msd->bad_primes);

      /* display_gbmodpoly(stderr, modgbs); */

      nprimes += st->nthrds;

      if(nprimes == 1){
        if(info_level>2){
          fprintf(stderr, "------------------------------------------\n");
          fprintf(stderr, "#ADDITIONS       %13lu\n", (unsigned long)st->application_nr_add * 1000);
          fprintf(stderr, "#MULTIPLICATIONS %13lu\n", (unsigned long)st->application_nr_mult * 1000);
          fprintf(stderr, "#REDUCTIONS      %13lu\n", (unsigned long)st->application_nr_red);
          fprintf(stderr, "------------------------------------------\n");
        }
        if(info_level>1){
          fprintf(stderr, "Application phase %.2f Gops/sec\n",
                  (st->application_nr_add+st->application_nr_mult)/1000.0/1000.0/(stf4));
          fprintf(stderr, "Elapsed time: %.2f\n", stf4);
        }
      }
      int bad = 0;
      for(int i = 0; i < st->nthrds; i++){
        if(msd->bad_primes[i] == 1){
          fprintf(stderr, "badprimes[%d] is 1\n", i);
          bad = 1;
        }
      }
      int lstart = dlift->lstart;
      double ost_rrec = st_rrec;
      if(!bad){
        ratrecon_gb(modgbs, dlift, msd->mod_p, msd->prod_p, recdata, st->nthrds, &st_crt, &st_rrec);
      }
      if((st_rrec - ost_rrec) > dlift->rr * stf4){
        dlift->rr = 2*dlift->rr;
        if(info_level){
          fprintf(stderr, "(->%d)", dlift->rr);
        }
      }

      if(info_level){
        if(!(nprimes & (nprimes - 1))){
          fprintf(stderr, "{%d}", nprimes);
        }
      }
      if(dlift->lstart != lstart && dlift->lstart < modgbs->ld - 1){
        if(info_level){
          fprintf(stderr, "<%.2f%%>", 100* (float)(dlift->lstart + 1)/modgbs->ld);
        }
        lstart = dlift->lstart;
      }
#ifdef NEWGBLIFT
      if(dlift->lstart >= modgbs->ld){
        if(info_level){
          fprintf(stderr, "<100%%>\n");
          fprintf(stderr, "CRT time = %.2f, Rational reconstruction time= %.2f\n", st_crt, st_rrec);
        }
        apply = 0;
      }
#else
      if(dlift->lstart == modgbs->ld - 1 && dlift->check2){
        if(info_level){

          fprintf(stderr, "<%.2f%%>\n", 100* (float)(dlift->lstart + 1)/modgbs->ld);
        }
        apply = 0;
      }
#endif
      /* this is where learn could be reset to 1 */
      /* but then duplicated datas and others should be free-ed */
    }
  }
  if(info_level){
    fprintf(stderr, "%d primes used\n", nprimes);
  }
  free_mstrace(msd, st);
  if(dlinit){
    data_lift_clear(dlift);
  }

  if(files->out_file != NULL){
    FILE *ofile = fopen(files->out_file, "w+");
    display_gbmodpoly_cf_qq(ofile, modgbs);
    fclose(ofile);
  }

  gb_modpoly_clear(modgbs);
  free_rrec_data(recdata);

  free(st);

  return 0;
}
