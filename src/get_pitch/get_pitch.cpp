/// @file

#include <iostream>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <cmath>
#include <algorithm>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

#define FRAME_LEN   0.030 /* 30 ms. */
#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator 

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -m, --umaxnorm FLOAT  Voiced threshold for lag-power ratio [default: 0.5]
    -h, --help  Show this screen
    --version   Show the version of the project

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0 
)";

int main(int argc, const char *argv[]) {
	/// \TODO  DONE?
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();
  float umaxnorm = std::stof(args["--umaxnorm"].asString());

  // Read input sound file
  unsigned int rate;
  vector<float> x;
  if (readwav_mono(input_wav, rate, x) != 0) {
    cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
    return -2;
  }

  int n_len = rate * FRAME_LEN;
  int n_shift = rate * FRAME_SHIFT;

  // Define analyzer
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::RECT, 50, 500, umaxnorm); // We send 50Hz and 500Hz as the pitch range (we override the constants 20Hz-10000Hz)

  /// \TODO
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  
  /// Preprocess the input signal: Center clipping + Low-pass simple
  vector<float> x_processed = x;

  // Center clipping (mejora voiced/unvoiced en ruido) --> El ruido de fondo (ruido blanco, respiración) tiene amplitud pequeña → se elimina
  float clip_level = 0.2f;  // 20% del rango dinámico
  for(float& sample : x_processed) {
      sample = (fabsf(sample) > clip_level) ? sample : 0.0f;
  }

  // Low-pass filter simple (solo fundamentals <1kHz) --> Promedia cada muestra con la anterior (α=0.95), actuando como filtro pasa-bajos ~1kHz.
  int cutoff_samples = rate / 1000;  // ~1kHz
  for(size_t i = cutoff_samples; i < x_processed.size(); ++i) {
      x_processed[i] = 0.95f * x_processed[i-1] + 0.05f * x_processed[i];
  }

  // Usar x_processed en lugar de x original
   vector <float>::iterator iX = x_processed.begin();



  // Iterate for each frame and save values in f0 vector
  vector<float> f0;
  for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
    float f = analyzer(iX, iX + n_len);
    f0.push_back(f);
  }

  /// \TODO
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.

  // Write f0 contour into the output file
  ofstream os(output_txt);
  if (!os.good()) {
    cerr << "Error reading output file " << output_txt << " (" << strerror(errno) << ")\n";
    return -3;
  }

  os << 0 << '\n'; //pitch at t=0
  for (iX = f0.begin(); iX != f0.end(); ++iX) 
    os << *iX << '\n';
  os << 0 << '\n';//pitch at t=Dur

  return 0;
}
