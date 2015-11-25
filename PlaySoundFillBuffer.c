/*	File:		PlaySoundFillBuffer.c		Description: Play Sound shows how to use the Sound Description Extention atom information with the				 SoundConverterFillBuffer APIs to play VBR and Non-VBR MP3 files. It will also play				 .aiff files using various encoding methods and .wav files.				 NOTE: This Sample requires CarbonLib 1.1 or above.	Author:		mc, era	Copyright: 	� Copyright 2000 Apple Computer, Inc. All rights reserved.		Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.				("Apple") in consideration of your agreement to the following terms, and your				use, installation, modification or redistribution of this Apple software				constitutes acceptance of these terms.  If you do not agree with these terms,				please do not use, install, modify or redistribute this Apple software.				In consideration of your agreement to abide by the following terms, and subject				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s				copyrights in this original Apple software (the "Apple Software"), to use,				reproduce, modify and redistribute the Apple Software, with or without				modifications, in source and/or binary forms; provided that if you redistribute				the Apple Software in its entirety and without modifications, you must retain				this notice and the following text and disclaimers in all such redistributions of				the Apple Software.  Neither the name, trademarks, service marks or logos of				Apple Computer, Inc. may be used to endorse or promote products derived from the				Apple Software without specific prior written permission from Apple.  Except as				expressly stated in this notice, no other rights or licenses, express or implied,				are granted by Apple herein, including but not limited to any patent rights that				may be infringed by your derivative works or by other works in which the Apple				Software may be incorporated.				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN				COMBINATION WITH YOUR PRODUCTS.				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.					Change History (most recent first): <2> 10/31/00 modified to use SoundConverterFillBuffer APIs										<1> 7/26/00 initial release as PlaySound.c*/#include "MP3Player.h"// globalsBoolean gBufferDone;// * ----------------------------// MySoundCallBackFunction//// used to signal when a buffer is done playingstatic pascal void MySoundCallBackFunction(SndChannelPtr theChannel, SndCommand *theCmd){#pragma unused(theChannel)		#ifndef TARGET_API_MAC_CARBON		#if !GENERATINGCFM			long oldA5;			oldA5 = SetA5(theCmd->param2);		#else			#pragma unused(theCmd)		#endif	#else		#pragma unused(theCmd)	#endif // TARGET_API_MAC_CARBON	gBufferDone = true;	#ifndef TARGET_API_MAC_CARBON		#if !GENERATINGCFM			oldA5 = SetA5(oldA5);		#endif	#endif // TARGET_API_MAC_CARBON}// * ----------------------------// SoundConverterFillBufferDataProc//// the callback routine that provides the source data for conversion - it provides data by setting// outData to a pointer to a properly filled out ExtendedSoundComponentData structurestatic pascal Boolean SoundConverterFillBufferDataProc(SoundComponentDataPtr *outData, void *inRefCon){	SCFillBufferDataPtr pFillData = (SCFillBufferDataPtr)inRefCon;		OSErr err = noErr;								// if after getting the last chunk of data the total time is over the duration, we're done	if (pFillData->getMediaAtThisTime >= pFillData->sourceDuration) {		pFillData->isThereMoreSource = false;		pFillData->compData.desc.buffer = NULL;		pFillData->compData.desc.sampleCount = 0;		pFillData->compData.bufferSize = 0;	}		if (pFillData->isThereMoreSource) {			long		sourceBytesReturned;		long		numberOfSamples;		TimeValue	sourceReturnedTime, durationPerSample;									HUnlock(pFillData->hSource);		err = GetMediaSample(pFillData->sourceMedia,		// specifies the media for this operation							 pFillData->hSource,			// function returns the sample data into this handle							 pFillData->maxBufferSize,		// maximum number of bytes of sample data to be returned							 &sourceBytesReturned,			// the number of bytes of sample data returned							 pFillData->getMediaAtThisTime,	// starting time of the sample to be retrieved (must be in Media's TimeScale)							 &sourceReturnedTime,			// indicates the actual time of the returned sample data							 &durationPerSample,			// duration of each sample in the media							 NULL,							// sample description corresponding to the returned sample data (NULL to ignore)							 NULL,							// index value to the sample description that corresponds to the returned sample data (NULL to ignore)							 0,								// maximum number of samples to be returned (0 to use a value that is appropriate for the media)							 &numberOfSamples,				// number of samples it actually returned							 NULL);							// flags that describe the sample (NULL to ignore)							 		HLock(pFillData->hSource);		if ((noErr != err) || (sourceBytesReturned == 0)) {			pFillData->isThereMoreSource = false;			pFillData->compData.desc.buffer = NULL;			pFillData->compData.desc.sampleCount = 0;								if ((err != noErr) && (sourceBytesReturned > 0))				DebugStr("\pGetMediaSample - Failed in FillBufferDataProc");		}				pFillData->getMediaAtThisTime = sourceReturnedTime + (durationPerSample * numberOfSamples);		pFillData->compData.bufferSize = sourceBytesReturned; 	}	// set outData to a properly filled out ExtendedSoundComponentData struct	*outData = (SoundComponentDataPtr)&pFillData->compData;		return (pFillData->isThereMoreSource);}// * ----------------------------// GetMovieMedia//// returns a Media identifier - if the file is a System 7 Sound a non-in-place import is done and// a handle to the data is passed back to the caller who is responsible for disposing of itstatic OSErr GetMovieMedia(const FSSpec *inFile, Media *outMedia, Handle *outHandle){	Movie theMovie = 0;	Track theTrack;	FInfo fndrInfo;	OSErr err = noErr;	err = FSpGetFInfo(inFile, &fndrInfo);	BailErr(err);		if (kQTFileTypeSystemSevenSound == fndrInfo.fdType) {		// if this is an 'sfil' handle it appropriately		// QuickTime can't import these files in place, but that's ok,		// we just need a new place to put the data				MovieImportComponent	theImporter = 0;		Handle 					hDataRef = NULL;				// create a new movie		theMovie = NewMovie(newMovieActive);				// allocate the data handle and create a data reference for this handle		// the caller is responsible for disposing of the data handle once done with the sound		*outHandle = NewHandle(0);		err = PtrToHand(outHandle, &hDataRef, sizeof(Handle));		if (noErr == err) {			SetMovieDefaultDataRef(theMovie, hDataRef, HandleDataHandlerSubType);			OpenADefaultComponent(MovieImportType, kQTFileTypeSystemSevenSound, &theImporter);			if (theImporter) {				Track		ignoreTrack;				TimeValue 	ignoreDuration;						long 		ignoreFlags;								err = MovieImportFile(theImporter, inFile, theMovie, 0, &ignoreTrack, 0, &ignoreDuration, 0, &ignoreFlags);				CloseComponent(theImporter);			}		} else {			if (*outHandle) {				DisposeHandle(*outHandle);				*outHandle = NULL;			}		}		if (hDataRef) DisposeHandle(hDataRef);		BailErr(err);	} else {		// import in place				short theRefNum;		short theResID = 0;	// we want the first movie		Boolean wasChanged;				// open the movie file		err = OpenMovieFile(inFile, &theRefNum, fsRdPerm);		BailErr(err);		// instantiate the movie		err = NewMovieFromFile(&theMovie, theRefNum, &theResID, NULL, newMovieActive, &wasChanged);		CloseMovieFile(theRefNum);		BailErr(err);	}			// get the first sound track	theTrack = GetMovieIndTrackType(theMovie, 1, SoundMediaType, movieTrackMediaType);	if (NULL == theTrack ) BailErr(invalidTrack);	// get and return the sound track media	*outMedia = GetTrackMedia(theTrack);	if (NULL == *outMedia) err = invalidMedia;	bail:	return err;}// * ----------------------------// MyGetSoundDescriptionExtension//// this function will extract the information needed to decompress the sound file, this includes // retrieving the sample description, the decompression atom and setting up the sound headerstatic OSErr MyGetSoundDescriptionExtension(Media inMedia, AudioFormatAtomPtr *outAudioAtom, CmpSoundHeaderPtr outSoundHeader){	OSErr err = noErr;				Size size;	Handle extension;			// Version 1 of this record includes four extra fields to store information about compression ratios. It also defines	// how other extensions are added to the SoundDescription record.	// All other additions to the SoundDescription record are made using QT atoms. That means one or more	// atoms can be appended to the end of the SoundDescription record using the standard [size, type]	// mechanism used throughout the QuickTime movie resource architecture.	// http://developer.apple.com/techpubs/quicktime/qtdevdocs/RM/frameset.htm	SoundDescriptionV1Handle sourceSoundDescription = (SoundDescriptionV1Handle)NewHandle(0);		// get the description of the sample data	GetMediaSampleDescription(inMedia, 1, (SampleDescriptionHandle)sourceSoundDescription);	BailErr(GetMoviesError());	extension = NewHandle(0);		// get the "magic" decompression atom	// This extension to the SoundDescription information stores data specific to a given audio decompressor.	// Some audio decompression algorithms require a set of out-of-stream values to configure the decompressor	// which are stored in a siDecompressionParams atom. The contents of the siDecompressionParams atom are dependent	// on the audio decompressor.	err = GetSoundDescriptionExtension((SoundDescriptionHandle)sourceSoundDescription, &extension, siDecompressionParams);		if (noErr == err) {		size = GetHandleSize(extension);		HLock(extension);		*outAudioAtom = (AudioFormatAtom*)NewPtr(size);		err = MemError();		// copy the atom data to our buffer...		BlockMoveData(*extension, *outAudioAtom, size);		HUnlock(extension);	} else {		// if it doesn't have an atom, that's ok		err = noErr;	}		// set up our sound header	outSoundHeader->format = (*sourceSoundDescription)->desc.dataFormat;	outSoundHeader->numChannels = (*sourceSoundDescription)->desc.numChannels;	outSoundHeader->sampleSize = (*sourceSoundDescription)->desc.sampleSize;	outSoundHeader->sampleRate = (*sourceSoundDescription)->desc.sampleRate;		DisposeHandle(extension);	DisposeHandle((Handle)sourceSoundDescription);	bail:	return err;}// * ----------------------------// PlaySound//// this function does the actual work of playing the sound file, it sets up the sound converter environment, allocates play buffers,// creates the sound channel and sends the appropriate sound commands to play the converted sound dataOSErr PlaySound(const FSSpec *inFileToPlay){		AudioCompressionAtomPtr	theDecompressionAtom;	SoundComponentData		theInputFormat,							theOutputFormat;	SoundConverter			mySoundConverter = NULL;	CmpSoundHeader			mySndHeader0,							mySndHeader1;	SCFillBufferData 		scFillBufferData = {NULL};	Media					theSoundMedia = NULL;	Ptr						pSourceBuffer = NULL;	Ptr						pDecomBuffer0 = NULL,							pDecomBuffer1 = NULL;								Handle					hSys7SoundData = NULL;							Boolean					isSoundDone = false;		OSErr 					err = noErr;		err = GetMovieMedia(inFileToPlay, &theSoundMedia, &hSys7SoundData);	BailErr(err);		err = MyGetSoundDescriptionExtension(theSoundMedia, (AudioFormatAtomPtr *)&theDecompressionAtom, &mySndHeader0);	if (noErr == err) {			// setup input/output format for sound converter		theInputFormat.flags = 0;		theInputFormat.format = mySndHeader0.format;		theInputFormat.numChannels = mySndHeader0.numChannels;		theInputFormat.sampleSize = mySndHeader0.sampleSize;		theInputFormat.sampleRate = mySndHeader0. sampleRate;		theInputFormat.sampleCount = 0;		theInputFormat.buffer = NULL;		theInputFormat.reserved = 0;		theOutputFormat.flags = 0;		theOutputFormat.format = kSoundNotCompressed;		theOutputFormat.numChannels = theInputFormat.numChannels;		theOutputFormat.sampleSize = theInputFormat.sampleSize;		theOutputFormat.sampleRate = theInputFormat.sampleRate;		theOutputFormat.sampleCount = 0;		theOutputFormat.buffer = NULL;		theOutputFormat.reserved = 0;		err = SoundConverterOpen(&theInputFormat, &theOutputFormat, &mySoundConverter);		BailErr(err);		// set up the sound converters decompresson 'environment' by passing in the 'magic' decompression atom		err = SoundConverterSetInfo(mySoundConverter, siDecompressionParams, theDecompressionAtom);		if (siUnknownInfoType == err) {			// clear this error, the decompressor didn't			// need the decompression atom and that's OK			err = noErr;		} else BailErr(err);				UInt32	targetBytes = 32768,				outputBytes = 0;				// find out how much buffer space to alocate for our output buffers		// The differences between SoundConverterConvertBuffer begin with how the buffering is done. SoundConverterFillBuffer will do as much or as		// little work as is required to satisfy a given request. This means that you can pass in buffers of any size you like and expect that		// the Sound Converter will never overflow the output buffer. SoundConverterFillBufferDataProc function will be called as many times as		// necessary to fulfill a request. This means that the SoundConverterFillBufferDataProc routine is free to provide data in whatever chunk size		// it likes. Of course with both sides, the buffer sizes will control how many times you need to request data and there is a certain amount of		// overhead for each call. You will want to balance this against the performance you require. While a call to SoundConverterGetBufferSizes is		// not required by the SoundConverterFillBuffer function, it is useful as a guide for non-VBR formats		do {			UInt32 inputFrames, inputBytes;			targetBytes *= 2;			err = SoundConverterGetBufferSizes(mySoundConverter, targetBytes, &inputFrames, &inputBytes, &outputBytes);		} while (notEnoughBufferSpace == err  && targetBytes < (MaxBlock() / 4));				// allocate play buffers		pDecomBuffer0 = NewPtr(outputBytes);		BailErr(MemError());				pDecomBuffer1 = NewPtr(outputBytes);		BailErr(MemError());		err = SoundConverterBeginConversion(mySoundConverter);		BailErr(err);		// setup first header		mySndHeader0.samplePtr = pDecomBuffer0;		mySndHeader0.numChannels = theOutputFormat.numChannels;		mySndHeader0.sampleRate = theOutputFormat.sampleRate;		mySndHeader0.loopStart = 0;		mySndHeader0.loopEnd = 0;		mySndHeader0.encode = cmpSH;					// compressed sound header encode value		mySndHeader0.baseFrequency = kMiddleC;		// mySndHeader0.AIFFSampleRate;					// this is not used		mySndHeader0.markerChunk = NULL;		mySndHeader0.format = theOutputFormat.format;		mySndHeader0.futureUse2 = 0;		mySndHeader0.stateVars = NULL;		mySndHeader0.leftOverSamples = NULL;		mySndHeader0.compressionID = fixedCompression;	// compression ID for fixed-sized compression, even uncompressed sounds use fixedCompression		mySndHeader0.packetSize = 0;					// the Sound Manager will figure this out for us		mySndHeader0.snthID = 0;		mySndHeader0.sampleSize = theOutputFormat.sampleSize;		mySndHeader0.sampleArea[0] = 0;					// no samples here because we use samplePtr to point to our buffer instead		// setup second header, only the buffer ptr is different		BlockMoveData(&mySndHeader0, &mySndHeader1, sizeof(mySndHeader0));		mySndHeader1.samplePtr = pDecomBuffer1;				// fill in struct that gets passed to SoundConverterFillBufferDataProc via the refcon		// this includes the ExtendedSoundComponentData record				scFillBufferData.sourceMedia = theSoundMedia;		scFillBufferData.getMediaAtThisTime = 0;				scFillBufferData.sourceDuration = GetMediaDuration(theSoundMedia);		scFillBufferData.isThereMoreSource = true;		scFillBufferData.maxBufferSize = kMaxInputBuffer;		scFillBufferData.hSource = NewHandle((long)scFillBufferData.maxBufferSize);		// allocate source media buffer		BailErr(MemError());		scFillBufferData.compData.desc = theInputFormat;		scFillBufferData.compData.desc.buffer = (Byte *)*scFillBufferData.hSource;		scFillBufferData.compData.desc.flags = kExtendedSoundData;		scFillBufferData.compData.recordSize = sizeof(ExtendedSoundComponentData);		scFillBufferData.compData.extendedFlags = kExtendedSoundSampleCountNotValid | kExtendedSoundBufferSizeValid;		scFillBufferData.compData.bufferSize = 0;				// setup the callback, create the sound channel and play the sound		// we will continue to convert the sound data into the free (non playing) buffer		SndCallBackUPP theSoundCallBackUPP = NewSndCallBackUPP(MySoundCallBackFunction);		SndChannelPtr pSoundChannel = NULL;		 				err = SndNewChannel(&pSoundChannel, sampledSynth, 0, theSoundCallBackUPP);		if (err == noErr) {						SndCommand			thePlayCmd0,								thePlayCmd1,								theCallBackCmd;			SndCommand	 		*pPlayCmd;			Ptr 				pDecomBuffer = NULL;			CmpSoundHeaderPtr 	pSndHeader = NULL;						UInt32 				outputFrames,								actualOutputBytes,								outputFlags;											eBufferNumber   	whichBuffer = kFirstBuffer;						SoundConverterFillBufferDataUPP theFillBufferDataUPP = NewSoundConverterFillBufferDataUPP(SoundConverterFillBufferDataProc);						thePlayCmd0.cmd = bufferCmd;			thePlayCmd0.param1 = 0;						// not used, but clear it out anyway just to be safe			thePlayCmd0.param2 = (long)&mySndHeader0;			thePlayCmd1.cmd = bufferCmd;			thePlayCmd1.param1 = 0;						// not used, but clear it out anyway just to be safe			thePlayCmd1.param2 = (long)&mySndHeader1;									whichBuffer = kFirstBuffer;					// buffer 1 will be free when callback runs			theCallBackCmd.cmd = callBackCmd;			theCallBackCmd.param2 = SetCurrentA5();						gBufferDone = true;						if (noErr == err) {				while (!isSoundDone && !Button()) {											if (gBufferDone == true) {						if (kFirstBuffer == whichBuffer) {							pPlayCmd = &thePlayCmd0;							pDecomBuffer = pDecomBuffer0;							pSndHeader = &mySndHeader0;							whichBuffer = kSecondBuffer;						} else {							pPlayCmd = &thePlayCmd1;							pDecomBuffer = pDecomBuffer1;							pSndHeader = &mySndHeader1;							whichBuffer = kFirstBuffer;						}						err = SoundConverterFillBuffer(mySoundConverter,		// a sound converter													   theFillBufferDataUPP,	// the callback UPP													   &scFillBufferData,		// refCon passed to FillDataProc													   pDecomBuffer,			// the decompressed data 'play' buffer													   outputBytes,				// size of the 'play' buffer													   &actualOutputBytes,		// number of output bytes													   &outputFrames,			// number of output frames													   &outputFlags);			// fillbuffer retured advisor flags						if (err) break;						if((outputFlags & kSoundConverterHasLeftOverData) == false) {							isSoundDone = true;						}												pSndHeader->numFrames = outputFrames;						gBufferDone = false;						if (!isSoundDone) {							SndDoCommand(pSoundChannel, &theCallBackCmd, true);	// reuse callBackCmd						}												SndDoCommand(pSoundChannel, pPlayCmd, true);			// play the next buffer					}				} // while			}						SoundConverterEndConversion(mySoundConverter, pDecomBuffer, &outputFrames, &outputBytes);			if (noErr == err && outputFrames) {				pSndHeader->numFrames = outputFrames;				SndDoCommand(pSoundChannel, pPlayCmd, true);	// play the last buffer.			}						if (theFillBufferDataUPP)				DisposeSoundConverterFillBufferDataUPP(theFillBufferDataUPP);		}				if (pSoundChannel)			err = SndDisposeChannel(pSoundChannel, false);		// wait until sounds stops playing before disposing of channel				if (theSoundCallBackUPP)			DisposeSndCallBackUPP(theSoundCallBackUPP);	}					bail:	if (scFillBufferData.hSource)		DisposeHandle(scFillBufferData.hSource);			if (mySoundConverter)		SoundConverterClose(mySoundConverter);			if (pDecomBuffer0)		DisposePtr(pDecomBuffer0);			if (pDecomBuffer1)		DisposePtr(pDecomBuffer1);			if (hSys7SoundData)		DisposeHandle(hSys7SoundData);	return err;}