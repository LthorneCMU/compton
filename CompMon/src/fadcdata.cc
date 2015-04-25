// fadcdata.cc
//
// used to unpack and access FADC data
//
// Sep 14, 09 copied from rundiana/src version
//Sep 14, 09  added flags for randome waveform sample input
// Feb 6, 2015  addded debug info  gbf
#include <iostream>
#include <stdio.h>

using namespace std;

#include "bankstructure.h"
#include "fadcdata.h"
#define MAX_SAMPLES 100000//changed from 100000
fadcdata::fadcdata(){
  for(int chan=0; chan<MAX_FADC_CHANNELS; chan++){
    RawSamples[chan]=NULL;
    UserBits[chan]=NULL;
  }
  return;

}
fadcdata::fadcdata(comptonParams* theParamsIn){
  theParams=theParamsIn;
  for(int chan=0; chan<MAX_FADC_CHANNELS; chan++){
    RawSamples[chan]=NULL;
    UserBits[chan]=NULL;
  }
  return;
}

int fadcdata::newMPS() {
  //set Valid Flags to False for all fadc data
  for(int chan=0; chan<MAX_FADC_CHANNELS; chan++){
    SamplesValid[chan]=false;
    NumberSamples[chan]=0;
    //RawSamples[chan]=NULL;
    //UserBits[chan]=NULL;
    AccumValid[chan]=false;
    for(int i=0; i<6; i++){
      AccumValue[chan][i]=0;
      AccumNumSamples[chan][i]=0;
      //      PedValue[i]=theParams->GetPedestal(); //run dependent pedestals
    }
    Sums_NumberFADCTriggers[chan]=-1;
    Sums_NumberTriggersSummed[chan]=-1;
  }

  return 0;
}
//print out subbank for debugging
void fadcdata::DumpBank(bankstructure bank) {
  uint32_t* data=bank.Data;
  int tmp;
  printf("Bank Dump*********************\n");
  printf("Bank Tag          0x%x\n",bank.Tag);
  printf("Bank Header       0x%x\n",bank.Header);
  printf("Number of data words %d\n",bank.DataWords);
  printf("Pointer to data      %d\n",bank.BankIndex);
  for(int i=0; i<bank.DataWords-2;i++){
    tmp=data[i];
    printf("i %4d  data: %8d  0x%8x\n",i,tmp,tmp);
  }

  return;
}	
// Unpack All subbanks
int fadcdata::UnpackAllSubbanks(THaCodaFile* codaData, vmeauxdata* theVMEauxdata){
    bankstructure subbank;  //will get subbank info
    subbank.Tag=1;
    while(subbank.Tag>0){
      subbank=codaData->getNextSubbank(0); //arg=# verbose words to print out
      if(subbank.Tag>0){
	if(subbank.Tag==0x212){
	  UnpackSamples(subbank); //Waveform samples
	}else if (subbank.Tag==0x211){
	  UnpackAccumulators(subbank);  //accumulator readout
	}else if (subbank.Tag==0x210){
	  theVMEauxdata->Unpack(subbank);  //AuxInfo (scaler reads, etc.)
	  //	      theVMEauxdata->DebugDump(5);
	}else if (subbank.Tag==0x213){
	  UnpackSums(subbank,0,1);  //subbank,verbose,abort on error
	}else {
	  printf("Unknown subbank tag %x \n",subbank.Tag);
	}
      }
    }
  return 0;
}
// Unpack FADC waveform data
int fadcdata::UnpackSamples(bankstructure bank) {
  //extract info from ROC subbank
  uint32_t* data=bank.Data;
  if((unsigned)data[0]==0xda0000da) {
    printf("Error UnpackSamples: Sis3320 not ready\n");
    return -1;
  }
  int chan=data[0];   //first word is channel number
  //  int chan = 0; //channel hardwired to zero for now
  if(chan>=0 && chan<8) {
    if(RawSamples[chan]==NULL){
      printf("Creating array for unpacked data channel=%d\n", chan);
      RawSamples[chan]=new int[MAX_SAMPLES];     //array for unpacked data
      UserBits[chan]=new int[MAX_SAMPLES];     //array for unpacked data
    }
    triggermode=data[1]&0xffff;;          //fadc trigger mode
    sampleformat=(data[1]>>16)&0xffff;   //sample data format
    if(triggermode==5){     //random sampling implemented 9/14/2009
      RandomTimes[chan]= 1;
    }else{
      RandomTimes[chan]= 0;
    }
    NumberSamples[chan]=data[2];
    if(triggermode==3 || triggermode==4 || triggermode==5){
      NumberEvents[chan]=data[4];
      samplepointer[chan]=0;
      SamplesPerEvent[chan]=data[3];
    } else {
      NumberEvents[chan]=1;
      samplepointer[chan]=data[2];          //fadc address of last sample
      SamplesPerEvent[chan]=NumberSamples[chan];
    }
    //unpack data if needed and set pointer to data
    int* unpacked=RawSamples[chan];
    int* userbit=UserBits[chan];
    if(NumberSamples[chan]>MAX_SAMPLES){
      printf("Truncating %d samples to %d\n",NumberSamples[chan], MAX_SAMPLES);
      NumberSamples[chan]=MAX_SAMPLES;
    }
    int word=0;
    int index=0;
    if(sampleformat==2){
      //unpack 2 12 bit samples and 2 user bits per dataword
      for(int event=0; event<NumberEvents[chan]; event++){
        PulseIndex[chan][event]=index;
        if(triggermode==4 || triggermode==5) {
          word++;             //skip header tag
          UnpackedClock[chan][event]=data[word+5];
          word++;
        } else {
          UnpackedClock[chan][event]=0;
        }
        for(int i=0; i<SamplesPerEvent[chan]; i+=2){
	  if(index<MAX_SAMPLES-1){
	    unpacked[index]=data[word+5]&0xfff;
	    unpacked[index+1]=(data[word+5]>>16)&0xfff;
	    userbit[index]=(data[word+5]>>15)&0x1;
	    userbit[index+1]=(data[word+5]>>31)&0x1;
	  }
          word++;
          index+=2;
          if(index>=MAX_SAMPLES-1) break;
        }
      }
      /*
         int IsFinishedBit = data[word+6];
         if(IsFinishedBit != 0){
         printf("Error UnpackSamples: sis3320 not finished\n");
         }
         int finishedMPScheck = (data[word+7] & 0x80)>>7;
         cerr<<"finishedMPScheck "<<finishedMPScheck<<" isfinishedbit "<<IsFinished
         Bit<<endl;
         if(finishedMPScheck!=1){
         printf("Error UnpackSamples: MPS finishd before end of readout!\n");
         }
       */
    } else if (sampleformat==1) {
      for(int event=0; event<NumberEvents[chan]; event++){
        if(triggermode==4) {
          UnpackedClock[chan][event]=data[word+5];
        } else {
          UnpackedClock[chan][event]=0;
        }
        for(int i=0; i<SamplesPerEvent[chan]; i++){
          unpacked[index]=data[word+5]&0xfff;
          userbit[index]=(data[word+5]>>15)&0x1;
          word++;
          index++;
          if(index>=MAX_SAMPLES) break;
        }
      }
    } else {
      printf("Invalid Sample Format n");
    }
    SamplesValid[chan]=true;
    return 0;
  }else {
    printf("Error: UnpackSamples found chan=%d not recognized\n",chan);
    return -1;
  }

}
pulsestructure fadcdata::GetPulse(int chan,int pulsenumber){
  pulsestructure pulse=CurrentPulse[chan];
  if(pulsenumber>=NumberEvents[chan]){
    pulse.Valid=0;
    pulse.Index=0;
    pulse.Clock=0;
    pulse.PulseNumber=-1;
    pulse.NumberSamples=0;
    pulse.Data=NULL;
    return pulse;
  }
  int* unpacked=RawSamples[chan];
  int* unpackedbits=UserBits[chan];
  pulse.Index=PulseIndex[chan][pulsenumber];
  pulse.Valid=1;
  pulse.PulseNumber=pulsenumber;
  pulse.Clock=UnpackedClock[chan][pulsenumber];
  pulse.Data=&unpacked[pulse.Index];
  pulse.UserBits=&unpackedbits[pulse.Index];
  pulse.NumberSamples=SamplesPerEvent[chan];
  pulse.RandomTimes=RandomTimes[chan];
  int sum=0;
  for(int i=0;i<pulse.NumberSamples;i++){
    sum+=unpacked[pulse.Index+i];
  }
  pulse.Sum=sum;
  return pulse;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int fadcdata::UnpackAccumulators(bankstructure bank) {
  //extract info from ROC subbank
  uint32_t* data=bank.Data;
  int chan;

  if((unsigned) data[0]==0xda0000da) {
    printf("Error UnpackAccumulators:: Sis3320 not ready\n");
    return -1;
  }
  // for accum-and-triggered mode: sampling subbank is inactive when
  // 0xaaaaaaaa flag is present
  if ((unsigned) data[0] == 0xaaaaaaaa){
    chan = data[1];
    AccumValid[chan] = false;
    return 0;
  }
  chan=data[0];   //first word is channel number
  int64_t tmp64;
  if(chan>=0 && chan<8) {
    int index=1;
    for(int i=0; i<6; i++){
      AccumNumSamples[chan][i]=data[index];
      tmp64=data[index+2];
      tmp64=tmp64<<32;
      tmp64+=data[index+1];
      AccumValue[chan][i]=tmp64;;
      index+=3;
    }
    //now unpack settings
    DacSetting[chan]=data[index++];      //Dac setting
    AccumThresh[chan][1]=data[index++];  //Thresh 1
    AccumThresh[chan][2]=data[index++];  //Thresh 2
    unsigned int tmp= data[index++];     //packed N5N6 info
    N5after[chan]= tmp & 0xff;
    tmp=tmp>>8;
    N5before[chan]=tmp & 0xff;
    tmp=tmp>>8;
    N4after[chan]=tmp & 0xff;
    tmp=tmp>>8;
    N4before[chan]=tmp & 0xff;

    AccumValid[chan]=true;
    return 0;
  }else {
    return -1;
  }
}
int fadcdata::UnpackSums(bankstructure bank, int verbose=0, int abortOnError=0) {
  //
  // unpack fadc triggered data that has already been summed into a single
  // word per trigger during readout of fadc  (bank 0x213)
  // If verbose>0,   dump info
  // If abortOnError>0, unpack even if data is flagged with a timing error
  //
  uint32_t* data;
  data=bank.Data;
  Sums_InputRegister=data[0];  //first word makes sure fadc was not busy, make sure MPS bit is on
  int mpscheck = (Sums_InputRegister & 0x80)>>7;
  int chanStart=0;   //channel number, set to zero for now
  int NumTriggers=data[1];
  int NumTriggersSummed=data[2];
  int NumSamples=data[3];
  int PedestalSubtracted=data[4];
  Sums_NumberFADCChannels=data[5];
  int chanEnd=chanStart+Sums_NumberFADCChannels-1;
  //double PedCorrection[MAX_FADC_CHANNELS];

  if(verbose>0){
    printf(" subbank 0x213 dump (summed pulses)\n");
    printf("  InputRegister      0x%x\n", Sums_InputRegister);
    printf("  NumTriggers        %8d\n",NumTriggers);
    printf("  NumTriggersSummed  %8d\n", NumTriggersSummed);
    printf("  NumSamples         %8d\n", NumSamples);
    printf("  CODA Pedestal       %8d\n", PedestalSubtracted);
    printf("  NumberADCChannels  %8d\n", Sums_NumberFADCChannels);
  }
  if(mpscheck ==1 || abortOnError>0){
    if(chanStart>=0 && chanEnd<MAX_FADC_CHANNELS){
      for(int chan=chanStart; chan<=chanEnd; chan++){
        Sums_NumberFADCTriggers[chan]=NumTriggers;
        Sums_NumberTriggersSummed[chan]=NumTriggersSummed;
        Sums_NumberSamplesSummed[chan]=NumSamples;
        Sums_PedestalSubtracted[chan]=PedestalSubtracted;
        //take out DAQ nominal Pedestal subtraction and put in actual correction
	//comment out Pedcorrection stuff (move to fadcTriggeredclass
        //PedCorrection[chan]=PedValue[chan]*NumSamples-PedestalSubtracted;
        SumsValid[chan]=true;
      }
      int pointer=6;  //first data word for firsttrigger
      for(int trig=0; trig<NumTriggersSummed; trig++){
        Sums_Clock[trig]=data[pointer++];
        for(int chan=chanStart; chan<=chanEnd; chan++){
	  //          SumsData[chan][trig]=data[pointer++]+PedCorrection[chan];
          Sums_Data[chan][trig]=(double) data[pointer++];
        }
      }
    }
  } 
  if(mpscheck !=1){
    printf("Error: fadcdata::Unpacksums:");
    printf("fadc readout took too long!  MPS has ended!\n");
  }
  return 0;
}
int fadcdata::UnpackTimer(bankstructure bank) {
  //extract info from HAPPEX timer board
  uint32_t* data=bank.Data;
  TimerPar1=data[0];
  TimerPar2=data[1];
  TimerDac[0]=-1;
  TimerDac[1]=-2;
  if(bank.DataWords>1) TimerDac[0]=data[2];
  if(bank.DataWords>2) TimerDac[1]=data[3];

  return 0;
}
