#include <iostream>
#include "Sensor.h"

Sensor::Sensor(const std::string& nombre, const std::string& unidad, double lectura)
    : nombre(nombre), unidad(unidad), lectura(lectura) {}

void Sensor::mostrarDatos() {
    std::cout << "Nombre: " << nombre
              << " | Lectura: " << lectura << " " << unidad << std::endl;
}
