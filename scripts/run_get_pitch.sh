#!/bin/bash

UMINPOT=${1:--8} # First input argument - Upper threshold for power in unvoiced decision
UMAXNORM_HI=${2:-0.4} # Second input argument - Lower voiced threshold for lag-power ratio
UMAXNORM_LO=${3:-0.3} # Third input argument - Upper unvoiced threshold for lag-power ratio
UR1NORM=${4:-0.96} # Fourth input argument - Lower threshold for r1norm when found in gray area, in voiced decision
CLIP_LEVEL=${5:-0.015}
MED_SIZE=${6:-3}
HARM_RATIO=${7:-0.96}

# EXAMPLE USE: get_pitch --uminPot -10 --umaxnorm-hi 0.29 --umaxnorm-lo 0.29 --ur1norm 0.9 prueba.wav prueba.f0

# Establecemos que el código de retorno de un pipeline sea el del último programa con código de retorno
# distinto de cero, o cero si todos devuelven cero.
set -o pipefail

# Put here the program (maybe with path)
GETF0="get_pitch --uminPot $UMINPOT --umaxnorm-hi $UMAXNORM_HI --umaxnorm-lo $UMAXNORM_LO --ur1norm $UR1NORM --clip-level $CLIP_LEVEL --med-size $MED_SIZE --harm-ratio $HARM_RATIO"

for fwav in pitch_db/train/*.wav; do
    ff0=${fwav/.wav/.f0}
    echo "$GETF0 $fwav $ff0 ----"
	$GETF0 $fwav $ff0 > /dev/null || { echo -e "\nError in $GETF0 $fwav $ff0" && exit 1; }
done

pitch_evaluate pitch_db/train/*.f0ref

exit 0
