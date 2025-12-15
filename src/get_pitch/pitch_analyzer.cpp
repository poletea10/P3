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
      for(unsigned int n=0; n<x.size()-l; n++){
          r[l] += x[n]*x[n + l];
      }
      r[l]=r[l]/static_cast<float>(x.size());
      /**   
       \DONE Autocorrelation implemented
       \f[
       r[l] = (1/N)*\sum_{n=0}^{N-l} x_i^*[n] x_i[n+l]
       \f]
       - Inicializamos la autocorrelacion a 0.
       - Sumamos la multiplicacion con la señal desplazada.
       */
    }

    if (r[0] == 0.0F) //to avoid log() and divide zero 
      r[0] = 1e-10; 
  }

  // Pitch estimation AMDF method (not used here since it gives worse performance than Autocorrelation/COSA)
  void PitchAnalyzer::amdf(const std::vector<float> &x, std::vector<float> &d) const {
    const unsigned int N = (unsigned int)x.size();
    const unsigned int L = (unsigned int)d.size(); // npitch_max

    for (unsigned int l = 0; l < L; ++l) { // For all permitted lags
      float acc = 0.0f;

      for (unsigned int n = 0; n < N - l; ++n) {
        acc += fabsf(x[n] - x[n + l]);
      }
      d[l] = acc / (float)(N-l); // Normalized since we're windowing and not all lags are calculated from the same number of samples
    }
  } 


  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;

    window.resize(frameLen);

    switch (win_type) {
    case HAMMING:
      /// \TODO Implement the Hamming window
      for (unsigned int n = 0; n < frameLen; ++n) {
        window[n] = 0.54f - 0.46f * cosf(2.0f * M_PI * n / (frameLen - 1));
      } 
      /// \DONE Hamming window implemented, though its use is not recommended (RECT windows give better results)
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
    
    // We'll choose if a frame is voiced or not "por descarte"

    // Energy gate (if the power is too low, it's most surely unvoiced)
    if (pot < this->uminPot)
        return true; // unvoiced
    
    // Intensity of periodicty vs pot
    if (rmaxnorm > this->umaxnorm_hi) // If the ratio is quite large, it's voiced
        return false;   // voiced
    
    if (rmaxnorm < this->umaxnorm_lo)
        return true;

    // Grey area (rmaxnorm is in a middle ground)
    if (r1norm >= this->ur1norm) // voiced parts tend to change slowly with just a 1 sample lag, so r[1]/r[0] is bigger
      return false;
    else 
      return true;

    /// \DONE Voiced/Unvoiced detector implemented \n
    /// It first tags all low power frames as unvoiced. The other ones are checked by rmaxnorm and, if they fall in a "Grey area", we check r1norm (voiced parts tend to change slowly with just a 1 sample lag)
  }

  float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen)
      return -1.0F;

    // Center clipping for voiced detection
    vector<float> x_clip = x;
    for (float &sample : x_clip) {
      if (fabsf(sample) < this->clip_level) {
          sample = 0.0f;
      }
    }

    //Window input frame (for clipped and non-clipped signal)
    for (unsigned int i=0; i<x.size(); ++i){
      x[i] *= window[i];
      x_clip[i] *= window[i];
    }

    // r will hold the autocorrelation values for lags from 0 to npitch_max-1 (max lag = lowest freq)
    vector<float> r_clip(npitch_max);
    vector<float> r_cosa(npitch_max);

    //Compute correlation
    autocorrelation(x_clip, r_clip);

    autocorrelation(x, r_cosa);
    r_cosa[0] = 100*r_cosa[0]; // Multiply by K = 100 to reduce COSA oscillation
    r_cosa[0] = r_cosa[0]*0.5; // Now we have the one-sided autocorrelation (with reduce COSA oscillation)

    // Complex cepstrum of the one-sided correlation (COSA)
    vector<float> COSA(npitch_max);
    COSA[0]=log10(r_cosa[0]);
    for (unsigned int n = 1; n < COSA.size(); ++n) {

        float s = 0.0f;

        // s = sum_{k=1..n-1} (k/n) * Cplus[k] * Rplus[n-k]
        for (unsigned int k = 1; k < n; ++k) {
            s += ((float)k / (float)n) * COSA[k] * r_cosa[n - k];
        }

        // Cplus[n] = (Rplus[n] - s) / Rplus[0]
        COSA[n] = (r_cosa[n] - s) / r_cosa[0];
    }

    vector<float>::const_iterator iR_clip = r_clip.begin(), iRMax_clip = iR_clip; // We create two iterators that, at this instant, point to the first element of r

    vector<float>::const_iterator iR_cosa = COSA.begin(), iRMax_cosa = iR_cosa;

    /// \TODO 
    /// Find the lag of the maximum value of the autocorrelation away from the origin.<br>
    /// Choices to set the minimum value of the lag are:
    ///    - The first negative value of the autocorrelation.
    ///    - The lag corresponding to the maximum value of the pitch.
    /// In either case, the lag should not exceed that of the minimum value of the pitch.

    iRMax_clip = std::max_element(iR_clip + npitch_min, iR_clip + npitch_max); // Returns the iterator pointing at the max position between npitch_min (smallest lag) & npitch_max (biggest lag)

    unsigned int lagR_clip = iRMax_clip - r_clip.begin(); // Computes the index (lag) of the maximum in that range (position of max iterator - pointer at the beginning)
    // Now lag is the number of samples corresponding to our estimated pitch period!

    iRMax_cosa = std::max_element(iR_cosa + npitch_min, iR_cosa + npitch_max);

    unsigned int lagR_cosa = iRMax_cosa - COSA.begin();

    // Check gross errors
    unsigned int bestLag = lagR_clip;
    float ratio = (float)lagR_cosa / (float)lagR_clip;

    if (npitch_max>=lagR_clip*2 && r_clip[lagR_clip/2] >= this->harm_ratio * r_clip[lagR_clip]){ // If largR/2 is really similar to lagR, we might be looking at a harmonic (harmonic consistency). So the fundamental should be at lagR*2
        lagR_clip = lagR_clip*2;
        bestLag = lagR_clip*2;
    }

    // if lagR_cosa is above +20% of lagR_clip (normally gross errors are caused by overestimating the pitch, so we should get the lower pitch/higher lag)
    if (ratio >= 1.2f)
        bestLag = lagR_cosa;

    /// \DONE Lag of maximum value implemented \n
    /// Since we've already set a minimum and maximum pitch (the code came like this), we used the second option: 
    /// The minimum lag value permitted is the one corresponding to the inputted maximum value of the pitch (F=350 in get_pitch.cpp). 
    /// The maximum lag value permitted (we won't go above it using std::max_element) is the one corresponding to the inputted minimum value of the pitch (F=50 in get_pitch.cpp) \n
    /// A lag that is very similar to its lag/2 counterpart is considered a harmonic, and if the lag measured by COSA is 20% bigger than the lag measured by autocorrelation, we choose the COSA lag (autocorrelation might have overestimated the pitch)

    float pot = 10 * log10(r_clip[0]); // Power of current window -> Good for voicing decision, since unvoiced parts have low energy, and vice versa
#if 0
    if (r_clip[0] > 0.0F)
      cout << pot << '\t' << r_clip[1]/r_clip[0] << '\t' << r_clip[lagR_clip]/r_clip[0] << endl;
#endif
    
    if (unvoiced(pot, r_clip[1]/r_clip[0], r_clip[lagR_clip]/r_clip[0])) // If unvoiced returns True, we set pitch as 0
      return 0;
    else
      return (float) samplingFreq/(float) bestLag; // Returns pitch (samplingFreq / lag)
  }
}
