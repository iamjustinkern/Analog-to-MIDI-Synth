// Author: Justin Kern
// MUT 431


#include <Bela.h>
#include <cmath>
#include <libraries/OscReceiver/OscReceiver.h>
#include <libraries/OscSender/OscSender.h>
#include "Yin.h"




const char* remoteIP = "192.168.7.1";
int remotePort = 7562;

// global variables used in render
std::array<float, 2048> window { 0.0f };
int windowIdx = 0;

Yin pitchDetector;

// making multiple threads so that this runs faster
AuxiliaryTask yinWorker;
std::array<float, 2048> workerWindow { 0.0f };
float workerEnvelope = 0.0f;

// envelope variables
float threshold = 0.1f;
float envelope = 0.0f;
float alpha = 0.075f;
bool noteIsOn = false;
int velocity = 0;
float sensitivity = 5.0f;


OscSender oscSender;

// the worker function 
void processPitch(void*) {
	float freq  = pitchDetector.process(workerWindow.data(), 2048);
	// on/off logic
	if (workerEnvelope > threshold  && !noteIsOn && freq > 0.0f) {
		int midiNote = std::round(12 * std::log2(freq/440.0f) + 69);
		rt_printf("test midi");
		velocity = (int)(workerEnvelope * sensitivity * 127.0);
		velocity = std::min(velocity, 127);
        rt_printf("  ---> TRIGGER NOTE ON: MIDI %d, Velocity %d\n", midiNote, velocity);
		oscSender.newMessage("/bela/midi/noteOn").add(midiNote).add(velocity).send();
		noteIsOn = true;
	}
	else if (workerEnvelope < threshold  && noteIsOn){
		rt_printf("  ---> TRIGGER NOTE OFF\n");
		oscSender.newMessage("/bela/midi/noteOff").add(0).add(0).send();
		noteIsOn = false;
	}
}




bool setup(BelaContext *context, void *userData)
{
	
	oscSender.setup(remotePort, remoteIP);
	
	pitchDetector.setup(context->audioSampleRate);
	
	yinWorker = Bela_createAuxiliaryTask(processPitch, 50, "yin-algorithm-worker");
	
	
	return true;
}

void render(BelaContext *context, void *userData)
{
    for (unsigned int n = 0; n < context->audioFrames; n++) {

		// 1. Read the audio input from the microphone
		float in = audioRead(context, n, 0);
		
		// envelope follower calculation
		envelope = (1- alpha) * envelope + alpha * fabsf(in);
		
		// create a window for the yin algorithm to process the buffer
		window[windowIdx] = in;
		++windowIdx;
		if ( windowIdx >= 2048 ) {
			// make copies for the workers
			workerWindow = window;
			workerEnvelope = envelope;
			// waking up the thread to work hard on the math
			Bela_scheduleAuxiliaryTask(yinWorker);
			
			windowIdx = 0;
		}
			

    }
	
}

void cleanup(BelaContext *context, void *userData)
{

}