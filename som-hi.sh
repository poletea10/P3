#! /usr/bin/bash

# Buscamos mejor combinación de umbrales para reconocer partes voiced/unvoiced
# for pot in $(seq -- -8 0.25 -7.5); do
#     for hi in $(seq 0.39 0.01 0.41); do
#         for lo in $(seq 0.28 0.01 0.31); do
#             for r1 in $(seq 0.94 0.02 0.98); do
#                 echo -ne "$pot $hi $lo $r1\t" # Imprime los valores actuales
#                 scripts/run_get_pitch.sh $pot $hi $lo $r1 | grep TOTAL # Ejecuta el Test para los valores actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
#             done
#         done
#     done
# done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor

# -8.00 0.40 0.30 0.96    ===>    TOTAL:  92.04 %

# Buscamos mejor clip level (antes lo teniamos fijado a 0.015)
# for clip in $(seq 0.014 0.001 0.025); do # 
#     echo -ne "$clip\t" # Imprime los valores actuales
#     scripts/run_get_pitch.sh -8 0.40 0.30 0.96 $clip | grep TOTAL # Ejecuta el Test para los valores actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
# done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor

# 0.015   ===>    TOTAL:  92.04 %

# Buscamos mejor tamaño de filtro de mediana (antes lo teniamos fijado a 3)
# for med in $(seq 1 2 19); do # Para todos los posibles valores de a1 entre 8 y 8.5 (con pasos de 0.05)
#     echo -ne "$med\t" # Imprime el valor de $a1, $a2 y $ti actual
#     scripts/run_get_pitch.sh -8 0.40 0.30 0.96 0.015 $med | grep TOTAL # Ejecuta el Test para los valores $a1/$a2/$ti actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
# done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor

# 3       ===>    TOTAL:  92.04 %

# Buscamos mejor detector de gross errors (miramos ratio respecto lag/2 para detectar si tenemos un armónico o no) (antes lo teniamos fijado a 0.98)
for harm in $(seq 0.85 0.01 0.99); do
  echo -ne "$harm \t" # Imprime los valores actuales
  scripts/run_get_pitch.sh -8 0.40 0.30 0.96 0.015 3 $harm | grep TOTAL # Ejecuta el Test para los valores actuales y imprime su Fscore total (solo imprimiendo la línea con la palabra "TOTAL")
done | sort -t: -k 2n # Ordena resultados por Fscores de mayor a menor

# 0.96    ===>    TOTAL:  92.04 %