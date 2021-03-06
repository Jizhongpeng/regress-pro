
/* plot_units.cpp
 *
 * Copyright (C) 2009, 2010 Francesco Abbate
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

#include <stdio.h>
#include <math.h>

#include "utils.h"
#include "plot_units.h"

void plot_units::init(double yinf, double ysup, double spacefact)
{
    double del;

    if(ysup == yinf) {
        ysup = yinf + 1.0;
    }

    del = (ysup - yinf) / spacefact;

    order = (int) floor(log10(del));

    double expf = pow(10, order);
    double delr = del / expf;

    if(5 <= delr) {
        m_major = 5;
    } else if(2 <= delr) {
        m_major = 2;
    } else {
        m_major = 1;
    }

    m_inf = (int) floor(yinf / (m_major * expf) + 1e-5);
    m_sup = (int) ceil(ysup / (m_major * expf) - 1e-5);

    nb_decimals = (order < 0 ? -order : 0);

    dmajor = m_major * expf;
}

void plot_units::mark_label(char *lab, unsigned size, int mark) const
{
    bool minus = (m_inf < 0);
    int asup = (minus ? -m_inf : m_sup);
    char fmt[16];

    if(size < 16) {
        return;
    }

    if(nb_decimals == 0) {
        snprintf(lab, size, "%d", int(mark * dmajor));
        lab[size-1] = '\0';
    } else {
        int dec = (nb_decimals < 10 ? nb_decimals : 9);
        int base = (int) floor(asup * dmajor);
        int space = dec + (base > 0 ? (int)log10(base): 0) + 1 \
                    + (minus ? 1 : 0) + 1;
        snprintf(fmt, 16, "%%%i.%if", space, dec);
        fmt[15] = '\0';
        snprintf(lab, size, fmt, mark * dmajor);
        lab[size-1] = '\0';
    }
}

double plot_units::mark_scale(double x)
{
    double xinf = m_inf * dmajor, xsup = m_sup * dmajor;
    return (x - xinf) / (xsup - xinf);
}
