#include "stack.h"

int Stack::write(Writer& w) {
    w << "stack" << int(m_layers.size()) + 2;
    w.newline_enter();

    for (const Layer& layer: m_layers) {
        w << layer.thickness;
    }
    w.newline();
    m_substrate->write(w);
    for (const Layer& layer: m_layers) {
        layer.dispersion->write(w);
    }
    m_environ->write(w);
    w.indent(-1);
    return 0;
}

std::unique_ptr<Stack> Stack::read(Lexer& lexer) {
    int n;
    lexer >> "stack" >> n;
    const int layers_number = n - 2;
    if (layers_number < 0 || layers_number > Stack::MAX_LAYERS_NUMBER) {
        throw std::invalid_argument("invalid number of layers");
    }
    std::unique_ptr<Stack> stack(new Stack(layers_number));
    for (Layer& layer : stack->m_layers) {
        lexer >> layer.thickness;
    }
    stack->m_substrate = Dispersion::read(lexer);
    for (Layer& layer : stack->m_layers) {
        layer.dispersion = Dispersion::read(lexer);
    }
    stack->m_environ = Dispersion::read(lexer);
    return stack;
}
