/*  -------------------------------------------------------
    Fluxamasynth.cpp
    A library to facilitate playing music on the Modern Device Fluxamasynth board.
    More info is at www.moderndevice.com; refer to "Using the Fluxamasynth Arduino 
    Library" in the DocsWiki.
    -------------------------------------
    This version was derived from the library provided by Modern Devices
    . The NewSoftSerial library is used to communicate with the synth
    . The Constructor has been changed to take the rx and tx pin numbers
      (and instantiate the NewSoftSerial library); The default constructtor
      writes to pin 4, and disables receive.
    . A Polymorphic function, fluxWrite(...) has been added to write thru
      to the synth; it may be used alone, or in combination with existing
      library functions (use of fluxWrite must leave the synth capable of
      accepting library commands after, however).
    -------------------------------------
    This software is in the public domain.
    Modified 4/2011 R.McGinnis
    ------------------------------------------------------- */

#include "Arduino.h"
#include "NewSoftSerial.h"
#include "PgmChange.h"

#ifndef  Fluxamasynth_h
#define  Fluxamasynth_h

class Fluxamasynth
{
  private:
    NewSoftSerial synth;
    byte synthInitialized;
    void begin();
  public:
    // default constructor sets NewSoftSerial to use pin 4 for tx, and inhibits rx
    Fluxamasynth();
    // constructor with 2 parameters sets NewSoftSerial's accordingly
    Fluxamasynth(byte rxPin, byte txPin);
    virtual size_t fluxWrite(byte c);
    virtual size_t fluxWrite(byte *buf, int cnt);
    void noteOn(byte channel, byte pitch, byte velocity);
    void noteOff(byte channel, byte pitch);
    void programChange (byte bank, byte channel, byte v);
    void pitchBend(byte channel, int v);
    void pitchBendRange(byte channel, byte v);
    void midiReset();
    void setChannelVolume(byte channel, byte level);
	void allNotesOff(byte channel);
    void setMasterVolume(byte level);
    void setReverb(byte channel, byte program, byte level, byte delayFeedback);
    void setChorus(byte channel, byte program, byte level, byte feedback, byte chorusDelay);
	void setMasterPan(byte pan1, byte pan2);
    void setTVFResonance(byte channel, byte resonance);
    void setTVFCutoff(byte channel, byte cutoff);
	void setEnvAttack(byte channel, byte attack);
	void setPortamento(byte channel, byte enable);
	void setSpecialSynthControl(byte channel, byte p1, byte p2);
};

#endif
