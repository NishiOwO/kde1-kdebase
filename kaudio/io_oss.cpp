#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <iostream.h>

// Linux/OSS includes
#ifdef linux
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>  // Here UNIX_SOUND_SYSTEM gets defined
#define OSS_AUDIO
#endif

#include "maudio.h"
#include "sample.h"
#include "io_oss.h"

extern int          BUFFSIZE;   // Crap! Must be out into AudioSample class



AudioDev::AudioDev(int devnum)
{
  opened=false;
  char textDevDSP[]="/dev/dsp";
  int  lenDevDSP = strlen(textDevDSP);

  bit_p_spl = 0; // This forces setting parameter new in "grab"

  if ( devnum > 10 || devnum<=0 ) {
    // Illegal or default device number: Use default DSP
    devname = new char[lenDevDSP+1];
    strcpy(devname,textDevDSP);  // Yields "/dev/dsp"
  }
  else {
    // Yields "/dev/dspX", with X in [0..9].
    devname = new char[lenDevDSP+2];
    strcpy(devname,textDevDSP);
    textDevDSP[lenDevDSP] = char('0'+(unsigned char)devnum);
    textDevDSP[lenDevDSP] = 0; // 0-termination of string
  }

 
  // Adding a quick hack. Trying out what happens if I feed OSS with
  // Zero-Data when idle. Is OSS perhaps happy then?!?
  silence8 = new char[BUFFER_MAX];
  silence16= new char[BUFFER_MAX];
  for (int i=0; i<BUFFER_MAX; i++) {
    silence8[i]=0x80;
    silence16[i]=0x00;
  }
}

void AudioDev::setBugs(int bugs)
{
  BugFlags = bugs;
}


bool AudioDev::grab()
{
  if (ParamsChanged) {
    // When playback parameters are changed with setParams(), we have to
    // re-open the device.
    release();
  }
    
  if (!opened) {
      /* Open audio device non-blocking! */
#ifdef OSS_AUDIO
      audiodev=open(devname, O_WRONLY, 0 /* O_NONBLOCK */);
#else
      audiodev=-1; // fail!
#endif 
     if (audiodev == -1 ) {
       cerr << "maudio: Cannot open audio device.\n";
       return false;
     }
     else {
       /*
	* Now set sample format, then channels, then speed. It is important to follow this
	* scheme. See OSS documentation for more info.
	*/
#ifdef OSS_AUDIO
       
       // Set fragments
       int arg;
       if (bit_p_spl==8)
	 arg = 0x00100000; // 0xMMMMSSSS;
       else
	 arg = 0x00200000; //

       // now calc count=ld(BUFFSIZE);
       int tmp   = BUFFSIZE;
       int count = 0;
       while (tmp>1) {
	 tmp /=2;
	 count++;
       }
       arg |= count;
//       cerr << "Count=" << count <<'\n';

       if (ioctl(audiodev, SNDCTL_DSP_SETFRAGMENT, &arg)) {
	 release();
	 return false;
       }
       
       int Param;  /* For calling ioctl() */
       Param= bit_p_spl ; ioctl(audiodev, SNDCTL_DSP_SAMPLESIZE , &Param);
       Param= stereo    ; ioctl(audiodev, SNDCTL_DSP_STEREO     , &Param);
       Param= frequency ; ioctl(audiodev, SNDCTL_DSP_SPEED      , &Param);
#endif
       opened = true;
       ParamsChanged=false;
     }
  }
  return opened;
}

bool AudioDev::release()
{
  if (opened) {
//  if ( BugFlags & CLOSELOOSE_BUG)
//  Loose data on close? Then sync before close

    sync(); // Always sync() on close. I am fed up with OSS-Bug compatibility mode :-(
    close(audiodev);
    opened=false;
  }

  return true;
}

bool AudioDev::reset()
{
  if (opened)
    {
#ifdef DEBUG
      cerr << "RESET!\n";
#endif

#ifdef OSS_AUDIO
      sync();
      return( ioctl(audiodev, SNDCTL_DSP_RESET, 0) );
#else
      return(true);
#endif
    }
  else
    return true;
}

bool AudioDev::sync()
{
  if (opened)
    {
#ifdef DEBUG
      cerr << "SYNC!\n";
#endif
#ifdef OSS_AUDIO
      return( ioctl(audiodev, SNDCTL_DSP_SYNC, 0) );
#else
      return(true);
#endif
    }
  else
    return true;
}

bool AudioDev::pause()
{
  if (opened) {

#ifdef OSS_AUDIO
    if( (BugFlags&POST_BUG) == true )
      // Do not use POST if POST is buggy
      return true;
    else
      return( ioctl(audiodev, SNDCTL_DSP_POST, 0) );
#else
    return true;
#endif
  }
  else
    return true;
}

void AudioDev::setParams(int8 Bit_p_spl, bool Stereo, uint32 Freq, int8 Channels)
{
  // See if any params were changed since the last call. If yes, then we have
  // to setup the sound device in AudioDev::grab()
  ParamsChanged = (    (bit_p_spl != Bit_p_spl)
		    || (stereo    != Stereo   )
		    || (frequency != Freq     )
		    || (channels  != Channels ));

  bit_p_spl = Bit_p_spl;
  stereo    = Stereo;
  frequency = Freq;
  channels  = Channels;
}

int AudioDev::Write(AudioSample *a)
{
#ifdef OSS_AUDIO
  return ( write(audiodev, a->Buffer, a->BuferValidLength) );
#else
  AudioSample *dummy = a;  // remove warnings on unsupported systems
  a = dummy;               // remove warnings on unsupported systems
  return true;
#endif
}

int AudioDev::emitSilence()
{
#ifdef OSS_AUDIO
  audio_buf_info ABI;
  ioctl(audiodev,SNDCTL_DSP_GETOSPACE, &ABI);
  if (ABI.fragments >= 8)
    {
      // Emit silence, if there may be sound underrun
#ifdef DEBUG
      cerr  << "FreeFrags=" << ABI.fragments << '\n';
#endif
      if (bit_p_spl == 8)
	return write(audiodev, silence8, BUFFSIZE );
      else
	return write(audiodev, silence16, BUFFSIZE );
    }
#else
  return true;
#endif
  return true;
}
