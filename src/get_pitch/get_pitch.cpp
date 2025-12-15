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
    --uminPot=FLOAT Upper threshold for power in unvoiced decision [default: -35.8]
    --umaxnorm-hi=FLOAT  Lower voiced threshold for lag-power ratio [default: 0.4]
    --umaxnorm-lo=FLOAT  Upper unvoiced threshold for lag-power ratio [default: 0.3]
    --ur1norm=FLOAT Lower threshold for r1norm when found in gray area, in voiced decision [default: 0.96]
    --clip-level=FLOAT Threshold for center clipping [default: 0.015]
    --med-size=INT Size of the median filter windown [default: 3]
    --harm-ratio=FLOAT Level comparison with lag*2 [default: 0.96]
    -h, --help  Show this screen
    --version   Show the version of the project

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0 
)";

int main(int argc, const char *argv[]) {
	/// \TODO
	///  Modify the program syntax and the call to **docopt()** in order to
	///  add options and arguments to the program.
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

	std::string input_wav = args["<input-wav>"].asString();
	std::string output_txt = args["<output-txt>"].asString();

  // Default values are not working (it runs into an error if no option is inputted), so we will add the defaults as standard fallback when an option is empty
  auto get_float_or = [&](const std::string& key, float def) -> float {
      return !args[key] ? def : std::stof(args[key].asString());
  };

    float uminPot = get_float_or("--uminPot", -35.8);  // Example default; adjust as needed
    float umaxnorm_hi = get_float_or("--umaxnorm-hi", 0.4f);
    float umaxnorm_lo = get_float_or("--umaxnorm-lo", 0.3f);
    float ur1norm = get_float_or("--ur1norm", 0.96f);
    float clip_level = get_float_or("--clip-level", 0.015f);
    unsigned int med_size = static_cast<unsigned int>(get_float_or("--med-size", 3));  // Cast since med_size is unsigned int
    float harm_ratio = get_float_or("--harm-ratio", 0.96f);

  /// \DONE Options have been added to choose all thresholds, clip level and median filter window size

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
  PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::RECT, 50, 350, uminPot, umaxnorm_hi, umaxnorm_lo, ur1norm, harm_ratio, clip_level); // We send 50Hz and 350Hz as the pitch range (we override the constants 20Hz-10000Hz)

  /// \TODO
  /// Preprocess the input signal in order to ease pitch estimation. For instance,
  /// central-clipping or low pass filtering may be used.
  vector<float> x_norm = x;

  // Frame normalization
  float max = *std::max_element(x_norm.begin(), x_norm.end());
  for (int i = 0; i < (int)x_norm.size(); i++)
    x_norm[i] /= max;

  /// \DONE The input signal has been normalized, and central-clipping has been added (the latter is included in pitch_analyzer.cpp)
  
  vector<float>::iterator iX;
  vector<float> f0;

  for (iX = x_norm.begin(); iX + n_len < x_norm.end(); iX = iX + n_shift) {
      float f = analyzer(iX, iX + n_len);
      f0.push_back(f);
  }

  /// \TODO
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
  /// \DONE A median filter has been added, with custom window length based on the input argument --med-size

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
