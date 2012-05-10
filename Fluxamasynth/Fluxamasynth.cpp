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
#include "Fluxamasynth.h"
#include "NewSoftSerial.h"

Fluxamasynth::Fluxamasynth() : synth(255, 4) {                   // 255 -> do not use rx; pin 4 for tx
    synthInitialized = 0;                                        // Initialization needs to be done
}

Fluxamasynth::Fluxamasynth(byte rxPin, byte txPin) : synth(rxPin, txPin) {
    synthInitialized = 0;                                        // Initialization needs to be done
}

void Fluxamasynth::begin() {
    if (!(this->synthInitialized)){
      this->synth.begin(31250);		                         // Set MIDI baud rate
      delay(2);                                                  // let the port settle
      this->synthInitialized = -1;                               // Initialization has been done
    }
}

 size_t Fluxamasynth::fluxWrite(byte c) {
    if (!(this->synthInitialized)){
        this->begin();
    }
    this->synth.write(c);
}

 size_t Fluxamasynth::fluxWrite(byte *buf, int cnt) {
    int  i;
    
    if (!this->synthInitialized){
        this->begin();
    }
    for (i=0; i<cnt; i++) {
        this->synth.write(buf[i]);
    }
}

void Fluxamasynth::noteOn(byte channel, byte pitch, byte velocity) {
    byte command[3] = { 0x90 | (channel & 0x0f), pitch, velocity };
    this->fluxWrite(command, 3);
}

void Fluxamasynth::noteOff(byte channel, byte pitch) {
    byte command[3] = { 0x80 | (channel & 0x0f), pitch, byte(0x00) };
    this->fluxWrite(command, 3);
}

void Fluxamasynth::programChange(byte bank, byte channel, byte v) {
    // bank is either 0 or 127
    byte command[3] = { 0xB0 | (channel & 0x0f), byte(0x00), bank };
    this->fluxWrite(command, 3);
    this->fluxWrite(0xc0 | (channel & 0x0f));
    this->fluxWrite(v);
}

void Fluxamasynth::pitchBend(byte channel, int v) {
    // v is a value from 0 to 1023
    // it is mapped to the full range 0 to 0x3fff
    v = map(v, 0, 1023, 0, 0x3fff);
    byte command[3] = { 0xe0 | (channel & 0x0f), byte(v & 0x00ef), byte(v >> 7) };
    this->fluxWrite(command, 3);
}

void Fluxamasynth::pitchBendRange(byte channel, byte v) {
    // Also called pitch bend sensitivity
    //BnH 65H 00H 64H 00H 06H vv
    byte command[7] = {0xb0 | (channel & 0x0f), 0x65, 0x00, 0x64, 0x00, 0x06, (v & 0x7f)};
    this->fluxWrite(command, 7);
}

void Fluxamasynth::midiReset() {
    this->fluxWrite(0xff);
}

void Fluxamasynth::setChannelVolume(byte channel, byte level) {
    byte command[3] = { (0xb0 | (channel & 0x0f)), 0x07, level };
    this->fluxWrite(command, 3);
}

void Fluxamasynth::allNotesOff(byte channel) {
    // BnH 7BH 00H
    byte command[3] = { (0xb0 | (channel & 0x0f)), 0x7b, 0x00 };
    this->fluxWrite(command, 3);
}

void Fluxamasynth::setMasterVolume(byte level) {
    //F0H 7FH 7FH 04H 01H 00H ll F7H
    byte command[8] = { 0xf0, 0x7f, 0x7f, 0x04, 0x01, 0x00, (level & 0x7f), 0xf7 };
    this->fluxWrite(command, 8);
}

void Fluxamasynth::setReverb(byte channel, byte program, byte level, byte delayFeedback) {
    // Program 
    // 0: Room1   1: Room2    2: Room3 
    // 3: Hall1   4: Hall2    5: Plate
    // 6: Delay   7: Pan delay
    this->fluxWrite(0xb0 | (channel & 0x0f));
    this->fluxWrite(0x50);
    this->fluxWrite(program & 0x07);
 
    // Set send level
    this->fluxWrite(0xb0 | (channel & 0x0f));
    this->fluxWrite(0x5b);
    this->fluxWrite(level & 0x7f);
  
    if (delayFeedback > 0) {
      //F0H 41H 00H 42H 12H 40H 01H 35H vv xx F7H
      byte command[11] = { 0xf0, 0x41, byte(0x00), 0x42, 0x12, 0x40, 0x01, 0x35, (delayFeedback & 0x7f), 0xf7 };
      this->fluxWrite(command, 11);
    }
}

void Fluxamasynth::setChorus(byte channel, byte program, byte level, byte feedback, byte chorusDelay) {
    // Program 
    // 0: Chorus1   1: Chorus2    2: Chorus3 
    // 3: Chorus4   4: Feedback   5: Flanger
    // 6: Short delay   7: FB delay
    this->fluxWrite(0xb0 | (channel & 0x0f));
    this->fluxWrite(0x51);
    this->fluxWrite(program & 0x07);
 
    // Set send level
    this->fluxWrite(0xb0 | (channel & 0x0f));
    this->fluxWrite(0x5d);
    this->fluxWrite(level & 0x7f);
  
    if (feedback > 0) {
    //F0H 41H 00H 42H 12H 40H 01H 3BH vv xx F7H
	byte command[11] = { 0xf0, 0x41, byte(0x00), 0x42, 0x12, 0x40, 0x01, 0x3B, (feedback & 0x7f), 0xf7 };
	this->fluxWrite(command, 11);
    }
  
    if (chorusDelay > 0) {
    // F0H 41H 00H 42H 12H 40H 01H 3CH vv xx F7H
        byte command[11] = { 0xf0, 0x41, byte(0x00), 0x42, 0x12, 0x40, 0x01, 0x3C, (chorusDelay & 0x7f), 0xf7 };
	this->fluxWrite(command, 11);
    }
}

void Fluxamasynth::setTVFResonance(byte channel, byte resonance) {
    // 0h - max reduce, 40h - no change, 7fh - max increase
    // Bnh 63h 01h 62h 21h 06h vv
    byte command[7] = {0xb0 | (channel & 0x0f), 0x63, 0x01, 0x62, 0x21, 0x06, (resonance & 0x7f)};
    this->fluxWrite(command, 7);
}

void Fluxamasynth::setTVFCutoff(byte channel, byte cutoff) {
    // 0h - max reduce, 40h - no change, 7fh - max increase
    // Bnh 63h 01h 62h 21h 06h vv
    byte command[7] = {0xb0 | (channel & 0x0f), 0x63, 0x01, 0x62, 0x20, 0x06, (cutoff & 0x7f)};
    this->fluxWrite(command, 7);
}

void Fluxamasynth::setEnvAttack(byte channel, byte attack) {
	// bnh 63h 01h 62h 63h 06h vv
	byte command[7] = {0xb0 | (channel & 0x0f), 0x63, 0x01, 0x62, 0x63, 0x06, (attack & 0x7f)};
    this->fluxWrite(command, 7);
}

void Fluxamasynth::setMasterPan(byte pan1, byte pan2) {
    // f0h 41h 00h 42h 12h 40h 00h 06h vv xx f7h
    byte command[11] = { 0xf0, 0x41, 0x00, 0x42, 0x12, 0x40, 0x00, 0x06, (pan1 & 0x7f), (pan2 & 0x7f), 0xf7 };
    this->fluxWrite(command, 11);
}

void Fluxamasynth::setPortamento(byte channel, byte enable) {
	// bnh 41h cc
	byte command[3] = {0xb0 | (channel & 0x0f), 0x41, enable};
	this->fluxWrite(command, 3);
}

void Fluxamasynth::setSpecialSynthControl(byte channel, byte p1, byte p2) {
	// bnh 63h 37h 62h xx 06h vv
	byte command[7] = {0xb0 | (channel & 0x0f), 0x63, 0x37, 0x62, (p1 & 0x7f), 0x06, (p2 & 0x7f)};
	this->fluxWrite(command, 7);
}

