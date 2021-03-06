#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dispers.h"
#include "disp-sample-table.h"
#include "error-messages.h"

static void     disp_sample_table_free(disp_t *d);
static disp_t * disp_sample_table_copy(const disp_t *d);

static cmpl disp_sample_table_n_value(const disp_t *disp, double lam);
static int disp_sample_table_write(writer_t *w, const disp_t *_d);
static int disp_sample_table_read(lexer_t *l, disp_t *d);

struct disp_class disp_sample_table_class = {
    .disp_class_id       = DISP_SAMPLE_TABLE,
    .full_name           = "Tabular",
    .short_name          = "table",

    .free                = disp_sample_table_free,
    .copy                = disp_sample_table_copy,

    .n_value             = disp_sample_table_n_value,
    .fp_number           = disp_base_fp_number,
    .n_value_deriv       = NULL,
    .apply_param         = NULL,
    .get_param_value     = NULL,

    .encode_param        = NULL,
    .write               = disp_sample_table_write,
    .read                = disp_sample_table_read,
};

static void
init_interp(struct disp_sample_table *dt)
{
    dt->interp_n = gsl_interp_alloc(gsl_interp_cspline, dt->len);
    dt->interp_k = gsl_interp_alloc(gsl_interp_cspline, dt->len);
    dt->accel = gsl_interp_accel_alloc();
}

static void
init(struct disp_sample_table *dt, int len)
{
    dt->len = len;
    dt->table = rc_matrix_alloc(3, len);
    init_interp(dt);
}

static double get_wavelength(const struct disp_sample_table *dt, int index)
{
    return dt->table->view.matrix.data[index];
}

static double get_n(const struct disp_sample_table *dt, int index)
{
    return dt->table->view.matrix.data[dt->table->view.matrix.tda + index];
}

static double get_k(const struct disp_sample_table *dt, int index)
{
    return dt->table->view.matrix.data[2*dt->table->view.matrix.tda + index];
}

static double *wavelength_array(struct disp_sample_table *dt)
{
    return dt->table->view.matrix.data;
}

static double *n_array(struct disp_sample_table *dt)
{
    return dt->table->view.matrix.data + dt->table->view.matrix.tda;
}

static double *k_array(struct disp_sample_table *dt)
{
    return dt->table->view.matrix.data + 2 * dt->table->view.matrix.tda;
}

static const double *wavelength_const_array(const struct disp_sample_table *dt)
{
    return dt->table->view.matrix.data;
}

static const double *n_const_array(const struct disp_sample_table *dt)
{
    return dt->table->view.matrix.data + dt->table->view.matrix.tda;
}

static const double *k_const_array(const struct disp_sample_table *dt)
{
    return dt->table->view.matrix.data + 2 * dt->table->view.matrix.tda;
}

void disp_sample_table_get_sample(const struct disp_sample_table *dt, int index, double *w, double *n, double *k)
{
    *w = get_wavelength(dt, index);
    *n = get_n(dt, index);
    *k = get_k(dt, index);
}

static void
prepare_interp(struct disp_sample_table *dt)
{
    gsl_interp_init(dt->interp_n, wavelength_array(dt), n_array(dt), dt->len);
    gsl_interp_init(dt->interp_k, wavelength_array(dt), k_array(dt), dt->len);
}

static void
clear(struct disp_sample_table *dt)
{
    dt->len = 0;
}

void
disp_sample_table_free(disp_t *d)
{
    struct disp_sample_table *dt = &d->disp.sample_table;
    if (dt->len > 0) {
        rc_matrix_unref(dt->table);
        gsl_interp_accel_free(dt->accel);
        gsl_interp_free(dt->interp_n);
        gsl_interp_free(dt->interp_k);
    }
    disp_base_free(d);
}

disp_t *
disp_sample_table_copy(const disp_t *src)
{
    disp_t *res = disp_base_copy(src);
    struct disp_sample_table *dt = &res->disp.sample_table;
    if(dt->len > 0) {
        rc_matrix_ref(dt->table);
        init_interp(dt);
        prepare_interp(dt);
    }
    return res;
}

cmpl
disp_sample_table_n_value(const disp_t *disp, double lam)
{
    const struct disp_sample_table *dt = & disp->disp.sample_table;
    if (lam <= get_wavelength(dt, 0)) {
        return get_n(dt, 0) - get_k(dt, 0) * I;
    } else if (lam >= get_wavelength(dt, dt->len - 1)) {
        return get_n(dt, dt->len - 1) - get_k(dt, dt->len - 1) * I;
    }
    double nx = gsl_interp_eval(dt->interp_n, wavelength_const_array(dt), n_const_array(dt), lam, dt->accel);
    double kx = gsl_interp_eval(dt->interp_k, wavelength_const_array(dt), k_const_array(dt), lam, dt->accel);
    return nx - kx * I;
}

enum {
    WL_UNIT_DEFAULT = 0, /* Nanometers. */
    WL_UNIT_CONVERT_EV = 1,
    WL_UNIT_CONVERT_ANGSTROMS = 1 << 1,
};

disp_t *
disp_sample_table_new_from_mat_file(const char * filename, str_ptr *error_msg)
{
    FILE * f;
    str_t row;
    disp_t *disp = NULL;
    enum disp_type dtype;
    unsigned int wl_unit_conv = 0;
    int provide_diel_k = 0;

    f = fopen(filename, "r");

    if(f == NULL) {
        *error_msg = new_error_message(LOADING_FILE_ERROR, "File \"%s\" does not exists or cannot be opened", filename);
        return NULL;
    }

    str_t name;
    str_init(name, 64);
    str_init(row, 64);
    str_getline(name, f);
    str_getline(row, f);

    if(strncasecmp(CSTR(row), "CAUCHY", 6) == 0) {
        dtype = DISP_CAUCHY;
    } else if(strncasecmp(CSTR(row), "ev", 2) == 0) {
        wl_unit_conv |= WL_UNIT_CONVERT_EV;
    } else if(strncasecmp(CSTR(row), "ANGSTROMS", 9) == 0) {
        wl_unit_conv |= WL_UNIT_CONVERT_ANGSTROMS;
    } else if(strncasecmp(CSTR(row), "nm", 2) != 0) {
        *error_msg = new_error_message(LOADING_FILE_ERROR, "Invalide MAT file: \"%s\"", filename);
        goto close_exit;
    }

    str_getline(row, f);
    if(strncasecmp(CSTR(row), "nk", 2) == 0) {
        dtype = DISP_SAMPLE_TABLE;
    } else if(strncasecmp(CSTR(row), "e1e2", 4) == 0) {
        dtype = DISP_SAMPLE_TABLE;
        provide_diel_k = 1;
    } else {
        *error_msg = new_error_message(LOADING_FILE_ERROR, "Invalide MAT file: \"%s\"", filename);
        goto close_exit;
    }

    switch(dtype) {
    case DISP_SAMPLE_TABLE: {
        struct disp_sample_table *dt;
        long start_pos = ftell(f);
        int j, lines;

        disp = disp_new(DISP_SAMPLE_TABLE);
        str_copy(disp->name, name);
        dt = & disp->disp.sample_table;
        clear(dt);

        start_pos = ftell(f);

        for(lines = 0; ;) {
            float xd[3];
            int read_status = fscanf(f, "%f %f %f\n", xd, xd+1, xd+2);
            if(read_status == 3) {
                lines ++;
            }
            if(read_status == EOF) {
                break;
            }
        }

        if(lines < 2) {
            disp_free(disp);
            disp = NULL;
            break;
        }

        fseek(f, start_pos, SEEK_SET);

        init(dt, lines);

        double *wptr = wavelength_array(dt);
        double *nptr = n_array(dt);
        double *kptr = k_array(dt);
        for(j = 0; j < lines; j++, wptr++, nptr++, kptr++) {
            int read_status;
            do {
                read_status = fscanf(f, "%lf %lf %lf\n", wptr, nptr, kptr);
            } while(read_status < 3 && read_status != EOF);

            if(wl_unit_conv & WL_UNIT_CONVERT_EV) {
                *wptr = 1239.8 / *wptr;
            } else if(wl_unit_conv & WL_UNIT_CONVERT_ANGSTROMS) {
                *wptr /= 10.0;
            }

            if(provide_diel_k) {
                double e1 = *nptr, e2 = *kptr;
                double ne = sqrt(e1*e1 + e2*e2);
                *nptr = sqrt((ne + e1) / 2.0);
                *kptr = sqrt((ne - e1) / 2.0);
            }

            if(read_status == EOF) {
                break;
            }
        }
        prepare_interp(dt);
        break;
    }
    case DISP_CAUCHY:
        *error_msg = new_error_message(LOADING_FILE_ERROR, "cauchy MAT format is unsupported");
        break;
#if 0
        cn = disp->disp.cauchy.n;
        ck = disp->disp.cauchy.k;
        fscanf(f, "%lf %lf %lf %*f %*f %*f\n", cn, cn+1, cn+2);
        cn[1] *= 1e3;
        cn[2] *= 1e6;
        ck[0] = ck[1] = ck[2] = 0.0;
        break;
#endif
    default:
        *error_msg = new_error_message(LOADING_FILE_ERROR, "corrupted material card");
        break;
    }

close_exit:
    fclose(f);
    str_free(name);
    str_free(row);
    return disp;
}

int
disp_sample_table_write(writer_t *w, const disp_t *_d)
{
    const struct disp_sample_table *d = &_d->disp.sample_table;
    writer_printf(w, "table \"%s\" %d", CSTR(_d->name), d->len);
    writer_newline_enter(w);
    rc_matrix_write(w, d->table, RC_MATRIX_TRANSPOSED);
    writer_newline_exit(w);
    return 0;
}

int
disp_sample_table_read(lexer_t *l, disp_t *d_gen)
{
    struct disp_sample_table *d = &d_gen->disp.sample_table;
    d->len = 0;
    int len;
    if (lexer_integer(l, &len)) return 1;
    d->table = rc_matrix_read(l, RC_MATRIX_TRANSPOSED);
    if (!d->table) return 1;
    d->len = len;
    init_interp(d);
    prepare_interp(d);
    return 0;
}
