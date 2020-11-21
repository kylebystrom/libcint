/*
 * Copyright (C) 2013-  Qiming Sun <osirpt.sun@gmail.com>
 *
 * basic cGTO integrals
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cint_bas.h"
#include "g2e.h"
#include "optimizer.h"
#include "cint2e.h"
#include "misc.h"
#include "cart2sph.h"
#include "c2f.h"

#define PRIM2CTR(ctrsymb, gp, ngp) \
        if (ctrsymb##_ctr > 1) {\
                if (*ctrsymb##empty) { \
                        CINTprim_to_ctr_0(gctr##ctrsymb, gp, c##ctrsymb+ctrsymb##p, \
                                          ngp, ctrsymb##_prim, ctrsymb##_ctr, \
                                          non0ctr##ctrsymb[ctrsymb##p], \
                                          non0idx##ctrsymb+ctrsymb##p*ctrsymb##_ctr); \
                } else { \
                        CINTprim_to_ctr_1(gctr##ctrsymb, gp, c##ctrsymb+ctrsymb##p, \
                                          ngp, ctrsymb##_prim, ctrsymb##_ctr, \
                                          non0ctr##ctrsymb[ctrsymb##p], \
                                          non0idx##ctrsymb+ctrsymb##p*ctrsymb##_ctr); \
                } \
        } \
        *ctrsymb##empty = 0

FINT CINT2e_loop_nopt(double *gctr, CINTEnvVars *envs, double *cache)
{
        /* COMMON_ENVS_AND_DECLARE */
        FINT *shls  = envs->shls;
        FINT *bas = envs->bas;
        double *env = envs->env;
        FINT i_sh = shls[0];
        FINT j_sh = shls[1];
        FINT k_sh = shls[2];
        FINT l_sh = shls[3];
        FINT i_ctr = envs->x_ctr[0];
        FINT j_ctr = envs->x_ctr[1];
        FINT k_ctr = envs->x_ctr[2];
        FINT l_ctr = envs->x_ctr[3];
        FINT i_prim = bas(NPRIM_OF, i_sh);
        FINT j_prim = bas(NPRIM_OF, j_sh);
        FINT k_prim = bas(NPRIM_OF, k_sh);
        FINT l_prim = bas(NPRIM_OF, l_sh);
        double *ri = envs->ri;
        double *rj = envs->rj;
        double *rk = envs->rk;
        double *rl = envs->rl;
        double *ai = env + bas(PTR_EXP, i_sh);
        double *aj = env + bas(PTR_EXP, j_sh);
        double *ak = env + bas(PTR_EXP, k_sh);
        double *al = env + bas(PTR_EXP, l_sh);
        double *ci = env + bas(PTR_COEFF, i_sh);
        double *cj = env + bas(PTR_COEFF, j_sh);
        double *ck = env + bas(PTR_COEFF, k_sh);
        double *cl = env + bas(PTR_COEFF, l_sh);
        double expcutoff = envs->expcutoff;
        double *log_maxci, *log_maxcj, *log_maxck, *log_maxcl;
        PairData *pdata_base, *pdata_ij;
        MALLOC_INSTACK(log_maxci, i_prim+j_prim+k_prim+l_prim);
        MALLOC_INSTACK(pdata_base, i_prim*j_prim);
        log_maxcj = log_maxci + i_prim;
        log_maxck = log_maxcj + j_prim;
        log_maxcl = log_maxck + k_prim;
        CINTOpt_log_max_pgto_coeff(log_maxci, ci, i_prim, i_ctr);
        CINTOpt_log_max_pgto_coeff(log_maxcj, cj, j_prim, j_ctr);
        if (CINTset_pairdata(pdata_base, ai, aj, envs->ri, envs->rj,
                             log_maxci, log_maxcj, envs->li_ceil, envs->lj_ceil,
                             i_prim, j_prim, SQUARE(envs->rirj), expcutoff)) {
                return 0;
        }
        CINTOpt_log_max_pgto_coeff(log_maxck, ck, k_prim, k_ctr);
        CINTOpt_log_max_pgto_coeff(log_maxcl, cl, l_prim, l_ctr);

        FINT n_comp = envs->ncomp_e1 * envs->ncomp_e2 * envs->ncomp_tensor;
        double fac1i, fac1j, fac1k, fac1l;
        FINT ip, jp, kp, lp;
        FINT empty[5] = {1, 1, 1, 1, 1};
        FINT *iempty = empty + 0;
        FINT *jempty = empty + 1;
        FINT *kempty = empty + 2;
        FINT *lempty = empty + 3;
        FINT *gempty = empty + 4;
        /* COMMON_ENVS_AND_DECLARE end */

        double rr_kl = SQUARE(envs->rkrl);
        double log_rr_kl = (envs->lk_ceil+envs->ll_ceil+1)*log(rr_kl+1)/2;
        double akl, ekl, expijkl, ccekl;
        double *rij;
        const double dist_ij = SQUARE(envs->rirj);
        const double dist_kl = SQUARE(envs->rkrl);

        FINT *idx;
        MALLOC_INSTACK(idx, envs->nf * 3);
        CINTg2e_index_xyz(idx, envs);

        FINT *non0ctri, *non0ctrj, *non0ctrk, *non0ctrl;
        FINT *non0idxi, *non0idxj, *non0idxk, *non0idxl;
        MALLOC_INSTACK(non0ctri, i_prim+j_prim+k_prim+l_prim+i_prim*i_ctr+j_prim*j_ctr+k_prim*k_ctr+l_prim*l_ctr);
        non0ctrj = non0ctri + i_prim;
        non0ctrk = non0ctrj + j_prim;
        non0ctrl = non0ctrk + k_prim;
        non0idxi = non0ctrl + l_prim;
        non0idxj = non0idxi + i_prim*i_ctr;
        non0idxk = non0idxj + j_prim*j_ctr;
        non0idxl = non0idxk + k_prim*k_ctr;
        CINTOpt_non0coeff_byshell(non0idxi, non0ctri, ci, i_prim, i_ctr);
        CINTOpt_non0coeff_byshell(non0idxj, non0ctrj, cj, j_prim, j_ctr);
        CINTOpt_non0coeff_byshell(non0idxk, non0ctrk, ck, k_prim, k_ctr);
        CINTOpt_non0coeff_byshell(non0idxl, non0ctrl, cl, l_prim, l_ctr);

        const FINT nc = i_ctr * j_ctr * k_ctr * l_ctr;
        // (irys,i,j,k,l,coord,0:1); +1 for nabla-r12
        const FINT leng = envs->g_size * 3 * ((1<<envs->gbits)+1);
        const FINT lenl = envs->nf * nc * n_comp; // gctrl
        const FINT lenk = envs->nf * i_ctr * j_ctr * k_ctr * n_comp; // gctrk
        const FINT lenj = envs->nf * i_ctr * j_ctr * n_comp; // gctrj
        const FINT leni = envs->nf * i_ctr * n_comp; // gctri
        const FINT len0 = envs->nf * n_comp; // gout
        const FINT len = leng + lenl + lenk + lenj + leni + len0;
        double *g;
        MALLOC_INSTACK(g, len);  // must be allocated last in this function
        double *g1 = g + leng;
        double *gout, *gctri, *gctrj, *gctrk, *gctrl;
        double eijcutoff;

        if (n_comp == 1) {
                gctrl = gctr;
        } else {
                gctrl = g1;
                g1 += lenl;
        }
        if (l_ctr == 1) {
                gctrk = gctrl;
                kempty = lempty;
        } else {
                gctrk = g1;
                g1 += lenk;
        }
        if (k_ctr == 1) {
                gctrj = gctrk;
                jempty = kempty;
        } else {
                gctrj = g1;
                g1 += lenj;
        }
        if (j_ctr == 1) {
                gctri = gctrj;
                iempty = jempty;
        } else {
                gctri = g1;
                g1 += leni;
        }
        if (i_ctr == 1) {
                gout = gctri;
                gempty = iempty;
        } else {
                gout = g1;
        }

        *lempty = 1;
        for (lp = 0; lp < l_prim; lp++) {
                envs->al = al[lp];
                if (l_ctr == 1) {
                        fac1l = envs->common_factor * cl[lp];
                } else {
                        fac1l = envs->common_factor;
                        *kempty = 1;
                }
                for (kp = 0; kp < k_prim; kp++) {
                        akl = ak[kp] + al[lp];
                        ekl = dist_kl * ak[kp] * al[lp] / akl;
                        ccekl = ekl - log_rr_kl - log_maxck[kp] - log_maxcl[lp];
                        if (ccekl > expcutoff) {
                                goto k_contracted;
                        }
                        envs->ak = ak[kp];
                        envs->akl = akl;
                        envs->rkl[0] = (ak[kp]*rk[0] + al[lp]*rl[0]) / envs->akl;
                        envs->rkl[1] = (ak[kp]*rk[1] + al[lp]*rl[1]) / envs->akl;
                        envs->rkl[2] = (ak[kp]*rk[2] + al[lp]*rl[2]) / envs->akl;
                        envs->rklrx[0] = envs->rkl[0] - envs->rx_in_rklrx[0];
                        envs->rklrx[1] = envs->rkl[1] - envs->rx_in_rklrx[1];
                        envs->rklrx[2] = envs->rkl[2] - envs->rx_in_rklrx[2];
                        eijcutoff = expcutoff - MAX(ccekl, 0);
                        ekl = exp(-ekl);

                        if (k_ctr == 1) {
                                fac1k = fac1l * ck[kp];
                        } else {
                                fac1k = fac1l;
                                *jempty = 1;
                        }

                        pdata_ij = pdata_base;
                        for (jp = 0; jp < j_prim; jp++) {
                                envs->aj = aj[jp];
                                if (j_ctr == 1) {
                                        fac1j = fac1k * cj[jp];
                                } else {
                                        fac1j = fac1k;
                                        *iempty = 1;
                                }
                                for (ip = 0; ip < i_prim; ip++, pdata_ij++) {
                                        if (pdata_ij->cceij > eijcutoff) {
                                                goto i_contracted;
                                        }
                                        envs->ai = ai[ip];
                                        envs->aij = ai[ip] + aj[jp];
                                        rij = pdata_ij->rij;
                                        envs->rij[0] = rij[0];
                                        envs->rij[1] = rij[1];
                                        envs->rij[2] = rij[2];
                                        envs->rijrx[0] = rij[0] - envs->rx_in_rijrx[0];
                                        envs->rijrx[1] = rij[1] - envs->rx_in_rijrx[1];
                                        envs->rijrx[2] = rij[2] - envs->rx_in_rijrx[2];
                                        expijkl = pdata_ij->eij * ekl;
                                        if (i_ctr == 1) {
                                                fac1i = fac1j*ci[ip]*expijkl;
                                        } else {
                                                fac1i = fac1j*expijkl;
                                        }
                                        if ((*envs->f_g0_2e)(g, fac1i, envs)) {
                                                (*envs->f_gout)(gout, g, idx, envs, *gempty);
                                                PRIM2CTR(i, gout, envs->nf*n_comp);
                                        }
i_contracted: ;
                                } // end loop i_prim
                                if (!*iempty) {
                                        PRIM2CTR(j, gctri, envs->nf*i_ctr*n_comp);
                                }
                        } // end loop j_prim
                        if (!*jempty) {
                                PRIM2CTR(k, gctrj,envs->nf*i_ctr*j_ctr*n_comp);
                        }
k_contracted: ;
                } // end loop k_prim
                if (!*kempty) {
                        PRIM2CTR(l, gctrk, envs->nf*i_ctr*j_ctr*k_ctr*n_comp);
                }
        } // end loop l_prim

        if (n_comp > 1 && !*lempty) {
                CINTdmat_transpose(gctr, gctrl, envs->nf*nc, n_comp);
        }
        return !*lempty;
}


#define COMMON_ENVS_AND_DECLARE \
        FINT *shls = envs->shls; \
        FINT *bas = envs->bas; \
        double *env = envs->env; \
        FINT i_sh = shls[0]; \
        FINT j_sh = shls[1]; \
        FINT k_sh = shls[2]; \
        FINT l_sh = shls[3]; \
        if (opt->pairdata != NULL && \
            ((opt->pairdata[i_sh*opt->nbas+j_sh] == NOVALUE) || \
             (opt->pairdata[k_sh*opt->nbas+l_sh] == NOVALUE))) { \
                return 0; \
        } \
        FINT i_ctr = envs->x_ctr[0]; \
        FINT j_ctr = envs->x_ctr[1]; \
        FINT k_ctr = envs->x_ctr[2]; \
        FINT l_ctr = envs->x_ctr[3]; \
        FINT i_prim = bas(NPRIM_OF, i_sh); \
        FINT j_prim = bas(NPRIM_OF, j_sh); \
        FINT k_prim = bas(NPRIM_OF, k_sh); \
        FINT l_prim = bas(NPRIM_OF, l_sh); \
        double *ai = env + bas(PTR_EXP, i_sh); \
        double *aj = env + bas(PTR_EXP, j_sh); \
        double *ak = env + bas(PTR_EXP, k_sh); \
        double *al = env + bas(PTR_EXP, l_sh); \
        double *ci = env + bas(PTR_COEFF, i_sh); \
        double *cj = env + bas(PTR_COEFF, j_sh); \
        double *ck = env + bas(PTR_COEFF, k_sh); \
        double *cl = env + bas(PTR_COEFF, l_sh); \
        double expcutoff = envs->expcutoff; \
        PairData *_pdata_ij, *_pdata_kl, *pdata_ij, *pdata_kl; \
        if (opt->pairdata != NULL) { \
                _pdata_ij = opt->pairdata[i_sh*opt->nbas+j_sh]; \
                _pdata_kl = opt->pairdata[k_sh*opt->nbas+l_sh]; \
        } else { \
                double *log_maxci = opt->log_max_coeff[i_sh]; \
                double *log_maxcj = opt->log_max_coeff[j_sh]; \
                MALLOC_INSTACK(_pdata_ij, i_prim*j_prim + k_prim*l_prim); \
                if (CINTset_pairdata(_pdata_ij, ai, aj, envs->ri, envs->rj, \
                                     log_maxci, log_maxcj, envs->li_ceil, envs->lj_ceil, \
                                     i_prim, j_prim, SQUARE(envs->rirj), expcutoff)) { \
                        return 0; \
                } \
                double *log_maxck = opt->log_max_coeff[k_sh]; \
                double *log_maxcl = opt->log_max_coeff[l_sh]; \
                _pdata_kl = _pdata_ij + i_prim*j_prim; \
                if (CINTset_pairdata(_pdata_kl, ak, al, envs->rk, envs->rl, \
                                     log_maxck, log_maxcl, envs->lk_ceil, envs->ll_ceil, \
                                     k_prim, l_prim, SQUARE(envs->rkrl), expcutoff)) { \
                        return 0; \
                } \
        } \
        FINT n_comp = envs->ncomp_e1 * envs->ncomp_e2 * envs->ncomp_tensor; \
        double fac1i, fac1j, fac1k, fac1l; \
        FINT ip, jp, kp, lp; \
        FINT empty[5] = {1, 1, 1, 1, 1}; \
        FINT *iempty = empty + 0; \
        FINT *jempty = empty + 1; \
        FINT *kempty = empty + 2; \
        FINT *lempty = empty + 3; \
        FINT *gempty = empty + 4; \
        FINT *non0ctri = opt->non0ctr[i_sh]; \
        FINT *non0ctrj = opt->non0ctr[j_sh]; \
        FINT *non0ctrk = opt->non0ctr[k_sh]; \
        FINT *non0ctrl = opt->non0ctr[l_sh]; \
        FINT *non0idxi = opt->sortedidx[i_sh]; \
        FINT *non0idxj = opt->sortedidx[j_sh]; \
        FINT *non0idxk = opt->sortedidx[k_sh]; \
        FINT *non0idxl = opt->sortedidx[l_sh]; \
        double expij, expkl, eijcutoff, eklcutoff; \
        eklcutoff = expcutoff; \
        double *rij, *rkl; \
        FINT *idx = opt->index_xyz_array[envs->i_l*LMAX1*LMAX1*LMAX1 \
                                        +envs->j_l*LMAX1*LMAX1 \
                                        +envs->k_l*LMAX1 \
                                        +envs->l_l]; \
        if (idx == NULL) { \
                MALLOC_INSTACK(idx, envs->nf * 3); \
                CINTg2e_index_xyz(idx, envs); \
        }

#define SET_RIJ(I,J)    \
        if (pdata_##I##J->cceij > e##I##J##cutoff) { \
                goto I##_contracted; } \
        envs->a##I = a##I[I##p]; \
        envs->a##I##J = a##I[I##p] + a##J[J##p]; \
        exp##I##J = pdata_##I##J->eij; \
        r##I##J = pdata_##I##J->rij; \
        envs->r##I##J[0] = r##I##J[0]; \
        envs->r##I##J[1] = r##I##J[1]; \
        envs->r##I##J[2] = r##I##J[2]; \
        envs->r##I##J##rx[0] = r##I##J[0] - envs->rx_in_r##I##J##rx[0]; \
        envs->r##I##J##rx[1] = r##I##J[1] - envs->rx_in_r##I##J##rx[1]; \
        envs->r##I##J##rx[2] = r##I##J[2] - envs->rx_in_r##I##J##rx[2];

// i_ctr = j_ctr = k_ctr = l_ctr = 1;
FINT CINT2e_1111_loop(double *gctr, CINTEnvVars *envs, CINTOpt *opt, double *cache)
{
        COMMON_ENVS_AND_DECLARE;
        const FINT nc = 1;
        const FINT leng = envs->g_size * 3 * ((1<<envs->gbits)+1);
        const FINT len0 = envs->nf * n_comp;
        const FINT len = leng + len0;
        double *gout;
        double *g;
        MALLOC_INSTACK(g, len);
        if (n_comp == 1) {
                gout = gctr;
        } else {
                gout = g + leng;
        }

        pdata_kl = _pdata_kl;
        for (lp = 0; lp < l_prim; lp++) {
                envs->al = al[lp];
                fac1l = envs->common_factor * cl[lp];
                for (kp = 0; kp < k_prim; kp++, pdata_kl++) {
                        SET_RIJ(k, l);
                        fac1k = fac1l * ck[kp];
                        eijcutoff = eklcutoff - MAX(pdata_kl->cceij, 0);
                        pdata_ij = _pdata_ij;
                        for (jp = 0; jp < j_prim; jp++) {
                                envs->aj = aj[jp];
                                fac1j = fac1k * cj[jp];
                                for (ip = 0; ip < i_prim; ip++, pdata_ij++) {
                                        SET_RIJ(i, j);
                                        fac1i = fac1j*ci[ip]*expij*expkl;
                                        if ((*envs->f_g0_2e)(g, fac1i, envs)) {
                                                (*envs->f_gout)(gout, g, idx, envs, *empty);
                                                *empty = 0;
                                        }
i_contracted: ;
                                } // end loop i_prim
                        } // end loop j_prim
k_contracted: ;
                } // end loop k_prim
        } // end loop l_prim

        if (n_comp > 1 && !*empty) {
                CINTdmat_transpose(gctr, gout, envs->nf*nc, n_comp);
        }
        return !*empty;
}

// i_ctr = n; j_ctr = k_ctr = l_ctr = 1;
FINT CINT2e_n111_loop(double *gctr, CINTEnvVars *envs, CINTOpt *opt, double *cache)
{
        COMMON_ENVS_AND_DECLARE;

        const FINT nc = i_ctr;
        const FINT leng = envs->g_size * 3 * ((1<<envs->gbits)+1);
        const FINT leni = envs->nf * i_ctr * n_comp; // gctri
        const FINT len0 = envs->nf * n_comp; // gout
        const FINT len = leng + leni + len0;
        double *g;
        MALLOC_INSTACK(g, len);
        double *g1 = g + leng;
        double *gout, *gctri;
        if (n_comp == 1) {
                gctri = gctr;
        } else {
                gctri = g1;
                g1 += leni;
        }
        gout = g1;

        pdata_kl = _pdata_kl;
        for (lp = 0; lp < l_prim; lp++) {
                envs->al = al[lp];
                fac1l = envs->common_factor * cl[lp];
                for (kp = 0; kp < k_prim; kp++, pdata_kl++) {
                        SET_RIJ(k, l);
                        fac1k = fac1l * ck[kp];
                        eijcutoff = eklcutoff - MAX(pdata_kl->cceij, 0);
                        pdata_ij = _pdata_ij;
                        for (jp = 0; jp < j_prim; jp++) {
                                envs->aj = aj[jp];
                                fac1j = fac1k * cj[jp];
                                for (ip = 0; ip < i_prim; ip++, pdata_ij++) {
                                        if (pdata_ij->cceij > eijcutoff) {
                                                goto i_contracted;
                                        }
                                        SET_RIJ(i, j);
                                        fac1i = fac1j*expij*expkl;
                                        if ((*envs->f_g0_2e)(g, fac1i, envs)) {
                                                (*envs->f_gout)(gout, g, idx, envs, 1);
                                                PRIM2CTR(i, gout,envs->nf*n_comp);
                                        }
i_contracted: ;
                                } // end loop i_prim
                        } // end loop j_prim
k_contracted: ;
                } // end loop k_prim
        } // end loop l_prim

        if (n_comp > 1 && !*iempty) {
                CINTdmat_transpose(gctr, gctri, envs->nf*nc, n_comp);
        }
        return !*iempty;
}

// j_ctr = n; i_ctr = k_ctr = l_ctr = 1;
FINT CINT2e_1n11_loop(double *gctr, CINTEnvVars *envs, CINTOpt *opt, double *cache)
{
        COMMON_ENVS_AND_DECLARE;

        const FINT nc = j_ctr;
        const FINT leng = envs->g_size * 3 * ((1<<envs->gbits)+1);
        const FINT lenj = envs->nf * j_ctr * n_comp; // gctrj
        const FINT len0 = envs->nf * n_comp; // gout
        const FINT len = leng + lenj + len0;
        double *g;
        MALLOC_INSTACK(g, len);
        double *g1 = g + leng;
        double *gout, *gctrj;
        if (n_comp == 1) {
                gctrj = gctr;
        } else {
                gctrj = g1;
                g1 += lenj;
        }
        gout = g1;

        pdata_kl = _pdata_kl;
        for (lp = 0; lp < l_prim; lp++) {
                envs->al = al[lp];
                fac1l = envs->common_factor * cl[lp];
                for (kp = 0; kp < k_prim; kp++, pdata_kl++) {
                        SET_RIJ(k, l);
                        fac1k = fac1l * ck[kp];
                        eijcutoff = eklcutoff - MAX(pdata_kl->cceij, 0);
                        pdata_ij = _pdata_ij;
                        for (jp = 0; jp < j_prim; jp++) {
                                envs->aj = aj[jp];
                                fac1j = fac1k;
                                *iempty = 1;
                                for (ip = 0; ip < i_prim; ip++, pdata_ij++) {
                                        SET_RIJ(i, j);
                                        fac1i = fac1j*ci[ip]*expij*expkl;
                                        if ((*envs->f_g0_2e)(g, fac1i, envs)) {
                                                (*envs->f_gout)(gout, g, idx, envs, *iempty);
                                                *iempty = 0;
                                        }
i_contracted: ;
                                } // end loop i_prim
                                if (!*iempty) {
                                        PRIM2CTR(j, gout,envs->nf*n_comp);
                                }
                        } // end loop j_prim
k_contracted: ;
                } // end loop k_prim
        } // end loop l_prim

        if (n_comp > 1 && !*jempty) {
                CINTdmat_transpose(gctr, gctrj, envs->nf*nc, n_comp);
        }
        return !*jempty;
}

// k_ctr = n; i_ctr = j_ctr = l_ctr = 1;
FINT CINT2e_11n1_loop(double *gctr, CINTEnvVars *envs, CINTOpt *opt, double *cache)
{
        COMMON_ENVS_AND_DECLARE;

        const FINT nc = k_ctr;
        const FINT leng = envs->g_size * 3 * ((1<<envs->gbits)+1);
        const FINT lenk = envs->nf * k_ctr * n_comp; // gctrk
        const FINT len0 = envs->nf * n_comp; // gout
        const FINT len = leng + lenk + len0;
        double *g;
        MALLOC_INSTACK(g, len);
        double *g1 = g + leng;
        double *gout, *gctrk;
        if (n_comp == 1) {
                gctrk = gctr;
        } else {
                gctrk = g1;
                g1 += lenk;
        }
        gout = g1;

        pdata_kl = _pdata_kl;
        for (lp = 0; lp < l_prim; lp++) {
                envs->al = al[lp];
                fac1l = envs->common_factor * cl[lp];
                for (kp = 0; kp < k_prim; kp++, pdata_kl++) {
                        SET_RIJ(k, l);
                        fac1k = fac1l;
                        eijcutoff = eklcutoff - MAX(pdata_kl->cceij, 0);
                        pdata_ij = _pdata_ij;
                        *jempty = 1;
                        for (jp = 0; jp < j_prim; jp++) {
                                envs->aj = aj[jp];
                                fac1j = fac1k * cj[jp];
                                for (ip = 0; ip < i_prim; ip++, pdata_ij++) {
                                        SET_RIJ(i, j);
                                        fac1i = fac1j*ci[ip]*expij*expkl;
                                        if ((*envs->f_g0_2e)(g, fac1i, envs)) {
                                                (*envs->f_gout)(gout, g, idx, envs, *jempty);
                                                *jempty = 0;
                                        }
i_contracted: ;
                                } // end loop i_prim
                        } // end loop j_prim
                        if (!*jempty) {
                                PRIM2CTR(k, gout,envs->nf*n_comp);
                        }
k_contracted: ;
                } // end loop k_prim
        } // end loop l_prim

        if (n_comp > 1 && !*kempty) {
                CINTdmat_transpose(gctr, gctrk, envs->nf*nc, n_comp);
        }
        return !*kempty;
}

// l_ctr = n; i_ctr = j_ctr = k_ctr = 1;
FINT CINT2e_111n_loop(double *gctr, CINTEnvVars *envs, CINTOpt *opt, double *cache)
{
        COMMON_ENVS_AND_DECLARE;

        const FINT nc = l_ctr;
        const FINT leng = envs->g_size * 3 * ((1<<envs->gbits)+1);
        const FINT lenl = envs->nf * l_ctr * n_comp; // gctrl
        const FINT len0 = envs->nf * n_comp; // gout
        const FINT len = leng + lenl + len0;
        double *g;
        MALLOC_INSTACK(g, len);
        double *g1 = g + leng;
        double *gout, *gctrl;
        if (n_comp == 1) {
                gctrl = gctr;
        } else {
                gctrl = g1;
                g1 += lenl;
        }
        gout = g1;

        pdata_kl = _pdata_kl;
        for (lp = 0; lp < l_prim; lp++) {
                envs->al = al[lp];
                fac1l = envs->common_factor;
                *kempty = 1;
                for (kp = 0; kp < k_prim; kp++, pdata_kl++) {
                        SET_RIJ(k, l);
                        fac1k = fac1l * ck[kp];
                        eijcutoff = eklcutoff - MAX(pdata_kl->cceij, 0);
                        pdata_ij = _pdata_ij;
                        for (jp = 0; jp < j_prim; jp++) {
                                envs->aj = aj[jp];
                                fac1j = fac1k * cj[jp];
                                for (ip = 0; ip < i_prim; ip++, pdata_ij++) {
                                        SET_RIJ(i, j);
                                        fac1i = fac1j*ci[ip]*expij*expkl;
                                        if ((*envs->f_g0_2e)(g, fac1i, envs)) {
                                                (*envs->f_gout)(gout, g, idx, envs, *kempty);
                                                *kempty = 0;
                                        }
i_contracted: ;
                                } // end loop i_prim
                        } // end loop j_prim
k_contracted: ;
                } // end loop k_prim
                if (!*kempty) {
                        PRIM2CTR(l, gout,envs->nf*n_comp);
                }
        } // end loop l_prim

        if (n_comp > 1 && !*lempty) {
                CINTdmat_transpose(gctr, gctrl, envs->nf*nc, n_comp);
        }
        return !*lempty;
}


FINT CINT2e_loop(double *gctr, CINTEnvVars *envs, CINTOpt *opt, double *cache)
{
        COMMON_ENVS_AND_DECLARE;
        const FINT nc = i_ctr * j_ctr * k_ctr * l_ctr;
        const FINT leng = envs->g_size * 3 * ((1<<envs->gbits)+1); // (irys,i,j,k,l,coord,0:1);
        const FINT lenl = envs->nf * nc * n_comp; // gctrl
        const FINT lenk = envs->nf * i_ctr * j_ctr * k_ctr * n_comp; // gctrk
        const FINT lenj = envs->nf * i_ctr * j_ctr * n_comp; // gctrj
        const FINT leni = envs->nf * i_ctr * n_comp; // gctri
        const FINT len0 = envs->nf * n_comp; // gout
        const FINT len = leng + lenl + lenk + lenj + leni + len0;
        double *g;
        MALLOC_INSTACK(g, len);
        double *g1 = g + leng;
        double *gout, *gctri, *gctrj, *gctrk, *gctrl;

        if (n_comp == 1) {
                gctrl = gctr;
        } else {
                gctrl = g1;
                g1 += lenl;
        }
        if (l_ctr == 1) {
                gctrk = gctrl;
                kempty = lempty;
        } else {
                gctrk = g1;
                g1 += lenk;
        }
        if (k_ctr == 1) {
                gctrj = gctrk;
                jempty = kempty;
        } else {
                gctrj = g1;
                g1 += lenj;
        }
        if (j_ctr == 1) {
                gctri = gctrj;
                iempty = jempty;
        } else {
                gctri = g1;
                g1 += leni;
        }
        if (i_ctr == 1) {
                gout = gctri;
                gempty = iempty;
        } else {
                gout = g1;
        }

        pdata_kl = _pdata_kl;
        for (lp = 0; lp < l_prim; lp++) {
                envs->al = al[lp];
                if (l_ctr == 1) {
                        fac1l = envs->common_factor * cl[lp];
                } else {
                        fac1l = envs->common_factor;
                        *kempty = 1;
                }
                for (kp = 0; kp < k_prim; kp++, pdata_kl++) {
                        /* SET_RIJ(k, l); */
                        if (pdata_kl->cceij > eklcutoff) {
                                goto k_contracted;
                        }
                        envs->ak = ak[kp];
                        envs->akl = ak[kp] + al[lp];
                        expkl = pdata_kl->eij;
                        rkl = pdata_kl->rij;
                        envs->rkl[0] = rkl[0];
                        envs->rkl[1] = rkl[1];
                        envs->rkl[2] = rkl[2];
                        envs->rklrx[0] = rkl[0] - envs->rx_in_rklrx[0];
                        envs->rklrx[1] = rkl[1] - envs->rx_in_rklrx[1];
                        envs->rklrx[2] = rkl[2] - envs->rx_in_rklrx[2];
                        eijcutoff = eklcutoff - MAX(pdata_kl->cceij, 0);
                        /* SET_RIJ(k, l); end */
                        if (k_ctr == 1) {
                                fac1k = fac1l * ck[kp];
                        } else {
                                fac1k = fac1l;
                                *jempty = 1;
                        }

                        pdata_ij = _pdata_ij;
                        for (jp = 0; jp < j_prim; jp++) {
                                envs->aj = aj[jp];
                                if (j_ctr == 1) {
                                        fac1j = fac1k * cj[jp];
                                } else {
                                        fac1j = fac1k;
                                        *iempty = 1;
                                }
                                for (ip = 0; ip < i_prim; ip++, pdata_ij++) {
                                        /* SET_RIJ(i, j); */
                                        if (pdata_ij->cceij > eijcutoff) {
                                                goto i_contracted;
                                        }
                                        envs->ai = ai[ip];
                                        envs->aij = ai[ip] + aj[jp];
                                        expij = pdata_ij->eij;
                                        rij = pdata_ij->rij;
                                        envs->rij[0] = rij[0];
                                        envs->rij[1] = rij[1];
                                        envs->rij[2] = rij[2];
                                        envs->rijrx[0] = rij[0] - envs->rx_in_rijrx[0];
                                        envs->rijrx[1] = rij[1] - envs->rx_in_rijrx[1];
                                        envs->rijrx[2] = rij[2] - envs->rx_in_rijrx[2];
                                        /* SET_RIJ(i, j); end */
                                        if (i_ctr == 1) {
                                                fac1i = fac1j*ci[ip] * expij*expkl;
                                        } else {
                                                fac1i = fac1j * expij*expkl;
                                        }
                                        if ((*envs->f_g0_2e)(g, fac1i, envs)) {
                                                (*envs->f_gout)(gout, g, idx, envs, *gempty);
                                                PRIM2CTR(i, gout, envs->nf*n_comp);
                                        }
i_contracted: ;
                                } // end loop i_prim
                                if (!*iempty) {
                                        PRIM2CTR(j, gctri, envs->nf*i_ctr*n_comp);
                                }
                        } // end loop j_prim
                        if (!*jempty) {
                                PRIM2CTR(k, gctrj, envs->nf*i_ctr*j_ctr*n_comp);
                        }
k_contracted: ;
                } // end loop k_prim
                if (!*kempty) {
//TODO: merge this contraction with COPY_AND_CLOSING for n_comp>1
                        PRIM2CTR(l, gctrk, envs->nf*i_ctr*j_ctr*k_ctr*n_comp);
                }
        } // end loop l_prim

        if (n_comp > 1 && !*lempty) {
                CINTdmat_transpose(gctr, gctrl, envs->nf*nc, n_comp);
        }
        return !*lempty;
}


static FINT (*CINTf_2e_loop[16])() = {
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_n111_loop,
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_loop,
        CINT2e_1n11_loop,
        CINT2e_loop,
        CINT2e_11n1_loop,
        CINT2e_111n_loop,
        CINT2e_1111_loop,
};

#define PAIRDATA_NON0IDX_SIZE(ps) \
                FINT *bas = envs->bas; \
                FINT *shls  = envs->shls; \
                FINT i_prim = bas(NPRIM_OF, shls[0]); \
                FINT j_prim = bas(NPRIM_OF, shls[1]); \
                FINT k_prim = bas(NPRIM_OF, shls[2]); \
                FINT l_prim = bas(NPRIM_OF, shls[3]); \
                FINT ps = ((i_prim*j_prim + k_prim*l_prim) * 5 \
                           + i_prim * x_ctr[0] \
                           + j_prim * x_ctr[1] \
                           + k_prim * x_ctr[2] \
                           + l_prim * x_ctr[3] \
                           +(i_prim+j_prim+k_prim+l_prim)*2 + envs->nf*3);

FINT CINT2e_cart_drv(double *out, FINT *dims, CINTEnvVars *envs, CINTOpt *opt,
                    double *cache)
{
        FINT *x_ctr = envs->x_ctr;
        FINT nc = envs->nf * x_ctr[0] * x_ctr[1] * x_ctr[2] * x_ctr[3];
        FINT n_comp = envs->ncomp_e1 * envs->ncomp_e2 * envs->ncomp_tensor;
        if (out == NULL) {
                PAIRDATA_NON0IDX_SIZE(pdata_size);
                FINT leng = envs->g_size*3*((1<<envs->gbits)+1);
                FINT len0 = envs->nf*n_comp;
                FINT cache_size = leng + len0 + nc*n_comp*3 + pdata_size;
                return cache_size;
        }
        double *stack = NULL;
        if (cache == NULL) {
                PAIRDATA_NON0IDX_SIZE(pdata_size);
                FINT leng = envs->g_size*3*((1<<envs->gbits)+1);
                FINT len0 = envs->nf*n_comp;
                FINT cache_size = leng + len0 + nc*n_comp*3 + pdata_size;
                stack = malloc(sizeof(double)*cache_size);
                cache = stack;
        }
        double *gctr;
        MALLOC_INSTACK(gctr, nc*n_comp);

        FINT n, has_value;
        if (opt != NULL) {
                n = ((x_ctr[0]==1) << 3) + ((x_ctr[1]==1) << 2)
                  + ((x_ctr[2]==1) << 1) +  (x_ctr[3]==1);
                has_value = CINTf_2e_loop[n](gctr, envs, opt, cache);
        } else {
                has_value = CINT2e_loop_nopt(gctr, envs, cache);
        }

        FINT counts[4];
        counts[0] = envs->nfi * x_ctr[0];
        counts[1] = envs->nfj * x_ctr[1];
        counts[2] = envs->nfk * x_ctr[2];
        counts[3] = envs->nfl * x_ctr[3];
        if (dims == NULL) {
                dims = counts;
        }
        FINT nout = dims[0] * dims[1] * dims[2] * dims[3];
        if (has_value) {
                for (n = 0; n < n_comp; n++) {
                        c2s_cart_2e1(out+nout*n, gctr+nc*n, dims, envs, cache);
                }
        } else {
                for (n = 0; n < n_comp; n++) {
                        c2s_dset0(out+nout*n, dims, counts);
                }
        }
        if (stack != NULL) {
                free(stack);
        }
        return has_value;
}
FINT CINT2e_spheric_drv(double *out, FINT *dims, CINTEnvVars *envs, CINTOpt *opt,
                       double *cache)
{
        FINT *x_ctr = envs->x_ctr;
        FINT nc = envs->nf * x_ctr[0] * x_ctr[1] * x_ctr[2] * x_ctr[3];
        FINT n_comp = envs->ncomp_e1 * envs->ncomp_e2 * envs->ncomp_tensor;
        if (out == NULL) {
                PAIRDATA_NON0IDX_SIZE(pdata_size);
                FINT leng = envs->g_size*3*((1<<envs->gbits)+1);
                FINT len0 = envs->nf*n_comp;
                FINT cache_size = MAX(leng+len0+nc*n_comp*3 + pdata_size,
                                      nc*n_comp+envs->nf*4);
                return cache_size;
        }
        double *stack = NULL;
        if (cache == NULL) {
                PAIRDATA_NON0IDX_SIZE(pdata_size);
                FINT leng = envs->g_size*3*((1<<envs->gbits)+1);
                FINT len0 = envs->nf*n_comp;
                FINT cache_size = MAX(leng+len0+nc*n_comp*3 + pdata_size,
                                      nc*n_comp+envs->nf*4);
                stack = malloc(sizeof(double)*cache_size);
                cache = stack;
        }
        double *gctr;
        MALLOC_INSTACK(gctr, nc*n_comp);

        FINT n, has_value;
        if (opt != NULL) {
                n = ((x_ctr[0]==1) << 3) + ((x_ctr[1]==1) << 2)
                  + ((x_ctr[2]==1) << 1) +  (x_ctr[3]==1);
                has_value = CINTf_2e_loop[n](gctr, envs, opt, cache);
        } else {
                has_value = CINT2e_loop_nopt(gctr, envs, cache);
        }

        FINT counts[4];
        counts[0] = (envs->i_l*2+1) * x_ctr[0];
        counts[1] = (envs->j_l*2+1) * x_ctr[1];
        counts[2] = (envs->k_l*2+1) * x_ctr[2];
        counts[3] = (envs->l_l*2+1) * x_ctr[3];
        if (dims == NULL) {
                dims = counts;
        }
        FINT nout = dims[0] * dims[1] * dims[2] * dims[3];
        if (has_value) {
                for (n = 0; n < n_comp; n++) {
                        c2s_sph_2e1(out+nout*n, gctr+nc*n, dims, envs, cache);
                }
        } else {
                for (n = 0; n < n_comp; n++) {
                        c2s_dset0(out+nout*n, dims, counts);
                }
        }
        if (stack != NULL) {
                free(stack);
        }
        return has_value;
}
FINT CINT2e_spinor_drv(double complex *out, FINT *dims, CINTEnvVars *envs, CINTOpt *opt,
                      double *cache, void (*f_e1_c2s)(), void (*f_e2_c2s)())
{
        FINT *shls = envs->shls;
        FINT *bas = envs->bas;
        FINT counts[4];
        counts[0] = CINTcgto_spinor(shls[0], bas);
        counts[1] = CINTcgto_spinor(shls[1], bas);
        counts[2] = CINTcgto_spinor(shls[2], bas);
        counts[3] = CINTcgto_spinor(shls[3], bas);
        FINT *x_ctr = envs->x_ctr;
        FINT nc = envs->nf * x_ctr[0] * x_ctr[1] * x_ctr[2] * x_ctr[3];
        FINT n_comp = envs->ncomp_e1 * envs->ncomp_e2 * envs->ncomp_tensor;
        FINT n1 = counts[0] * envs->nfk * x_ctr[2]
                           * envs->nfl * x_ctr[3] * counts[1];
        if (out == NULL) {
                PAIRDATA_NON0IDX_SIZE(pdata_size);
                FINT leng = envs->g_size*3*((1<<envs->gbits)+1);
                FINT len0 = envs->nf*n_comp;
                FINT cache_size = MAX(leng+len0+nc*n_comp*3 + pdata_size,
                                     nc*n_comp + n1*envs->ncomp_e2*OF_CMPLX
                                     + envs->nf*32*OF_CMPLX);
                return cache_size;
        }
        double *stack = NULL;
        if (cache == NULL) {
                PAIRDATA_NON0IDX_SIZE(pdata_size);
                FINT leng = envs->g_size*3*((1<<envs->gbits)+1);
                FINT len0 = envs->nf*n_comp;
                FINT cache_size = MAX(leng+len0+nc*n_comp*3 + pdata_size,
                                     nc*n_comp + n1*envs->ncomp_e2*OF_CMPLX
                                     + envs->nf*32*OF_CMPLX);
                stack = malloc(sizeof(double)*cache_size);
                cache = stack;
        }
        double *gctr;
        MALLOC_INSTACK(gctr, nc*n_comp);

        FINT n, m, has_value;
        if (opt != NULL) {
                n = ((x_ctr[0]==1) << 3) + ((x_ctr[1]==1) << 2)
                  + ((x_ctr[2]==1) << 1) +  (x_ctr[3]==1);
                has_value = CINTf_2e_loop[n](gctr, envs, opt, cache);
        } else {
                has_value = CINT2e_loop_nopt(gctr, envs, cache);
        }

        if (dims == NULL) {
                dims = counts;
        }
        FINT nout = dims[0] * dims[1] * dims[2] * dims[3];
        if (has_value) {
                double complex *opij;
                MALLOC_INSTACK(opij, n1*envs->ncomp_e2);
                for (n = 0; n < envs->ncomp_tensor; n++) {
                        for (m = 0; m < envs->ncomp_e2; m++) {
                                (*f_e1_c2s)(opij+n1*m, gctr, dims, envs, cache);
                                gctr += nc * envs->ncomp_e1;
                        }
                        (*f_e2_c2s)(out+nout*n, opij, dims, envs, cache);
                }
        } else {
                for (n = 0; n < envs->ncomp_tensor; n++) {
                        c2s_zset0(out+nout*n, dims, counts);
                }
        }
        if (stack != NULL) {
                free(stack);
        }
        return has_value;
}


/*
 * <ki|jl> = (ij|kl); i,j\in electron 1; k,l\in electron 2
 */
void CINTgout2e(double *gout, double *g, FINT *idx,
                CINTEnvVars *envs, FINT gout_empty)
{
        FINT nf = envs->nf;
        FINT i, ix, iy, iz, n;
        double s;

        if (gout_empty) {
                switch (envs->nrys_roots) {
                        case 1:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix] * g[iy] * g[iz];
                                }
                                break;
                        case 2:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1];
                                }
                                break;
                        case 3:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2];
                                }
                                break;
                        case 4:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3];
                                }
                                break;
                        case 5:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4];
                                }
                                break;
                        case 6:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4]
                                                + g[ix+5] * g[iy+5] * g[iz+5];
                                }
                                break;
                        case 7:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4]
                                                + g[ix+5] * g[iy+5] * g[iz+5]
                                                + g[ix+6] * g[iy+6] * g[iz+6];
                                }
                                break;
                        case 8:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] = g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4]
                                                + g[ix+5] * g[iy+5] * g[iz+5]
                                                + g[ix+6] * g[iy+6] * g[iz+6]
                                                + g[ix+7] * g[iy+7] * g[iz+7];
                                }
                                break;
                        default:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        s = 0;
                                        for (i = 0; i < envs->nrys_roots; i++) {
                                                s += g[ix+i] * g[iy+i] * g[iz+i];
                                        }
                                        gout[n] = s;
                                }
                                break;
                } // end switch nroots
        } else { // not flag_acc
                switch (envs->nrys_roots) {
                        case 1:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] += g[ix] * g[iy] * g[iz];
                                }
                                break;
                        case 2:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] +=g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1];
                                }
                                break;
                        case 3:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] +=g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2];
                                }
                                break;
                        case 4:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] +=g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3];
                                }
                                break;
                        case 5:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] +=g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4];
                                }
                                break;
                        case 6:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] +=g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4]
                                                + g[ix+5] * g[iy+5] * g[iz+5];
                                }
                                break;
                        case 7:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] +=g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4]
                                                + g[ix+5] * g[iy+5] * g[iz+5]
                                                + g[ix+6] * g[iy+6] * g[iz+6];
                                }
                                break;
                        case 8:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        gout[n] +=g[ix  ] * g[iy  ] * g[iz  ]
                                                + g[ix+1] * g[iy+1] * g[iz+1]
                                                + g[ix+2] * g[iy+2] * g[iz+2]
                                                + g[ix+3] * g[iy+3] * g[iz+3]
                                                + g[ix+4] * g[iy+4] * g[iz+4]
                                                + g[ix+5] * g[iy+5] * g[iz+5]
                                                + g[ix+6] * g[iy+6] * g[iz+6]
                                                + g[ix+7] * g[iy+7] * g[iz+7];
                                }
                                break;
                        default:
                                for (n = 0; n < nf; n++, idx+=3) {
                                        ix = idx[0];
                                        iy = idx[1];
                                        iz = idx[2];
                                        s = 0;
                                        for (i = 0; i < envs->nrys_roots; i++) {
                                                s += g[ix+i] * g[iy+i] * g[iz+i];
                                        }
                                        gout[n] += s;
                                }
                                break;
                } // end switch nroots
        }
}

FINT int2e_sph(double *out, FINT *dims, FINT *shls, FINT *atm, FINT natm,
              FINT *bas, FINT nbas, double *env, CINTOpt *opt, double *cache)
{
        FINT ng[] = {0, 0, 0, 0, 0, 1, 1, 1};
        CINTEnvVars envs;
        CINTinit_int2e_EnvVars(&envs, ng, shls, atm, natm, bas, nbas, env);
        envs.f_gout = &CINTgout2e;
        return CINT2e_spheric_drv(out, dims, &envs, opt, cache);
}
void int2e_optimizer(CINTOpt **opt, FINT *atm, FINT natm,
                     FINT *bas, FINT nbas, double *env)
{
        FINT ng[] = {0, 0, 0, 0, 0, 1, 1, 1};
        CINTall_2e_optimizer(opt, ng, atm, natm, bas, nbas, env);
}

FINT int2e_cart(double *out, FINT *dims, FINT *shls, FINT *atm, FINT natm,
               FINT *bas, FINT nbas, double *env, CINTOpt *opt, double *cache)
{
        FINT ng[] = {0, 0, 0, 0, 0, 1, 1, 1};
        CINTEnvVars envs;
        CINTinit_int2e_EnvVars(&envs, ng, shls, atm, natm, bas, nbas, env);
        envs.f_gout = &CINTgout2e;
        return CINT2e_cart_drv(out, dims, &envs, opt, cache);
}

/*
 * spinor <ki|jl> = (ij|kl); i,j\in electron 1; k,l\in electron 2
 */
FINT int2e_spinor(double complex *out, FINT *dims, FINT *shls, FINT *atm, FINT natm,
                 FINT *bas, FINT nbas, double *env, CINTOpt *opt, double *cache)
{
        FINT ng[] = {0, 0, 0, 0, 0, 1, 1, 1};
        CINTEnvVars envs;
        CINTinit_int2e_EnvVars(&envs, ng, shls, atm, natm, bas, nbas, env);
        envs.f_gout = &CINTgout2e;
        return CINT2e_spinor_drv(out, dims, &envs, opt, cache,
                                 &c2s_sf_2e1, &c2s_sf_2e2);
}


ALL_CINT(int2e)
ALL_CINT_FORTRAN_(int2e)

