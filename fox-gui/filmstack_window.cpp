#include "filmstack_window.h"
#include "recipe_window.h"
#include "fit-params.h"
#include "dispers-library.h"
#include "dispers_ui_utils.h"
#include "dispers_win.h"
#include "regress_pro.h"
#include "dispers_save_dialog.h"

// Map
FXDEFMAP(filmstack_window) filmstack_window_map[]= {
    FXMAPFUNCS(SEL_LEFTBUTTONPRESS, filmstack_window::ID_FILM_MENU, filmstack_window::ID_FILM_MENU_LAST, filmstack_window::on_cmd_film_menu),
    FXMAPFUNCS(SEL_CHANGED, filmstack_window::ID_FILM_NAME, filmstack_window::ID_FILM_NAME_LAST, filmstack_window::on_change_name),
    FXMAPFUNCS(SEL_CHANGED, filmstack_window::ID_FILM_THICKNESS, filmstack_window::ID_FILM_THICKNESS_LAST, filmstack_window::on_change_thickness),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_INSERT_LAYER, filmstack_window::on_cmd_insert_layer),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_DELETE_LAYER, filmstack_window::on_cmd_delete_layer),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_REPLACE_LAYER, filmstack_window::on_cmd_replace_layer),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_EDIT_LAYER, filmstack_window::on_cmd_edit_layer),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_SAVE_USERLIB, filmstack_window::on_cmd_save_userlib),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_DISP_SAVEFILE, filmstack_window::on_cmd_disp_save_tofile),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_DELETE, filmstack_window::onCmdHide),
    FXMAPFUNC(SEL_COMMAND, filmstack_window::ID_PLOT_DISP, filmstack_window::on_cmd_plot_dispersion),
};

FXIMPLEMENT(filmstack_window,FXPacker,filmstack_window_map,ARRAYNUMBER(filmstack_window_map));

filmstack_window::filmstack_window(stack_t *s, FXComposite *p, FXuint opts, FXint x, FXint y, FXint w, FXint h, FXint pl, FXint pr, FXint pt, FXint pb, FXint hs, FXint vs)
    : FXPacker(p, opts, x, y, w, h, pl, pr, pt, pb, hs, vs),
    recipe_target(NULL), recipe_sel_change(0), recipe_sel_shift(0), stack(s)
{
    stack_window = setup_stack_window(this);

    popupmenu = new FXMenuPane(this);
    new FXMenuCommand(popupmenu,"Delete Layer", NULL, this, ID_DELETE_LAYER);
    new FXMenuCommand(popupmenu,"Select New Layer", NULL, this, ID_REPLACE_LAYER);
    new FXMenuCommand(popupmenu,"Insert Layer Above", NULL, this, ID_INSERT_LAYER);
    new FXMenuCommand(popupmenu,"Edit Dispersion", NULL, this, ID_EDIT_LAYER);
    new FXMenuSeparator(popupmenu);
    new FXMenuCommand(popupmenu,"Save to User Library", NULL, this, ID_SAVE_USERLIB);
    new FXMenuCommand(popupmenu,"Save to File", NULL, this, ID_DISP_SAVEFILE);
    new FXMenuCommand(popupmenu,"Plot\t\tPlot dispersion", NULL, this, ID_PLOT_DISP);
}

filmstack_window::~filmstack_window()
{
    delete popupmenu;
}

void
filmstack_window::set_target_stack_changes(FXObject *rcp, FXuint sel_change, FXuint sel_shift)
{
    recipe_target = rcp;
    recipe_sel_change = sel_change;
    recipe_sel_shift = sel_shift;
}

FXWindow *
filmstack_window::setup_stack_window(FXComposite *cont)
{
    regress_pro *app = regressProApp();

    FXVerticalFrame *vf = new FXVerticalFrame(cont, LAYOUT_FILL_X|LAYOUT_FILL_Y);
    matrix = new FXMatrix(vf, 3, LAYOUT_FILL_X|LAYOUT_BOTTOM|MATRIX_BY_COLUMNS);

    new FXVerticalFrame(matrix);
    FXLabel *lab1 = new FXLabel(matrix, "film material");
    lab1->setFont(&app->small_font);
    lab1->setTextColor(app->blue_highlight);
    FXLabel *lab2 = new FXLabel(matrix, "thickness");
    lab2->setFont(&app->small_font);
    lab2->setTextColor(app->blue_highlight);

    FXString nstr;
    for (int i = 0; i < stack->nb; i++) {
        nstr.format("%d", i);
        new FXButton(matrix, nstr, NULL, this, ID_FILM_MENU + i, FRAME_SUNKEN);
        FXTextField *filmtf = new FXTextField(matrix, 16, this, ID_FILM_NAME + i, FRAME_SUNKEN|LAYOUT_FILL_COLUMN);
        filmtf->setText(CSTR(stack->disp[i]->name));
        if (i > 0 && i < stack->nb - 1) {
            FXTextField *thktf = new FXTextField(matrix, 6, this, ID_FILM_THICKNESS + i, FRAME_SUNKEN|TEXTFIELD_REAL);
            nstr.format("%.4g", stack->thickness[i - 1]);
            thktf->setText(nstr);
        } else {
            new FXVerticalFrame(matrix);
        }
    }
    return vf;
}

void filmstack_window::create()
{
    FXPacker::create();
    popupmenu->create();
    getParent()->resize(getDefaultWidth(), getDefaultHeight());
}

long
filmstack_window::on_cmd_film_menu(FXObject*sender, FXSelector sel, void *ptr)
{
    FXEvent *event = (FXEvent *) ptr;
    current_layer = FXSELID(sel) - ID_FILM_MENU;
    if(!event->moved){
        popupmenu->popup(NULL, event->root_x, event->root_y);
        getApp()->runModalWhileShown(popupmenu);
    }
    return 1;
}

void
filmstack_window::rebuild_stack_window(bool notify)
{
    delete stack_window;
    stack_window = setup_stack_window(this);
    stack_window->create();
    getParent()->resize(getDefaultWidth(), getDefaultHeight());
    if (notify) {
        notify_stack_change();
    }
}

void
filmstack_window::notify_stack_change()
{
    if (recipe_target) {
        recipe_target->handle(this, recipe_sel_change, NULL);
    }
}

void
filmstack_window::notify_stack_shift(shift_info *info)
{
    if (recipe_target) {
        recipe_target->handle(this, recipe_sel_shift, info);
    }
}

long
filmstack_window::on_cmd_insert_layer(FXObject*,FXSelector,void*)
{
    disp_t *d = ui_choose_dispersion(this);
    if (!d) return 1;
    stack_insert_layer(stack, current_layer, d, 0.0);
    shift_info info = {short(SHIFT_INSERT_LAYER), short(current_layer)};
    notify_stack_shift(&info);
    rebuild_stack_window();
    return 1;
}

long
filmstack_window::on_cmd_replace_layer(FXObject*,FXSelector,void*)
{
    disp_t *d = ui_choose_dispersion(this);
    if (!d) return 1;
    disp_free(stack->disp[current_layer]);
    stack->disp[current_layer] = d;
    rebuild_stack_window();
    return 1;
}

long
filmstack_window::on_cmd_delete_layer(FXObject*, FXSelector, void*)
{
    if (stack->nb <= 2) return 0;
    stack_delete_layer(stack, current_layer);
    shift_info info = {short(SHIFT_DELETE_LAYER), short(current_layer)};
    notify_stack_shift(&info);
    rebuild_stack_window();
    return 1;
}

long
filmstack_window::on_cmd_edit_layer(FXObject*, FXSelector, void*)
{
    disp_t *current_disp = stack->disp[current_layer];
    disp_t *new_disp = ui_edit_dispersion(this, current_disp);
    if (new_disp) {
        stack->disp[current_layer] = new_disp;
        rebuild_stack_window();
        return 1;
    }
    return 1;
}

long
filmstack_window::on_cmd_save_userlib(FXObject*, FXSelector, void*)
{
    disp_t *d = disp_copy(stack->disp[current_layer]);
    disp_list_add(user_lib, d, NULL);
    return 1;
}

long
filmstack_window::on_change_name(FXObject*, FXSelector sel, void *data)
{
    int index = FXSELID(sel) - ID_FILM_NAME;
    str_copy_c(stack->disp[index]->name, (FXchar *)data);
    return 1;
}

long
filmstack_window::on_change_thickness(FXObject*, FXSelector sel, void *data)
{
    int index = FXSELID(sel) - ID_FILM_THICKNESS;
    double x = strtod((FXchar *)data, NULL);
    stack->thickness[index - 1] = x;
    return 1;
}

void
filmstack_window::bind_new_filmstack(stack_t *s, bool notify) {
    stack = s;
    rebuild_stack_window(notify);
}

void
filmstack_window::update_values()
{
    FXString nstr;
    for (int i = 1; i < stack->nb - 1; i++) {
        FXTextField *thktf = (FXTextField *) matrix->childAtRowCol(i + 1, 2);
        nstr.format("%.4g", stack->thickness[i - 1]);
        thktf->setText(nstr);
    }
}

long
filmstack_window::on_cmd_plot_dispersion(FXObject *, FXSelector, void *)
{
    dispers_win dwin(this, stack->disp[current_layer]);
    dwin.execute();
    return 1;
}

long
filmstack_window::on_cmd_disp_save_tofile(FXObject *, FXSelector, void *)
{
    disp_t *d = stack->disp[current_layer];
    dispers_save_dialog open(d, this, "Save Dispersion As");
    while (open.execute()) {
        if (open.save_dispersion() == 0) {
            break;
        }
    }
    return 1;
}
