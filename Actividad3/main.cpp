#include "Movil.h"

int main() {
    Movil m1("Dron1", 0, 0);

    m1.reportar_estado();
    m1.mover(3, 2);
    m1.reportar_estado();

    m1.mover(-8, 1);
    m1.reportar_estado();

    return 0;
}
