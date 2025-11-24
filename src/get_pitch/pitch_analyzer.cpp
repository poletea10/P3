/// @file

#include <iostream>
#include <math.h>
#include "pitch_analyzer.h"

using namespace std;

/// Name space of UPC
namespace upc {
  void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {

    for (unsigned int l = 0; l < r.size(); ++l) {
  		/// \TODO Compute the autocorrelation r[l]
      r[l]=0.0F;
      for(unsigned int n=0; n<r.size()-l; n++){
          r[l] += x[n]*x[n + l];
      }
      /**   
       \DONE Autocorrelation implemented
       \f[
       r[l] = \sum_{n=0}^{N-l} x_i^*[n] x_i[n+l]
       \f]
       - Inicializamos la autocorrelacion a 0.
       - Sumamos la multiplicacion con la señal desplazada.
       */
    }

    if (r[0] == 0.0F) //to avoid log() and divide zero 
      r[0] = 1e-10; 
  }

  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;

    window.resize(frameLen);

    switch (win_type) {
    case HAMMING:
      /// \TODO Implement the Hamming window
      break;
    case RECT:
    default:
      window.assign(frameLen, 1); // Square window (1 coefficients for the whole frameLen)
    }
  }

  void PitchAnalyzer::set_f0_range(float min_F0, float max_F0) {
    npitch_min = (unsigned int) samplingFreq/max_F0;
    if (npitch_min < 2)
      npitch_min = 2;  // samplingFreq/2

    npitch_max = 1 + (unsigned int) samplingFreq/min_F0;

    //frameLen should include at least 2*T0
    if (npitch_max > frameLen/2)
      npitch_max = frameLen/2;
  }

  bool PitchAnalyzer::unvoiced(float pot, float r1norm, float rmaxnorm) const {
    /// \TODO Implement a rule to decide whether the sound is voiced or not.
    /// * You can use the standard features (pot, r1norm, rmaxnorm),
    ///   or compute and use other ones.
    return false; // Antes era true! Ahora estamos marcando todo como voiced
  }

  float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen)
      return -1.0F;

    //Window input frame
    for (unsigned int i=0; i<x.size(); ++i)
      x[i] *= window[i];

    // r will hold the autocorrelation values for lags from 0 to npitch_max-1 (max lag = lowest freq)
    vector<float> r(npitch_max);

    //Compute correlation
    autocorrelation(x, r);

    vector<float>::const_iterator iR = r.begin(), iRMax = iR; // We create two iterators that, at this instant, point to the first element of r

    /// \TODO 
	/// Find the lag of the maximum value of the autocorrelation away from the origin.<br>
	/// Choices to set the minimum value of the lag are:
	///    - The first negative value of the autocorrelation.
	///    - The lag corresponding to the maximum value of the pitch.
    ///	   .
	/// In either case, the lag should not exceed that of the minimum value of the pitch.

    iRMax = std::max_element(iR + npitch_min, iR + npitch_max); // Returns the iterator pointing at the max position between npitch_min (smallest lag) & npitch_max (biggest lag)

    unsigned int lag = iRMax - r.begin(); // Computes the index (lag) of the maximum in that range (position of max iterator - pointer at the beginning)
    // Now lag is the number of samples (tied to samplingFreq) corresponding to our estimated pitch period!

    float pot = 10 * log10(r[0]); // Power of current window -> Good for voicing decision, since unvoiced parts have low energy and noisy curve, and vice versa

    //You can print these (and other) features, look at them using wavesurfer
    //Based on that, implement a rule for unvoiced
    //change to #if 1 and compile
#if 0
    if (r[0] > 0.0F)
      cout << pot << '\t' << r[1]/r[0] << '\t' << r[lag]/r[0] << endl;
#endif
    
    if (unvoiced(pot, r[1]/r[0], r[lag]/r[0])) // If unvoiced returns True, we set pitch as 0
      return 0;
    else
      return (float) samplingFreq/(float) lag; // Returns pitch (samplingFreq / lag)
  }
}
