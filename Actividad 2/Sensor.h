#pragma once
#include <string>

class Sensor {
private:
    std::string nombre;
    std::string unidad;
    double lectura;

public:
    Sensor(const std::string& nombre, const std::string& unidad, double lectura);
    void mostrarDatos();
};
