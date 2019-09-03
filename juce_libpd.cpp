/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  
  ==============================================================================
*/

/*
 * Copyright (c) 2011 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxPd for documentation
 *
 * This project uses libpd, copyrighted by Miller Puckette and others using the
 * "Standard Improved BSD License". See the file "LICENSE.txt" in src/pd.
 *
 * See http://gitorious.org/pdlib/pages/Libpd for documentation
 *
 */


#ifdef JUCE_LIBPD_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#include "juce_libpd.h"
#include <algorithm>

// needed for libpd audio passing
#ifndef USEAPI_DUMMY
	#define USEAPI_DUMMY
#endif

using namespace std;
using namespace pd;

//--------------------------------------------------------------------
ofxPd::ofxPd() : PdBase() {
	inBuffer = NULL;
	computing = false;
	clear();
}

//--------------------------------------------------------------------
ofxPd::~ofxPd() {
    clear();
}

//--------------------------------------------------------------------
bool ofxPd::init(const int numOutChannels, const int numInChannels, 
                 const int sampleRate, const int ticksPerBuffer, bool queued) {
	
    log = juce::Logger::getCurrentLogger();
    
	// init pd
	if(!PdBase::init(numInChannels, numOutChannels, sampleRate, queued)) {
		log->writeToLog("could not init");
		clear();
		return false;
	}
    
	ticks = ticksPerBuffer;
	bsize = ticksPerBuffer*blockSize();
	srate = sampleRate;
	inChannels = numInChannels;
	outChannels = numOutChannels;

	// allocate buffers
	inBuffer = new float[numInChannels*bsize];

	log->writeToLog("inited");
	log->writeToLog(" samplerate: " + to_string(sampleRate));
	log->writeToLog(" channels in: " + to_string(numInChannels));
	log->writeToLog(" channels out: " + to_string(numOutChannels));
	log->writeToLog(" ticks: " + to_string(ticksPerBuffer));
	log->writeToLog(" block size: " + to_string(blockSize()));
	log->writeToLog(" calc buffer size: " + to_string(bsize));
	
	return true;
}

void ofxPd::clear() {
	
	// this seems hacky, but (so far) gets rid of a deadlock on Windows
	// which causes a hang on exit
	//
	// hopefully to be fixed for real at a future point ...
//	#ifndef TARGET_WIN32
//		lock();
//	#endif
	if(inBuffer != NULL) {
		delete[] inBuffer;
		inBuffer = NULL;
	}
	PdContext::instance().clear();
//	#ifndef TARGET_WIN32
//		unlock();
//	#endif
	unsubscribeAll();

	channels.clear();

	// add default global channel
	Channel c;
	channels.insert(make_pair(-1, c));
	
	ticks = 0;
	bsize = 0;
	srate = 0;
	inChannels = 0;
	outChannels = 0;
}

//--------------------------------------------------------------------
void ofxPd::addToSearchPath(const std::string& path) {
    string fullpath = "/Users/shawn/";
	//FIXME string fullpath = ofFilePath::getAbsolutePath(ofToDataPath(path));
	//FIXME juce::Logger::outputDebugString("Pd") << "adding search path: "+fullpath;
	PdBase::addToSearchPath(fullpath.c_str());
}

void ofxPd::clearSearchPath() {
	log->writeToLog("clearing search paths");
	PdBase::clearSearchPath();
}

//--------------------------------------------------------------------
Patch ofxPd::openPatch(const std::string& patch) {

	//FIXME string fullpath = ofFilePath::getAbsolutePath(ofToDataPath(patch));
	//FIXME string file = ofFilePath::getFileName(fullpath);
	//FIXME string folder = ofFilePath::getEnclosingDirectory(fullpath);
	
	string fullpath = "/Users/shawn/testPatch.pd"; //FIXME
	string file = "testPatch.pd"; //FIXME
	string folder = "/Users/shawn/"; //FIXME 
	
	// trim the trailing slash Poco::Path always adds ... blarg
	if(folder.size() > 0 && folder.at(folder.size()-1) == '/') {
		folder.erase(folder.end()-1);
	}
	
	log->writeToLog("opening patch: "+file+" path: "+folder);

	// [; pd open file folder(
	Patch p = PdBase::openPatch(file.c_str(), folder.c_str());
 	if(!p.isValid()) {
		log->writeToLog("opening patch \""+file+"\" failed");
	}
	
	return p;
}

pd::Patch ofxPd::openPatch(pd::Patch& patch) {
    log->writeToLog("##opening patch: "+patch.filename()+" path: "+patch.path());
	Patch p = PdBase::openPatch(patch);
	if(!p.isValid()) {
		log->writeToLog("opening patch \""+patch.filename()+"\" failed");
	}
	return p;
}

void ofxPd::closePatch(const std::string& patch) {
	log->writeToLog("closing path: "+patch);
	PdBase::closePatch(patch);
}

void ofxPd::closePatch(Patch& patch) {
	log->writeToLog("closing patch: "+patch.filename());
	PdBase::closePatch(patch);
}	

//--------------------------------------------------------------------
void ofxPd::computeAudio(bool state) {
	if(state) {
		log->writeToLog("audio processing on");
	}
	else {
		log->writeToLog("audio processing off");
	}
	computing = state;

	// [; pd dsp $1(
	PdBase::computeAudio(state);
}
void ofxPd::start() {
	// [; pd dsp 1(
	computeAudio(true);
}

void ofxPd::stop() {	
	// [; pd dsp 0(
	computeAudio(false);
}

//----------------------------------------------------------
void ofxPd::subscribe(const std::string& source) {
	if(exists(source)) {
		//FIXME juce::Logger::outputDebugString("Pd") << "subscribe: ignoring duplicate source";
		return;
	}
	PdBase::subscribe(source);
	Source s;
	sources.insert(pair<string,Source>(source, s));
}

void ofxPd::unsubscribe(const std::string& source) {
	map<string,Source>::iterator iter;
	iter = sources.find(source);
	if(iter == sources.end()) {
		log->writeToLog("unsubscribe: ignoring unknown source");
		return;
	}
	PdBase::unsubscribe(source);
	sources.erase(iter);
}

bool ofxPd::exists(const std::string& source) {
	if(sources.find(source) != sources.end()) {
		return true;
	}
	return false;
}

void ofxPd::unsubscribeAll(){

    PdBase::unsubscribeAll();
	sources.clear();

	// add default global source
	Source s;
	sources.insert(make_pair("", s));
}

//--------------------------------------------------------------------
void ofxPd::addReceiver(PdReceiver& receiver) {
	
	pair<set<PdReceiver*>::iterator, bool> ret;
	ret = receivers.insert(&receiver);
	if(!ret.second) {
		log->writeToLog("addReceiver: ignoring duplicate receiver");
		return;
	}

	// set PdBase receiver on adding first reciever
	if(receivers.size() == 1) {
		PdBase::setReceiver(this);
	}

	// receive from all sources by default
	receiveSource(receiver);
}

void ofxPd::removeReceiver(PdReceiver& receiver) {
	
	// exists?
	set<PdReceiver*>::iterator r_iter;
	r_iter = receivers.find(&receiver);
	if(r_iter == receivers.end()) {
		log->writeToLog("removeReceiver: ignoring unknown receiver");
		return;
	}
	receivers.erase(r_iter);
	
	// clear PdBase receiver on removing last reciever
	if(receivers.size() == 0) {
		PdBase::setReceiver(NULL);
	}

	// remove from all sources
	ignoreSource(receiver);
}

bool ofxPd::receiverExists(PdReceiver& receiver) {
	if(receivers.find(&receiver) != receivers.end())
		return true;
	return false;
}

void ofxPd::clearReceivers() {

	receivers.clear();
	
	map<string,Source>::iterator iter;
	for(iter = sources.begin(); iter != sources.end(); ++iter) {
		iter->second.receivers.clear();
	}
	
	PdBase::setReceiver(NULL);
}

//----------------------------------------------------------
int ofxPd::ticksPerBuffer() {
	return ticks;
}

int ofxPd::bufferSize() {
	return bsize;
}

int ofxPd::sampleRate() {
	return srate;
}

int ofxPd::numInChannels() {
	return inChannels;
}

int ofxPd::numOutChannels() {
	return outChannels;
}

bool ofxPd::isComputingAudio() {
	return computing;
}

//----------------------------------------------------------
void ofxPd::receiveSource(PdReceiver& receiver, const std::string& source) {

	if(!receiverExists(receiver)) {
		log->writeToLog("receive: unknown receiver, call addReceiver first");
		return;
	}
	
	if(!exists(source)) {
		log->writeToLog("receive: unknown source, call subscribe first");
		return;
	}
	
	// global source (all sources)
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	
	// subscribe to specific source
	if(source != "") {
	
		// make sure global source is ignored
		if(g_iter->second.receiverExists(&receiver)) {
			g_iter->second.removeReceiver(&receiver);
		}
		
		// receive from specific source
		map<string,Source>::iterator s_iter;
		s_iter = sources.find(source);
		s_iter->second.addReceiver(&receiver);
	}
	else {
		// make sure all sources are ignored
		ignoreSource(receiver);
	
		// receive from the global source
		g_iter->second.addReceiver(&receiver);
	}
}

void ofxPd::ignoreSource(PdReceiver& receiver, const std::string& source) {

	if(!receiverExists(receiver)) {
		log->writeToLog("ignore: ignoring unknown receiver");
		return;
	}

	if(!exists(source)) {
		log->writeToLog("ignore: ignoring unknown source");
		return;
	}
	
	map<string,Source>::iterator s_iter;
	
	// unsubscribe from specific source
	if(source != "") {
	
		// global source (all sources)
		map<string,Source>::iterator g_iter;
		g_iter = sources.find("");
	
		// negation from global
		if(g_iter->second.receiverExists(&receiver)) {
			
			// remove from global
			g_iter->second.removeReceiver(&receiver);
			
			// add to *all* other sources
			for(s_iter = sources.begin(); s_iter != sources.end(); ++s_iter) {
				if(s_iter != g_iter) {
					s_iter->second.addReceiver(&receiver);
				}
			}
		}
		
		// remove from source
		s_iter = sources.find(source);
		s_iter->second.removeReceiver(&receiver);
	}
	else {	// ignore all sources	
		for(s_iter = sources.begin(); s_iter != sources.end(); ++s_iter) {
			s_iter->second.removeReceiver(&receiver);
		}
	}
}

bool ofxPd::isReceivingSource(PdReceiver& receiver, const std::string& source) {
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(source);
	if(s_iter != sources.end() && s_iter->second.receiverExists(&receiver)) {
		return true;
	}
	return false;
}

//----------------------------------------------------------
void ofxPd::addMidiReceiver(PdMidiReceiver& receiver) {

	pair<set<PdMidiReceiver*>::iterator, bool> ret;
	ret = midiReceivers.insert(&receiver);
	if(!ret.second) {
		log->writeToLog("addMidiReceiver: ignoring duplicate receiver");
		return;
	}

	// set PdBase receiver on adding first reciever
	if(midiReceivers.size() == 1) {
		PdBase::setMidiReceiver(this);
	}

	// receive from all channels by default
	receiveMidiChannel(receiver);
}

void ofxPd::removeMidiReceiver(PdMidiReceiver& receiver) {

	// exists?
	set<PdMidiReceiver*>::iterator r_iter;
	r_iter = midiReceivers.find(&receiver);
	if(r_iter == midiReceivers.end()) {
		log->writeToLog("removeMidiReceiver: ignoring unknown receiver");
		return;
	}
	midiReceivers.erase(r_iter);
	
	// clear PdBase receiver on removing last reciever
	if(receivers.size() == 0) {
		PdBase::setMidiReceiver(NULL);
	}

	// remove from all sources
	ignoreMidiChannel(receiver);
}

bool ofxPd::midiReceiverExists(PdMidiReceiver& receiver) {
	if(midiReceivers.find(&receiver) != midiReceivers.end()) {
		return true;
	}
	return false;
}

void ofxPd::clearMidiReceivers() {

	midiReceivers.clear();
	
	map<int,Channel>::iterator iter;
	for(iter = channels.begin(); iter != channels.end(); ++iter) {
		iter->second.receivers.clear();
	}

	PdBase::setMidiReceiver(NULL);
}

//----------------------------------------------------------
void ofxPd::receiveMidiChannel(PdMidiReceiver& receiver, int channel) {

	if(!midiReceiverExists(receiver)) {
		log->writeToLog("receiveMidi: unknown receiver, call addMidiReceiver first");
		return;
	}

	// handle bad channel numbers
	if(channel < 0) {
		channel = 0;
	}

	// insert channel if it dosen't exist yet
	if(channels.find(channel) == channels.end()) {
		Channel c;
		channels.insert(pair<int,Channel>(channel, c));
	}

	// global channel (all channels)
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	
	// subscribe to specific channel
	if(channel != 0) {
	
		// make sure global channel is ignored
		if(g_iter->second.midiReceiverExists(&receiver)) {
			g_iter->second.removeMidiReceiver(&receiver);
		}
		
		// receive from specific channel
		map<int,Channel>::iterator c_iter;
		c_iter = channels.find(channel);
		c_iter->second.addMidiReceiver(&receiver);
	}
	else {
		// make sure all channels are ignored
		ignoreMidiChannel(receiver);

		// receive from the global channel
		g_iter->second.addMidiReceiver(&receiver);
	}
}

void ofxPd::ignoreMidiChannel(PdMidiReceiver& receiver, int channel) {

	if(!midiReceiverExists(receiver)) {
		log->writeToLog("ignoreMidi: ignoring unknown receiver");
		return;
	} 

	// handle bad channel numbers
	if(channel < 0) {
		channel = 0;
	}

	// insert channel if it dosen't exist yet
	if(channels.find(channel) == channels.end()) {
		Channel c;
		channels.insert(pair<int,Channel>(channel, c));
	}

	map<int,Channel>::iterator c_iter;
	
	// unsubscribe from specific channel
	if(channel != 0) {
	
		// global channel (all channels)
		map<int,Channel>::iterator g_iter;
		g_iter = channels.find(0);
	
		// negation from global
		if(g_iter->second.midiReceiverExists(&receiver)) {
			
			// remove from global
			g_iter->second.removeMidiReceiver(&receiver);
			
			// add to *all* other channels
			for(c_iter = channels.begin(); c_iter != channels.end(); ++c_iter) {
				if(c_iter != g_iter) {
					c_iter->second.addMidiReceiver(&receiver);
				}
			}
		}
		
		// remove from channel
		c_iter = channels.find(channel);
		c_iter->second.removeMidiReceiver(&receiver);
	}
	else {	// ignore all sources	
		for(c_iter = channels.begin(); c_iter != channels.end(); ++c_iter) {
			c_iter->second.removeMidiReceiver(&receiver);
		}
	}
}

bool ofxPd::isReceivingMidiChannel(PdMidiReceiver& receiver, int channel) {

	// handle bad channel numbers
	if(channel < 0) {
		channel = 0;
	}

	map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end() && c_iter->second.midiReceiverExists(&receiver))
		return true;
	return false;
}

//----------------------------------------------------------
void ofxPd::sendNoteOn(const int channel, const int pitch, const int velocity) {
	PdBase::sendNoteOn(channel-1, pitch, velocity);
}

void ofxPd::sendControlChange(const int channel, const int control, const int value) {
	PdBase::sendControlChange(channel-1, control, value);
}

void ofxPd::sendProgramChange(const int channel, int program) {
	PdBase::sendProgramChange(channel-1, program-1);
}

void ofxPd::sendPitchBend(const int channel, const int value) {
	PdBase::sendPitchBend(channel-1, value);
}

void ofxPd::sendAftertouch(const int channel, const int value) {
	PdBase::sendAftertouch(channel-1, value);
}

void ofxPd::sendPolyAftertouch(const int channel, int pitch, int value) {
	PdBase::sendPolyAftertouch(channel-1, pitch, value);
}

//----------------------------------------------------------
void ofxPd::audioIn(float* input, int bufferSize, int nChannels) {
	try {
		if(inBuffer != NULL) {
			if(bufferSize != bsize || nChannels != inChannels) {
				ticks = bufferSize/blockSize();
				bsize = bufferSize;
				inChannels = nChannels;
				log->writeToLog("buffer size or num input channels updated");
				init(outChannels, inChannels, srate, ticks, isQueued());
				PdBase::computeAudio(computing);
			}
			memcpy(inBuffer, input, bufferSize*nChannels*sizeof(float));
		}
	}
	catch (...) {
		log->writeToLog("could not copy input buffer");
	}
}

void ofxPd::audioOut(float* output, int bufferSize, int nChannels) {
	if(inBuffer != NULL) {
		if(bufferSize != bsize || nChannels != outChannels) {
			ticks = bufferSize/blockSize();
			bsize = bufferSize;
			outChannels = nChannels;
			log->writeToLog("buffer size or num output channels updated");
			init(outChannels, inChannels, srate, ticks, isQueued());
			PdBase::computeAudio(computing);
		}
        if(!PdBase::processFloat(ticks, inBuffer, output)) {
			log->writeToLog("could not process output buffer");
		}
	}
}

/* ***** PROTECTED ***** */

//----------------------------------------------------------
void ofxPd::print(const std::string& message) {

	log->writeToLog("print: " +  message);

	// broadcast
	set<PdReceiver*>::iterator iter;
	for(iter = receivers.begin(); iter != receivers.end(); ++iter) {
		(*iter)->print(message);
	}
}

void ofxPd::receiveBang(const std::string& dest) {

	log->writeToLog("bang: " + dest);
	
	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* r_set;

	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveBang(dest);
	}
	
	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(dest);
	r_set = &s_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveBang(dest);
	}
}

void ofxPd::receiveFloat(const std::string& dest, float value) {

	log->writeToLog("float: " + dest + " " + to_string(value));

	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* r_set;

	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveFloat(dest, value);
	}

	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(dest);
	r_set = &s_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveFloat(dest, value);
	}
}

void ofxPd::receiveSymbol(const std::string& dest, const std::string& symbol) {

	log->writeToLog("symbol: " + dest + " " + symbol);

	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* r_set;

	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveSymbol(dest, (string) symbol);
	}

	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(dest);
	r_set = &s_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveSymbol(dest, (string) symbol);
	}
}

void ofxPd::receiveList(const std::string& dest, const List& list) {

	log->writeToLog("list: " + dest + " " + list.toString());

	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* r_set;

	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveList(dest, list);
	}

	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(dest);
	r_set = &s_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveList(dest, list);
	}
}

void ofxPd::receiveMessage(const std::string& dest, const std::string& msg, const List& list) {

	//FIXME juce::Logger::outputDebugString("Pd") << "message: " << dest << " " << msg << " " << list.toString();

	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* r_set;

	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveMessage(dest, msg, list);
	}

	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(dest);
	r_set = &s_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveMessage(dest, msg, list);
	}
}

//----------------------------------------------------------
void ofxPd::receiveNoteOn(const int channel, const int pitch, const int velocity) {

	//FIXME juce::Logger::outputDebugString("Pd") << "note on: " << channel+1 << " " << pitch << " " << velocity;

	set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* r_set;

	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveNoteOn(channel+1, pitch, velocity);
	}

	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end()) {
		r_set = &c_iter->second.receivers;
		for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
			(*r_iter)->receiveNoteOn(channel+1, pitch, velocity);
		}
	}
}

void ofxPd::receiveControlChange(const int channel, const int controller, const int value) {

	//FIXME juce::Logger::outputDebugString("Pd") << "control change: " << channel+1 << " " << controller << " " << value;

	set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* r_set;
 
	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveControlChange(channel+1, controller, value);
	}

	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end()) {
		r_set = &c_iter->second.receivers;
		for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
			(*r_iter)->receiveControlChange(channel+1, controller, value);
		}
	}
}

void ofxPd::receiveProgramChange(const int channel, const int value) {

	//FIXME juce::Logger::outputDebugString("Pd") << "program change: " << channel+1 << " " << value+1;

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* r_set;

	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveProgramChange(channel+1, value+1);
	}

	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end()) {
		r_set = &c_iter->second.receivers;
		for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
			(*r_iter)->receiveProgramChange(channel+1, value+1);
		}
	}
}

void ofxPd::receivePitchBend(const int channel, const int value) {

	//FIXME juce::Logger::outputDebugString("Pd") << "pitch bend: " << channel+1 << value;

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* r_set;

	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receivePitchBend(channel+1, value);
	}

	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end()) {
		r_set = &c_iter->second.receivers;
		for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
			(*r_iter)->receivePitchBend(channel+1, value);
		}
	}
}

void ofxPd::receiveAftertouch(const int channel, const int value) {

	//FIXME juce::Logger::outputDebugString("Pd") << "aftertouch: " << channel+1 << value;

	set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* r_set;

	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receiveAftertouch(channel+1, value);
	}

	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end()) {
		r_set = &c_iter->second.receivers;
		for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
			(*r_iter)->receiveAftertouch(channel+1, value);
		}
	}
}

void ofxPd::receivePolyAftertouch(const int channel, const int pitch, const int value) {

	//FIXME juce::Logger::outputDebugString("Pd") << "poly aftertouch: " << channel+1 << " " << pitch << " " << value;

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* r_set;

	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	r_set = &g_iter->second.receivers;
	for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
		(*r_iter)->receivePolyAftertouch(channel+1, pitch, value);
	}

	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end()) {
		r_set = &c_iter->second.receivers;
		for(r_iter = r_set->begin(); r_iter != r_set->end(); ++r_iter) {
			(*r_iter)->receivePolyAftertouch(channel+1, pitch, value);
		}
	}
}

void ofxPd::receiveMidiByte(const int port, const int byte) {

	//FIXME juce::Logger::outputDebugString("Pd") << "midi byte: " << port << " " << byte;

	set<PdMidiReceiver*>& r_set = midiReceivers;
	set<PdMidiReceiver*>::iterator iter;
	for(iter = r_set.begin(); iter != r_set.end(); ++iter) {
		(*iter)->receiveMidiByte(port, byte);
	}
}

