//  comptonParams.cc
//  Special header file used to collect parameters that are edited in this file
//
// Old history deleted 6/15/2015 gbf
#include "comptonParams.h"
comptonParams::comptonParams(){
  return;
}
void comptonParams::newRun(int run) {
  // update parameters, including run-dependent parameters
  //
  runnum=run;
  //
  //FADC related parameters
  channel_calorimeter = 0; // compton calorimeter FADC channel
  last_event = 1000000;
  sum_size = 3;
  ped_value = 2370.;  //changed for test data gbf
  accumHistoRange=2.E7;  //used for raw accumulator histogram XMAX
  accumScaledRange=accumHistoRange;  //used for scaled accumulator histogram XMAX

  pedestal_horizontalfinger = 3687;
  pedestal_verticalfinger = 3701;
  sthr1 = ped_value - 10.;
  sthr2 = ped_value - 60.;
  sum_pedchannel = ped_value * sum_size;
  sum_sthr1 = sthr1 * sum_size;
  pulse_sep = 120;
  pre_thr = 5;
  post_thr = 20;
  start_deadtime = 10;
  stop_deadtime = 10;
  snap_num = 200;
  sample_time = 0.000000005; 	// 5 ns per sample
  //
  //compton Status parameters
  cavity_power_on_thresh = 60;
  cavity_power_off_thresh = 500;		// Watts, Oct 13, 2010, for DVCS
  // Don't understand how this factor is deterimined.  Boot it up for now 6/17/15
  bcm_calibration_factor = 2.*7.68E-5*.912;	// microamps per normalized bcm scaler
  bcm_calibration_factor*= 10.0;
  cav_pow_calibration_factor = 0.0329;  //calibrate normalized cavity power to watts, May 7, 2010
  bcm_cutoff = 1;	// in microamps
  high_bcm_cutoff = 5;	// set very low for now
  HV_cutoff = 0; //set Mar 4, 2012, for g2p 
  helicity_delay=0;  //set for no helicity delay
  helicity_structure-8;  //needed if we shift to a delayed hellicity
  helicity_bits=30;
return;
}

