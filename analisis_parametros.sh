#! /usr/bin/bash

# Análisis cambio clipping
for clip in $(seq 0 0.001 0.025); do # 
    echo -ne "$clip\n" # Imprime los valores actuales
    scripts/run_get_pitch.sh -35.8 0.4 0.30 0.96 $clip | grep TOTAL # Ejecuta el Test para los valores actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
done

echo -ne "\n"

# Análisis cambio longitud filtro mediana
for med in $(seq 1 2 25); do # Para todos los posibles valores de a1 entre 8 y 8.5 (con pasos de 0.05)
    echo -ne "$med\n" # Imprime el valor de $a1, $a2 y $ti actual
    scripts/run_get_pitch.sh -35.8 0.40 0.30 0.96 0.015 $med | grep TOTAL # Ejecuta el Test para los valores $a1/$a2/$ti actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
done