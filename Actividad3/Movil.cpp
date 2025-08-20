#include "Movil.h"

Movil::Movil(string n, int x_inicial, int y_inicial) {
    nombre = n;
    x = x_inicial;
    y = y_inicial;
}

void Movil::mover(int dx, int dy) {
    x += dx;
    y += dy;
}

void Movil::reportar_estado() const {
    cout << "Movil: " << nombre << " | Posición: (" << x << ", " << y << ")" << endl;
}
