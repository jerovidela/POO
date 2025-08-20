#ifndef MOVIL_H
#define MOVIL_H

#include <iostream>
#include <string>
using namespace std;

class Movil {
private:
    int x;
    int y;
    string nombre;

public:
    Movil(string n, int x_inicial, int y_inicial);
    void mover(int dx, int dy);
    void reportar_estado() const;
};

#endif