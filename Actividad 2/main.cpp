#include <iostream>
#include "Sensor.h"
#include "Alarma.h"

int main() {
    Sensor temp("Sensor de Temperatura", "°C", 30.5);
    temp.mostrarDatos();

    Alarma alarma("Alarma de Temperatura");
    alarma.disparar();
    alarma.mostrarEstado();

    alarma.detener();
    alarma.mostrarEstado();

    return 0;
}
