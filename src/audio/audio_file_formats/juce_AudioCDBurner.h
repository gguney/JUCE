/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-10 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#ifndef __JUCE_AUDIOCDBURNER_JUCEHEADER__
#define __JUCE_AUDIOCDBURNER_JUCEHEADER__

#if JUCE_USE_CDBURNER

#include "juce_AudioFormatReader.h"
#include "../audio_sources/juce_AudioSource.h"


//==============================================================================
/**
*/
class AudioCDBurner     : public ChangeBroadcaster
{
public:
    //==============================================================================
    /** Returns a list of available optical drives.

        Use openDevice() to open one of the items from this list.
    */
    static const StringArray findAvailableDevices();

    /** Tries to open one of the optical drives.

        The deviceIndex is an index into the array returned by findAvailableDevices().
    */
    static AudioCDBurner* openDevice (const int deviceIndex);

    /** Destructor. */
    ~AudioCDBurner();

    //==============================================================================
    enum DiskState
    {
        unknown,                /**< An error condition, if the device isn't responding. */
        trayOpen,               /**< The drive is currently open. Note that a slot-loading drive
                                     may seem to be permanently open. */
        noDisc,                 /**< The drive has no disk in it. */
        writableDiskPresent,    /**< The drive contains a writeable disk. */
        readOnlyDiskPresent     /**< The drive contains a read-only disk. */
    };

    /** Returns the current status of the device.

        To get informed when the drive's status changes, attach a ChangeListener to
        the AudioCDBurner.
    */
    DiskState getDiskState() const;

    /** Returns true if there's a writable disk in the drive. */
    bool isDiskPresent() const;

    /** Sends an eject signal to the drive.
        The eject will happen asynchronously, so you can use getDiskState() and
        waitUntilStateChange() to monitor its progress.
    */
    bool openTray();

    /** Blocks the current thread until the drive's state changes, or until the timeout expires.
        @returns    the device's new state
    */
    DiskState waitUntilStateChange (int timeOutMilliseconds);

    //==============================================================================
    /** Returns the set of possible write speeds that the device can handle.
        These are as a multiple of 'normal' speed, so e.g. '24x' returns 24, etc.
        Note that if there's no media present in the drive, this value may be unavailable!
        @see setWriteSpeed, getWriteSpeed
    */
    const Array<int> getAvailableWriteSpeeds() const;

    //==============================================================================
    /** Tries to enable or disable buffer underrun safety on devices that support it.
        @returns    true if it's now enabled. If the device doesn't support it, this
                    will always return false.
    */
    bool setBufferUnderrunProtection (const bool shouldBeEnabled);

    //==============================================================================
    /** Returns the number of free blocks on the disk.

        There are 75 blocks per second, at 44100Hz.
    */
    int getNumAvailableAudioBlocks() const;

    /** Adds a track to be written.

        The source passed-in here will be kept by this object, and it will
        be used and deleted at some point in the future, either during the
        burn() method or when this AudioCDBurner object is deleted. Your caller
        method shouldn't keep a reference to it or use it again after passing
        it in here.
    */
    bool addAudioTrack (AudioSource* source, int numSamples);

    //==============================================================================
    /** Receives progress callbacks during a cd-burn operation.
        @see AudioCDBurner::burn()
    */
    class BurnProgressListener
    {
    public:
        BurnProgressListener() throw() {}
        virtual ~BurnProgressListener() {}

        /** Called at intervals to report on the progress of the AudioCDBurner.

            To cancel the burn, return true from this method.
        */
        virtual bool audioCDBurnProgress (float proportionComplete) = 0;
    };

    /** Runs the burn process.
        This method will block until the operation is complete.

        @param listener             the object to receive callbacks about progress
        @param ejectDiscAfterwards  whether to eject the disk after the burn completes
        @param performFakeBurnForTesting    if true, no data will actually be written to the disk
        @param writeSpeed           one of the write speeds from getAvailableWriteSpeeds(), or
                                    0 or less to mean the fastest speed.
    */
    const String burn (BurnProgressListener* listener,
                       bool ejectDiscAfterwards,
                       bool performFakeBurnForTesting,
                       int writeSpeed);

    /** If a burn operation is currently in progress, this tells it to stop
        as soon as possible.

        It's also possible to stop the burn process by returning true from
        BurnProgressListener::audioCDBurnProgress()
    */
    void abortBurn();

    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    AudioCDBurner (const int deviceIndex);

    class Pimpl;
    friend class ScopedPointer<Pimpl>;
    ScopedPointer<Pimpl> pimpl;
};


#endif
#endif   // __JUCE_AUDIOCDBURNER_JUCEHEADER__
