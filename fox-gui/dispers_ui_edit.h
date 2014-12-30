#ifndef DISPERS_UI_EDIT_H
#define DISPERS_UI_EDIT_H

#include <fx.h>

#include "dispers.h"
#include "fx_numeric_field.h"

class fx_disp_window : public FXVerticalFrame {
    FXDECLARE(fx_disp_window)

protected:
    fx_disp_window() {};
private:
    fx_disp_window(const fx_disp_window&);
    fx_disp_window &operator=(const fx_disp_window&);

public:
    fx_disp_window(disp_t *d, FXComposite* p, FXuint opts=0);
    ~fx_disp_window();

    virtual void create();
    virtual void setup_dialog() { }
    virtual void add_dispersion_element() {}
    virtual double *map_parameter(int index) { return 0; }

    long on_cmd_value(FXObject*, FXSelector, void *);
    long on_changed_name(FXObject*, FXSelector, void *);
    long on_disp_element_add(FXObject*, FXSelector, void *);

    enum {
        ID_NAME = FXVerticalFrame::ID_LAST,
        ID_PARAM_0,
        ID_PARAM_LAST = ID_PARAM_0 + 5 * 16,
        ID_DISP_ELEMENT_DELETE,
        ID_DISP_ELEMENT_ADD,
        ID_LAST
    };

protected:
    FXIcon *delete_icon, *add_icon;

    // This is just a reference. The class is not owner of this object.
    disp_t *disp;
};

class fx_disp_ho_window : public fx_disp_window {
public:
    fx_disp_ho_window(disp_t *d, FXComposite* p, FXuint opts=0): fx_disp_window(d, p, opts) { }
    virtual void setup_dialog();
    virtual void add_dispersion_element();
    virtual double *map_parameter(int index);
private:
    FXMatrix *matrix;
};

class fx_disp_cauchy_window : public fx_disp_window {
public:
    fx_disp_cauchy_window(disp_t *d, FXComposite* p, FXuint opts=0): fx_disp_window(d, p, opts) { }
    virtual void setup_dialog();
    virtual double *map_parameter(int index);
};

#endif