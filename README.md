Overview: 
To use this project, you must have a microphone feeding audio into a Bela board. 
The Bela must be networked to a computer running the JUCE plugin. As you sing or 
play an instrument into the mic, the Bela tracks the pitch and sends it to JUCE, where 
the user uses the GUI to choose the synth waveform, tweak the ADSR envelope, and 
adjust the filter to create the final sound.


Bela:
The Bela board first reads in the audio buffer from the mic input. From there, it performs
the envelope-follower calculation to determine an appropriate threshold for activating
the OSC message. When computing the Yin algorithm, it uses a 2048-sample window so that it 
has time to run. From there, it makes copies of the calculated envelope window
buffer. These copies are used by the threads in the worker function. The worker function
is what calculates the frequency using the Yin algorithm. If the envelope meets the
threshold and the frequency is not 0, the MIDI note/velocity is calculated and sent over
to the JUCE processor via OSC. Triggering the first branch also sets the note-on flag,
which is used in the next condition statement for when the note is off-messaged OSC.

JUCE Processor:
The processor is what receives the MIDI note and velocity from the Bela board over
OSC. There is a method that converts MIDI notes into a frequency value and
velocity into a gain level. This method is also where the logic is for receiving the note-on
and note-off messages. When the message is note-on, it saves the frequency value and
level into a thread-safe variable. Along with that, a thread-safe flag is used in
the process block to signal that its ok to trigger the note-on or note-off function of the
ADSR class. This kind of thread safety is necessary for this project since the audio
thread is working independently from the network one. Using atomic variables will
prevent data races. From there, in the process block, it loads in the type of waveform
that was selected. The user can choose among Sine, Sawtooth, and Triangle
waves in the GUI. This information is transferred over to the process block via the
AudioProcessorValueTree class. From there, it stores the frequency and level it
received over the network into local variables and calculates the phase delta. Based on
the waveform, it calculates the current sample differently in a phase accumulator. Once
the wave for the sample is calculated, the buffer gets processed by the ADSR object.
The ADSR object processes the buffer based on the attack, decay, sustain, and release
parameters that were loaded in from the GUI. After the ADSR object processes the
buffer, the buffer is then processed by my low-pass filter class. This class has two
parameters controlled by the GUI, namely the cutoff frequency and the resonance.
