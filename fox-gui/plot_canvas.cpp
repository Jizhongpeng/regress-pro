
#include "plot_canvas.h"
#include "colors.h"

// Map
FXDEFMAP(plot_canvas) plot_canvas_map[]= {
    FXMAPFUNC(SEL_PAINT,  0, plot_canvas::on_cmd_paint),
    FXMAPFUNC(SEL_UPDATE, 0, plot_canvas::on_update),
};

// Object implementation
FXIMPLEMENT(plot_canvas,FXCanvas,plot_canvas_map,ARRAYNUMBER(plot_canvas_map));

void plot_canvas::prepare_image_buffer(int ww, int hh)
{
    delete m_img;
    m_img = new FXImage(getApp(), NULL, IMAGE_KEEP|IMAGE_OWNED|IMAGE_SHMI|IMAGE_SHMP, ww, hh);
    m_img->create();

    agg::int8u* buf = (agg::int8u*) m_img->getData();
    unsigned width = ww, height = hh;
    unsigned stride = - width * sizeof(FXColor);

    m_rbuf.attach(buf, width, height, stride);
    m_canvas = new canvas(m_rbuf, width, height, colors::white);
}

void plot_canvas::ensure_canvas_size(int ww, int hh)
{
    if(! m_img) {
        prepare_image_buffer(ww, hh);
    } else if(m_img->getWidth() != ww || m_img->getHeight() != hh) {
        prepare_image_buffer(ww, hh);
    }
}

void
plot_canvas::draw_plot(FXEvent* event)
{
    int ww = getWidth(), hh = getHeight();

    ensure_canvas_size(ww, hh);

    if(m_canvas) {
        m_canvas->clear();
        m_plots.draw(m_canvas, ww, hh);
        m_img->render();

        FXDCWindow *dc = (event ? new FXDCWindow(this, event) : new FXDCWindow(this));
        dc->drawImage(m_img, 0, 0);
        delete dc;
    }

    m_dirty_flag = false;
}

long
plot_canvas::on_update(FXObject *, FXSelector, void *)
{
    if(m_dirty_flag) {
        draw_plot(NULL);
        return 1;
    }
    return 0;
}

long
plot_canvas::on_cmd_paint(FXObject *, FXSelector, void *ptr)
{
    FXEvent* ev = (FXEvent*) ptr;
    draw_plot(dirty() ? NULL : ev);
    return 1;
}

static void
add_new_plot_raw(plot_canvas* canvas, vs_object* ref, vs_object* model,
                 const char *title)
{
    agg::rgba8 red(220,0,0);
    agg::rgba8 blue(0,0,220);

    plot_canvas::plot_type *p = new plot_canvas::plot_type();
    p->set_title(title);
    p->pad_mode(true);

    if(ref) {
        p->add(ref,   red, true);
    }
    if(model) {
        p->add(model, blue, true);
    }
    p->commit_pending_draw();

    canvas->add(p);
}

void add_new_simple_plot(plot_canvas* canvas, vs_object* v, const char *title)
{
    add_new_plot_raw(canvas, v, 0, title);
}

void add_new_plot(plot_canvas* canvas, vs_object* v1, vs_object* v2,
                  const char *title)
{
    add_new_plot_raw(canvas, v1, v2, title);
}

