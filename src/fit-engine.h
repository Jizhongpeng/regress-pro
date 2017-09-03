
/* fit-engine.h
 *
 * Copyright (C) 2005-2011 Francesco Abbate
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef FIT_ENGINE_H
#define FIT_ENGINE_H

#include "defs.h"
#include "elliss.h"
#include "spectra.h"
#include "stack.h"
#include "fit-params.h"
#include "fit-engine-common.h"
#include "writer.h"

#include <gsl/gsl_vector.h>
#include <gsl/gsl_multifit_nlin.h>

__BEGIN_DECLS

#define FIT_OPTIONS_ACQUISITION(n) ((n) & ((1 << 8) - 1))
#define FIT_OPTIONS_SUBSAMPLING(n) ((n) >> 8)

enum fit_engine_acq {
    FIT_KEEP_ACQUISITION  = 0,
    FIT_RESET_ACQUISITION = 1,
};

enum fit_engine_subsamplig {
    FIT_ENABLE_SUBSAMPLING  = 1 << 8,
};

struct spectrum_item {
    struct spectrum *spectrum;
    struct acquisition_parameters acquisition[1];
    int scope;
};

struct fit_engine {
    struct fit_config config[1];

    int spectra_number, spectra_capacity;    
    struct spectrum_item *spectra_list;
    int *spectra_groups; /* An array with the group number of each sample (stack). */

    int samples_number; /* Equal to the number of film stacks. */
    struct stack **stack_list;
    struct fit_parameters *parameters;

    gsl_multifit_function_fdf mffun;
    gsl_vector *results;
    struct stack_cache cache;
};

struct seeds;

extern struct fit_engine *fit_engine_new();

/* Create a clone of the film stack by copying the film stack. */
extern struct fit_engine *fit_engine_clone(struct fit_engine *fit);

/* Bind the fit_engine to the given film stack, config and fit parameters.
   It makes internally a copy of the stack and of the config.
   It holds only a reference to the fit parameters and this latter can be
   NULL if the parameters are given later. */
extern void fit_engine_bind(struct fit_engine *fit, const stack_t *stack, const struct fit_config *config, struct fit_parameters *parameters);

/* Bind the fit_engine to the provided film stack by taking the ownership.
   No copy of the film stack is made. */
extern void fit_engine_bind_stack(struct fit_engine *fit, stack_t *stack);

extern void fit_engine_free(struct fit_engine *fit);

extern void fit_engine_clear_spectra(struct fit_engine *f);
extern void fit_engine_add_spectrum(struct fit_engine *f, const struct spectrum *s, const int spectrum_scope);
extern int  fit_engine_prepare(struct fit_engine *f, const struct spectrum *s, const int fit_engine_flags);
extern int  fit_engine_prepare_final(struct fit_engine *f, const int fit_engine_flags);

extern void fit_engine_disable(struct fit_engine *f);

/* Return the stack owned by the fit_engine and gives it ownership to the
   caller function. */
extern stack_t *fit_engine_yield_stack(struct fit_engine *f);

extern int  check_fit_parameters(struct stack *stack, struct fit_parameters *fps, str_ptr *error_msg);

extern void fit_engine_commit_parameters(struct fit_engine *fit,
        const gsl_vector *x);

extern int fit_engine_apply_param(struct fit_engine *fit,
                                  const fit_param_t *fp, double val);

extern void fit_engine_apply_parameters(struct fit_engine *fit,
                                        const struct fit_parameters *fps,
                                        const gsl_vector *x);

extern void fit_engine_get_wavelength_limits(const struct fit_engine *fit, double *wavelength_start, double *wavelength_end);
extern void fit_engine_update_disp_info(struct fit_engine *fit);
extern void fit_engine_copy_disp_info(struct fit_engine *dst, const struct fit_engine *src);

extern void build_stack_cache(struct stack_cache *cache,
                              stack_t *stack,
                              struct spectrum *spectr,
                              int th_only_optimize, int require_acquisition_jacob);

extern void dispose_stack_cache(struct stack_cache *cache);

extern void fit_engine_get_cached_ns(struct fit_engine *fit, int j, cmpl ns[]);

extern void fit_engine_generate_spectrum(struct fit_engine *fit, struct spectrum *ref, int spectrum_no, struct spectrum *synth);

extern void fit_engine_print_fit_results(struct fit_engine *fit,
        str_t text, int tabular);

extern struct fit_parameters *
fit_engine_get_all_parameters(struct fit_engine *fit);

extern double
fit_engine_get_parameter_value(const struct fit_engine *fit,
                               const fit_param_t *fp);

extern double
fit_engine_estimate_param_grid_step(struct fit_engine *fit, const gsl_vector *x, const fit_param_t *fp, double delta);

extern double
fit_engine_get_seed_value(const struct fit_engine *fit, const fit_param_t *fp, const seed_t *s);

extern void
fit_config_set_default(struct fit_config *cfg);

extern void
fit_engine_set_acquisition(struct fit_engine *fit, struct acquisition_parameters *acquisition);

extern int fit_config_write(writer_t *w, const struct fit_config *config);
extern int fit_config_read(lexer_t *l, struct fit_config *config);

__END_DECLS

#endif
