/// @file

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <stdlib.h>

#include "docopt.h"

using namespace std;

int read_vector(const string &filename, vector<float> &x);

void compare(const vector<float> &vref, const vector<float> &vtest, int &num_voiced,
  int &num_unvoiced, int &num_voiced_unvoiced, int &num_unvoiced_voiced,
  int &num_voiced_voiced, int &num_gross_errors, float &fine_error);

void print_results(int nframes, int num_voiced, int num_unvoiced,
  int num_voiced_unvoiced, int num_unvoiced_voiced, int num_voiced_voiced,
  int num_gross_errors,  float fine_errori, string filename);

const float gross_threshold = 0.2F; //gross error:  +/-20 %

static const char USAGE[] = R"(
pitch_evaluate - Evaluate error rate of pitch evaluation

Usage:
    pitch_evaluate <file-f0ref> ...

Arguments:
    file-f0ref  File with the reference values of F0 (ext. .f0ref)

Note:
    For each reference file, a test file is required in the same directory
    with the same base name and with extension .f0
)";

int main(int argc, const char * argv[])  {
  int vTot=0, uTot=0, vuTot=0, uvTot=0, nTot=0, grossTot=0, vvTot=0, nfiles=0;
  float fineTot=0.0F;

  std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
    {argv + 1, argv + argc},
    true,    // show help if requested
    "2.0");   // version string

  std::vector<std::string> const& file_f0ref = args["<file-f0ref>"].asStringList();

  for (unsigned int i=0; i<file_f0ref.size(); ++i) {
    vector<float> f0ref, f0test;
    if (read_vector(file_f0ref[i], f0ref)) {
      cerr << "Error reading ref file: " << file_f0ref[i] << endl;
      return 2;
    }

    // Change extension of ref file to .f0
    string ftest(file_f0ref[i]);
    string::size_type pos = ftest.rfind('.');
    if (pos != string::npos)
      ftest.erase(pos);    
    ftest += ".f0";
    if (ftest == file_f0ref[i]) ftest += "test";

    if (read_vector(ftest, f0test)) {
      cerr << "Error reading test file: " << ftest << endl;
      return 3;
    }
    
    cout << "### Compare " << file_f0ref[i] << " and " << ftest << "\n";


    int diff_frames = f0ref.size() - f0test.size();
    if (abs(diff_frames) > 3) {
      cerr << "Error: number of frames in ref (" << f0ref.size() 
	   << ") != number of frames in test (" << f0test.size() 
	   << ")\n";
      return 4;
    } 
    if (diff_frames > 0)      f0ref.resize(f0test.size());
    else if (diff_frames < 0) f0test.resize(f0ref.size());

    int num_voiced, num_unvoiced, num_voiced_unvoiced, num_unvoiced_voiced;
    int num_gross_errors, num_voiced_voiced;
    float fine_error;
    compare(f0ref, f0test, num_voiced, num_unvoiced, num_voiced_unvoiced, num_unvoiced_voiced,
			num_voiced_voiced, num_gross_errors, fine_error);    


    vTot     += num_voiced;
    uTot     += num_unvoiced;
    vuTot    += num_voiced_unvoiced;
    uvTot    += num_unvoiced_voiced;

    vvTot    += num_voiced_voiced;
    grossTot += num_gross_errors;
    fineTot  += fine_error;
    nTot     += f0ref.size();
    nfiles++;
    
    print_results(f0ref.size(), num_voiced, num_unvoiced, num_voiced_unvoiced, num_unvoiced_voiced, 
		  num_voiced_voiced, num_gross_errors, fine_error, ftest);
    cout << "--------------------------\n\n";
  }   

  if (nfiles > 1) {
    cout << "### Summary\n";
    print_results(nTot, vTot, uTot, vuTot, uvTot, vvTot, grossTot, fineTot/nfiles, "TOTAL");
    cout << "--------------------------\n\n";
  }
 
  return 0;  
}

int read_vector(const string &filename, vector<float> &x) {
  x.clear();
  ifstream is(filename.c_str());
  if (!is.good())
    return -1;

  float f;
  while (is >> f)
    x.push_back(f);

  return 0;
}

void compare(const vector<float> &vref, const vector<float> &vtest, 
     int &num_voiced, int &num_unvoiced, int &num_voiced_unvoiced, int &num_unvoiced_voiced, 
     int &num_voiced_voiced, int &num_gross_errors, float &fine_error) {
  
  num_voiced = num_unvoiced = num_voiced_unvoiced = num_unvoiced_voiced = 0;
  num_voiced_voiced = num_gross_errors = 0;
  fine_error = 0.0F;
  int nfine = 0;

  if (vref.size() != vtest.size())
      return;

  for (unsigned int i = 0; i < vref.size(); ++i) {

    if (vref[i] == 0.0F)
      num_unvoiced++; 
    else
      num_voiced++;

    if (vref[i] == 0.0F and vtest[i] == 0.0F)
      continue;
  
    if (vref[i] == 0.0F and vtest[i] != 0.0F) {
      num_unvoiced_voiced++;
    } else if (vref[i] != 0.0F and vtest[i] == 0.0F) {
      num_voiced_unvoiced++;
    } else {
      float f = fabs((vref[i] - vtest[i])/vref[i]);
      num_voiced_voiced++;
      if  (f > gross_threshold) {
	num_gross_errors++;
      } else {
	nfine++;
	fine_error += f*f;
      }
    }
  }
  if (nfine > 0)
    fine_error = sqrt(fine_error/nfine);
}

void print_results(int nframes, int num_voiced, int num_unvoiced,
  int num_voiced_unvoiced, int num_unvoiced_voiced, int num_voiced_voiced,
  int num_gross_errors,  float fine_error, string filename) {

  float UU, UV, VV, VU;
  float recU, recV, precU, precV, Fu, Fv, Fss, score;

  cout << fixed << setprecision(2);

  cout << "Num. frames:\t" << nframes 
       << " = " << num_unvoiced << " unvoiced + " 
       << num_voiced << " voiced\n";

  cout << filename << ":\t" << "Unvoiced frames as voiced:\t" << num_unvoiced_voiced << "/" << num_unvoiced 
       << " (" << 100.0F * num_unvoiced_voiced/num_unvoiced << " %)\n";

  cout << filename << ":\t" << "Voiced frames as unvoiced:\t" << num_voiced_unvoiced << "/" << num_voiced 
       << " (" << 100.0F * num_voiced_unvoiced/num_voiced << " %)\n";

  cout << filename << ":\t" << "Gross voiced errors (+" << 100*gross_threshold << " %):\t" << num_gross_errors << "/" << num_voiced_voiced
       << " (" << 100.0F * num_gross_errors/(1.e-12 + num_voiced_voiced) << " %)\n";
  cout << filename << ":\t" << "MSE of fine errors:\t" << 100*fine_error << " %\n";

  VV = num_voiced_voiced;
  VU = num_voiced_unvoiced;
  UV = num_unvoiced_voiced;
  UU = num_unvoiced - num_unvoiced_voiced;

  VV -= num_gross_errors / 2.;
  VU += num_gross_errors / 2.;

  recU = UU / (1.e-12 + UU + UV);
  recV = VV / (1.e-12 + VV + VU);
  precU = UU / (1.e-12 + UU + VU);
  precV = VV / (1.e-12 + VV + UV);

  Fu = 2 * (recU * precU) / (1.e-12 + recU + precU);
  Fv = 2 * (recV * precV) / (1.e-12 + recV + precV);

  Fss = sqrt(Fu * Fv);

  score = Fss * (1 - fine_error);

  cout << fixed << setprecision(2);
  cout << "\n===>\t" << filename << ":\t" << 100 * score << " %\n";
}
