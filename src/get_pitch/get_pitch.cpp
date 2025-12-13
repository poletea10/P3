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
    --uminPot FLOAT Upper threshold for power in unvoiced decision [default: -19.5]
    --umaxnorm-hi FLOAT  Lower voiced threshold for lag-power ratio [default: 0.57]
    --umaxnorm-lo FLOAT  Upper unvoiced threshold for lag-power ratio [default: 0.19]
    --ur1norm FLOAT Lower threshold for r1norm when found in gray area, in voiced decision [default: 0.86]
    --clip-level FLOAT Threshold for center clipping [default: 0.007]
    --med-size INT Size of the median filter windown [default: 3]
    --harm-ratio FLOAT Level comparison with lag*2 [default: 0.96]
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
    float uminPot = std::stof(args["--uminPot"].asString());
    float umaxnorm_hi = std::stof(args["--umaxnorm-hi"].asString());
    float umaxnorm_lo = std::stof(args["--umaxnorm-lo"].asString());
    float ur1norm = std::stof(args["--ur1norm"].asString());
    float clip_level = std::stof(args["--clip-level"].asString());
    unsigned int med_size = std::stof(args["--med-size"].asString());
    float harm_ratio = std::stof(args["--harm-ratio"].asString());

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
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::RECT, 50, 350, uminPot, umaxnorm_hi, umaxnorm_lo, ur1norm, harm_ratio); // We send 50Hz and 350Hz as the pitch range (we override the constants 20Hz-10000Hz)

  /// \TODO --> DONE?
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  vector<float> x_processed = x;

  // Frame normalization
  float max = *std::max_element(x_processed.begin(), x_processed.end());
  for (int i = 0; i < (int)x_processed.size(); i++)
    x_processed[i] /= max;
  
  // Center clipping --> Noise has low amplitude -> we eliminate it -> harmonics are intensified

  // Without offset
  for (float &sample : x_processed) {
      if (fabsf(sample) < clip_level) {
          sample = 0.0f;
      }
  }

  // With offset (worse performance, from what we've tried)
//   for (float &sample : x_processed) {
//       if (fabsf(sample) < clip_level) {
//           sample = 0.0f;
//       }else if (sample < 0){
//         sample = sample + clip_level;
//       }else{
//         sample = sample - clip_level;
//       }
//   }

  vector<float>::iterator iX;
  vector<float> f0;

  for (iX = x_processed.begin(); iX + n_len < x_processed.end(); iX = iX + n_shift) {
      float f = analyzer(iX, iX + n_len);
      f0.push_back(f);
  }


  /// \TODO --> DONE?
  /// Postprocess the estimation in order to supress errors. For instance, a median filter
  /// or time-warping may be used.

  /// Postprocess: Median filter
  if (f0.size() >= med_size) {
      vector<float> f0_med(f0.size());
      
      // Primer y último frame sin tocar
      f0_med[0] = f0[0];
      f0_med.back() = f0.back();
      
      // Median for center frames
      size_t half = (med_size - 1) / 2;
      for(size_t i = half; i + half < f0.size() - 1; ++i) {
        vector<float> window;
        window.reserve(med_size);

        for (size_t j = i - half; j <= i + half; ++j)
            window.push_back(f0[j]);

        sort(window.begin(), window.end());
        f0_med[i] = window[half];   // median
      }
      
      f0 = f0_med;
  }

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
